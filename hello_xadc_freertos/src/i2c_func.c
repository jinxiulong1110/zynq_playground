/*
 * i2c_func.c
 *
 *  Created on: 2020年5月25日
 *      Author: xl_25
 */

#include "i2c_func.h"


/************************** Function Prototypes ******************************/

static void Handler(void *CallBackRef, u32 Event);
static int IicPsSlaveMonitor(u16 Address, u16 DeviceId, u32 Int_Id);
static int SetupInterruptSystem(XIicPs *IicPsPtr, u32 Int_Id);
static int MuxInitChannel(u16 MuxIicAddr, u8 WriteBuffer);
static int FindEepromDevice(u16 Address);
static int IicPsFindEeprom(u16 *Eeprom_Addr, int *PageSize);
static int IicPsConfig(u16 DeviceId, u32 Int_Id);
static int IicPsFindDevice(u16 addr);
/************************** Variable Definitions *****************************/

XIicPs IicInstance;		/* The instance of the IIC device. */
XScuGic InterruptController;	/* The instance of the Interrupt Controller. */

u32 Platform;

/*
 * Write buffer for writing a page.
 */
u8 WriteBuffer[sizeof(AddressType) + MAX_SIZE];

u8 ReadBuffer[MAX_SIZE];	/* Read buffer for reading a page. */

int WrBfrOffset;

volatile u8 TransmitComplete;	/**< Flag to check completion of Transmission */
volatile u8 ReceiveComplete;	/**< Flag to check completion of Reception */
volatile u32 TotalErrorCount;	/**< Total Error Count Flag */
volatile u32 SlaveResponse;		/**< Slave Response Flag */

/**Searching for the required EEPROM Address and user can also add
 * their own EEPROM Address in the below array list**/
u16 EepromAddr[] = {0x50};
u16 MuxAddr[] = {0x50,0};
u16 EepromSlvAddr;
int PageSize;
//XIicPs IicInstance;

int i2c_ini(void)
{
	int Status;



	AddressType Address = EEPROM_START_ADDRESS;



	Status = IicPsFindEeprom(&EepromSlvAddr,&PageSize);
	xil_printf("EepromSlvAddr = 0x%x\r\n", EepromSlvAddr);
	xil_printf("IIC EEPROM Page Size %d\r\n", PageSize);
	if (Status != XST_SUCCESS) {
		xil_printf("IIC EEPROM Find Failed\r\n");
		return XST_FAILURE;
	}
	/*
	 * Initialize the data to write and the read buffer.
	 */
	if (PageSize == PAGE_SIZE_16) {
		WriteBuffer[0] = (u8) (Address);
		WrBfrOffset = 1;
	} else {
		WriteBuffer[0] = (u8) (Address >> 8);
		WriteBuffer[1] = (u8) (Address);
		WrBfrOffset = 2;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function writes a buffer of data to the IIC serial EEPROM.
*
* @param	ByteCount contains the number of bytes in the buffer to be
*		written.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The Byte count should not exceed the page size of the EEPROM as
*		noted by the constant PAGE_SIZE.
*
******************************************************************************/

int i2c_write_u32(u32 *data, AddressType Address, char PageNr)
{
	u8 tmp_data[4];

	TransmitComplete = FALSE;
	*tmp_data = *data;
	/*
	 * Send the Data.
	 */
	XIicPs_MasterSend(&IicInstance, tmp_data,
			   4, EepromSlvAddr);

	/*
	 * Wait for the entire buffer to be sent, letting the interrupt
	 * processing work in the background, this function may get
	 * locked up in this loop if the interrupts are not working
	 * correctly.
	 */
	while (TransmitComplete == FALSE) {
		if (0 != TotalErrorCount) {
			return XST_FAILURE;
		}
}

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance));

	/*
	 * Wait for a bit of time to allow the programming to complete
	 */
	usleep(250000);

	return XST_FAILURE;
}

/*****************************************************************************/
/**
* This function reads data from the IIC serial EEPROM into a specified buffer.
*
* @param	BufferPtr contains the address of the data buffer to be filled.
* @param	ByteCount contains the number of bytes in the buffer to be read.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None.
*
******************************************************************************/


int i2c_read_u32(u32 *data, AddressType Address, char PageNr)
{
	u8 tmp_data[4];

	/*
	 * Position the Pointer in EEPROM.
	 */


	ReceiveComplete = FALSE;

	/*
	 * Receive the Data.
	 */
	XIicPs_MasterRecv(&IicInstance, tmp_data,
			   4, EepromSlvAddr);

	while (ReceiveComplete == FALSE) {
		if (0 != TotalErrorCount) {
			return XST_FAILURE;
		}
	}

	 *data = *tmp_data;

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance));

	return XST_SUCCESS;
}



/******************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for the IIC.
*
* @param	IicPsPtr contains a pointer to the instance of the Iic
*		which is going to be connected to the interrupt controller.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
*******************************************************************************/
static int SetupInterruptSystem(XIicPs *IicPsPtr, u32 Int_Id)
{
	int Status;
	XScuGic_Config *IntcConfig; /* Instance of the interrupt controller */

	Xil_ExceptionInit();

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				(Xil_ExceptionHandler)XScuGic_InterruptHandler,
				&InterruptController);

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(&InterruptController, Int_Id,
			(Xil_InterruptHandler)XIicPs_MasterInterruptHandler,
			(void *)IicPsPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the Iic device.
	 */
	XScuGic_Enable(&InterruptController, Int_Id);


	/*
	 * Enable interrupts in the Processor.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is the handler which performs processing to handle data events
* from the IIC.  It is called from an interrupt context such that the amount
* of processing performed should be minimized.
*
* This handler provides an example of how to handle data for the IIC and
* is application specific.
*
* @param	CallBackRef contains a callback reference from the driver, in
*		this case it is the instance pointer for the IIC driver.
* @param	Event contains the specific kind of event that has occurred.
* @param	EventData contains the number of bytes sent or received for sent
*		and receive events.
*
* @return	None.
*
* @note		None.
*
*******************************************************************************/
void Handler(void *CallBackRef, u32 Event)
{
	/*
	 * All of the data transfer has been finished.
	 */

	if (0 != (Event & XIICPS_EVENT_COMPLETE_SEND)) {
		TransmitComplete = TRUE;
	} else if (0 != (Event & XIICPS_EVENT_COMPLETE_RECV)){
		ReceiveComplete = TRUE;
	} else if (0 != (Event & XIICPS_EVENT_SLAVE_RDY)) {
		SlaveResponse = TRUE;
	} else if (0 != (Event & XIICPS_EVENT_ERROR)){
		TotalErrorCount++;
	}
}

/*****************************************************************************/
/**
* This function initializes the IIC MUX to select the required channel.
*
* @param	MuxAddress and Channel select value.
*
* @return	XST_SUCCESS if pass, otherwise XST_FAILURE.
*
* @note		None.
*
****************************************************************************/
static int MuxInitChannel(u16 MuxIicAddr, u8 WriteBuffer)
{
	u8 Buffer = 0;

	TotalErrorCount = 0;
	TransmitComplete = FALSE;
	TotalErrorCount = 0;

	XIicPs_MasterSend(&IicInstance, &WriteBuffer,1,MuxIicAddr);
	while (TransmitComplete == FALSE) {
		if (0 != TotalErrorCount) {
			return XST_FAILURE;
		}
	}
	/*
	 * Wait until bus is idle to start another transfer.
	 */

	while (XIicPs_BusIsBusy(&IicInstance));

	ReceiveComplete = FALSE;
	/*
	 * Receive the Data.
	 */
	XIicPs_MasterRecv(&IicInstance, &Buffer,1, MuxIicAddr);

	while (ReceiveComplete == FALSE) {
		if (0 != TotalErrorCount) {
			return XST_FAILURE;
		}
	}
	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance));

	return XST_SUCCESS;
}
/*****************************************************************************/
/**
* This function perform the initial configuration for the IICPS Device.
*
* @param	DeviceId instance and Interrupt ID mapped to the device.
*
* @return	XST_SUCCESS if pass, otherwise XST_FAILURE.
*
* @note		None.
*
****************************************************************************/
static int IicPsConfig(u16 DeviceId, u32 Int_Id)
{
	int Status;
	XIicPs_Config *ConfigPtr;	/* Pointer to configuration data */

	/*
	 * Initialize the IIC driver so that it is ready to use.
	 */
	ConfigPtr = XIicPs_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(&IicInstance, ConfigPtr,
					ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Setup the Interrupt System.
	 */
	Status = SetupInterruptSystem(&IicInstance, Int_Id);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Setup the handlers for the IIC that will be called from the
	 * interrupt context when data has been sent and received, specify a
	 * pointer to the IIC driver instance as the callback reference so
	 * the handlers are able to access the instance data.
	 */
	XIicPs_SetStatusHandler(&IicInstance, (void *) &IicInstance, Handler);

	/*
	 * Set the IIC serial clock rate.
	 */
	XIicPs_SetSClk(&IicInstance, IIC_SCLK_RATE);
	return XST_SUCCESS;
}

static int IicPsFindDevice(u16 addr)
{
	int Status;

	Status = IicPsSlaveMonitor(addr,0,XPAR_XIICPS_0_INTR);
	if (Status == XST_SUCCESS) {
		return XST_SUCCESS;
	}
	Status = IicPsSlaveMonitor(addr,1,XPAR_XIICPS_1_INTR);
	if (Status == XST_SUCCESS) {
		return XST_SUCCESS;
	}
	Status = IicPsSlaveMonitor(addr,0,XPAR_XIICPS_1_INTR);
	if (Status == XST_SUCCESS) {
		return XST_SUCCESS;
	}
	Status = IicPsSlaveMonitor(addr,1,XPAR_XIICPS_0_INTR);
	if (Status == XST_SUCCESS) {
		return XST_SUCCESS;
	}
	return XST_FAILURE;
}
/*****************************************************************************/
/**
* This function is use to figure out the Eeprom slave device
*
* @param	addr: u16 variable
*
* @return	XST_SUCCESS if successful and also update the epprom slave
* device address in addr variable else XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
static int IicPsFindEeprom(u16 *Eeprom_Addr,int *PageSize)
{
	int Status;
	int MuxIndex,Index;
	u8 MuxChannel;

	for(MuxIndex=0;MuxAddr[MuxIndex] != 0;MuxIndex++){
		Status = IicPsFindDevice(MuxAddr[MuxIndex]);
		if (Status == XST_SUCCESS) {
			for(Index=0;EepromAddr[Index] != 0;Index++) {
					for(MuxChannel = 0x01; MuxChannel <= MAX_CHANNELS; MuxChannel = MuxChannel << 1) {
					Status = MuxInitChannel(MuxAddr[MuxIndex], MuxChannel);
					if (Status != XST_SUCCESS) {
						xil_printf("Failed to enable the MUX channel\r\n");
						return XST_FAILURE;
					}
					Status = FindEepromDevice(EepromAddr[Index]);
					FindEepromDevice(MUX_ADDR);
					if (Status == XST_SUCCESS) {
						*Eeprom_Addr = EepromAddr[Index];
						*PageSize = PAGE_SIZE_16;
						return XST_SUCCESS;
					}
				}
			}
		}
	}
	for(Index=0;EepromAddr[Index] != 0;Index++) {
		Status = IicPsFindDevice(EepromAddr[Index]);
		if (Status == XST_SUCCESS) {
			*Eeprom_Addr = EepromAddr[Index];
			*PageSize = PAGE_SIZE_32;
			return XST_SUCCESS;
		}
	}
	return XST_FAILURE;
}
static int FindEepromDevice(u16 Address)
{
	int Index;
	XIicPs *IicPtr = &IicInstance;
	SlaveResponse = FALSE;

	XIicPs_DisableAllInterrupts(IicPtr->Config.BaseAddress);
	XIicPs_EnableSlaveMonitor(&IicInstance, Address);

		TotalErrorCount = 0;

		Index = 0;

		/*
		 * Wait for the Slave Monitor Interrupt, the interrupt processing
		 * works in the background, this function may get locked up in this
		 * loop if the interrupts are not working correctly or the slave
		 * never responds.
		 */
		while ((!SlaveResponse) && (Index < SLV_MON_LOOP_COUNT)) {
			Index++;

			/*
			 * Ignore any errors. The hardware generates NACK interrupts
			 * if the slave is not present.
			 */
			if (0 != TotalErrorCount) {
				xil_printf("Test error unexpected NACK\n");
			}
		}

		if (Index >= SLV_MON_LOOP_COUNT) {
			XIicPs_DisableSlaveMonitor(&IicInstance);
			return XST_FAILURE;

		}

		XIicPs_DisableSlaveMonitor(&IicInstance);
		return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function checks the availability of a slave using slave monitor mode.
*
* @param	DeviceId is the Device ID of the IicPs Device and is the
*		XPAR_<IICPS_instance>_DEVICE_ID value from xparameters.h
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note 	None.
*
*******************************************************************************/
static int IicPsSlaveMonitor(u16 Address, u16 DeviceId, u32 Int_Id)
{
	u32 Index;
	int Status;
	XIicPs *IicPtr;

	SlaveResponse = FALSE;
	/*
	 * Initialize the IIC driver so that it is ready to use.
	 */
	Status = IicPsConfig(DeviceId,Int_Id);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	IicPtr = &IicInstance;
	XIicPs_DisableAllInterrupts(IicPtr->Config.BaseAddress);
	XIicPs_EnableSlaveMonitor(&IicInstance, Address);

	TotalErrorCount = 0;

	Index = 0;

	/*
	 * Wait for the Slave Monitor Interrupt, the interrupt processing
	 * works in the background, this function may get locked up in this
	 * loop if the interrupts are not working correctly or the slave
	 * never responds.
	 */
	while ((!SlaveResponse) && (Index < SLV_MON_LOOP_COUNT)) {
		Index++;

		/*
		 * Ignore any errors. The hardware generates NACK interrupts
		 * if the slave is not present.
		 */
		if (0 != TotalErrorCount) {
			xil_printf("Test error unexpected NACK\n");
			return XST_FAILURE;
		}
	}

	if (Index >= SLV_MON_LOOP_COUNT) {
		return XST_FAILURE;

	}

	XIicPs_DisableSlaveMonitor(&IicInstance);
	return XST_SUCCESS;
}

/******************************************************************************/
