/*
 * i2c_func.c
 *
 *  Created on: 2020��5��25��
 *      Author: xl_25
 */

#include "i2c_func.h"


/************************** Function Prototypes ******************************/


static s32 IicPsSlaveMonitor(u16 Address, u16 DeviceId);
static s32 MuxInitChannel(u16 MuxIicAddr, u8 WriteBuffer);
static s32 FindEepromDevice(u16 Address);
static s32 IicPsFindEeprom(u16 *Eeprom_Addr, u32 *PageSize);
static s32 IicPsConfig(u16 DeviceId);
static s32 IicPsFindDevice(u16 addr);
/************************** Variable Definitions *****************************/

XIicPs IicInstance;		/* The instance of the IIC device. */

u32 Platform;

/*
 * Write buffer for writing a page.
 */
u8 WriteBuffer[sizeof(AddressType) + MAX_SIZE];

u8 ReadBuffer[MAX_SIZE];	/* Read buffer for reading a page. */

u8 tmp_data[34];

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
u32 PageSize;
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

	s32 bytecount;
	u32 Reg;
	int Status;

	bytecount = 5;

	TransmitComplete = FALSE;
	tmp_data[1] = (u8)((*data)>>24);
	tmp_data[2] = (u8)((*data)>>16);
	tmp_data[3] = (u8)((*data)>>8);
	tmp_data[4] = (u8)(*data);
	xil_printf("data = 0x%.8x\r\n", *data);
	xil_printf("tmp_data = 0x%.8x\r\n", tmp_data);
	/*
	 * Send the Data.
	 */
	tmp_data[0] = WriteBuffer[0];
	Status = XIicPs_MasterSendPolled(&IicInstance, tmp_data,
			bytecount, EepromSlvAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	for (int i=0; i < 17; i++ ) {
				xil_printf("tmp_data[%d] = 0x%x\r\n", i,tmp_data[i]);

			}
	

		usleep(250000);
		xil_printf("TransmitComplete = 0x%x, TotalErrorCount = 0x%x\r\n", TransmitComplete,TotalErrorCount);
		Reg = IicInstance.Config.BaseAddress;
		xil_printf("BaseAddres = 0x%x\r\n", Reg);

		Reg = XIicPs_ReadReg(IicInstance.Config.BaseAddress, (u32)XIICPS_ADDR_OFFSET);
		xil_printf("SlaveAddriess = %d\r\n", Reg);

		Reg = XIicPs_ReadReg(IicInstance.Config.BaseAddress, (u32)XIICPS_SR_OFFSET);
		xil_printf("XIICPS_SR_OFFSET = %d\r\n", Reg);

		Reg = XIicPs_ReadReg(IicInstance.Config.BaseAddress, (u32)XIICPS_ISR_OFFSET);
		xil_printf("XIICPS_ISR_OFFSET = %d\r\n", Reg);

		Reg = XIicPs_ReadReg(IicInstance.Config.BaseAddress, (u32)XIICPS_TRANS_SIZE_OFFSET);
		xil_printf("XIICPS_TRANS_SIZE_OFFSET = %d\r\n", Reg);

		Reg = XIicPs_ReadReg(IicInstance.Config.BaseAddress, (u32)XIICPS_TIME_OUT_OFFSET);
		xil_printf("XIICPS_TIME_OUT_OFFSET = %d\r\n", Reg);

		Reg = XIicPs_ReadReg(IicInstance.Config.BaseAddress, (u32)0x24);
		xil_printf("IER = %d\r\n", Reg);

		Reg = XIicPs_ReadReg(IicInstance.Config.BaseAddress, (u32)0x20);
		xil_printf("IMR = %d\r\n", Reg);



	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance));

	/*
	 * Wait for a bit of time to allow the programming to complete
	 */
	usleep(250000);

	return XST_SUCCESS;
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
	s32 Status;

	/*
	 * Position the Pointer in EEPROM.
	 */

	/*
		 * Position the Pointer in EEPROM.
		 */
		if (PageSize == PAGE_SIZE_16) {
			WriteBuffer[0] = (u8) (Address);
			WrBfrOffset = 1;
		} else {
			WriteBuffer[0] = (u8) (Address >> 8);
			WriteBuffer[1] = (u8) (Address);
			WrBfrOffset = 2;
		}

		Status = XIicPs_MasterSendPolled(&IicInstance, tmp_data,
					   1, EepromSlvAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	
	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance));

	/*
	 * Wait for a bit of time to allow the programming to complete
	 */
	usleep(250000);


	/*
	 * Receive the Data.
	 */

	Status = XIicPs_MasterRecvPolled(&IicInstance, tmp_data,
						  4, EepromSlvAddr);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}
	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance)){};

	 *data = *tmp_data;


	return XST_SUCCESS;
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
static s32 MuxInitChannel(u16 MuxIicAddr, u8 WriteBuffer)
{
	u8 Buffer = 0;
	s32 Status = 0;


	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance));

	/*
	 * Send the Data.
	 */
	Status = XIicPs_MasterSendPolled(&IicInstance, &WriteBuffer,1,
					MuxIicAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&IicInstance));

	/*
	 * Receive the Data.
	 */
	Status = XIicPs_MasterRecvPolled(&IicInstance, &Buffer,1, MuxIicAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
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
* @param	DeviceId instance.
*
* @return	XST_SUCCESS if pass, otherwise XST_FAILURE.
*
* @note		None.
*
****************************************************************************/
static s32 IicPsConfig(u16 DeviceId)
{
	s32 Status;
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
	 * Set the IIC serial clock rate.
	 */
	XIicPs_SetSClk(&IicInstance, IIC_SCLK_RATE);
	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is use to figure out the slave device is alive or not.
*
* @param        slave address and Device ID .
*
* @return       XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note         None.
*
*******************************************************************************/

static s32 IicPsFindDevice(u16 addr)
{
	s32 Status;

	Status = IicPsSlaveMonitor(addr,0);
	if (Status == XST_SUCCESS) {
		return XST_SUCCESS;
	}
	Status = IicPsSlaveMonitor(addr,1);
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
static s32 IicPsFindEeprom(u16 *Eeprom_Addr,u32 *PageSize)
{
	s32 Status;
	u32 MuxIndex,Index;
	u8 MuxChannel;

	for(MuxIndex=0;MuxAddr[MuxIndex] != 0;MuxIndex++){
		Status = IicPsFindDevice(MuxAddr[MuxIndex]);
		if (Status == XST_SUCCESS) {
			for(Index=0;EepromAddr[Index] != 0;Index++) {
				for(MuxChannel = MAX_CHANNELS; MuxChannel > 0x0; MuxChannel = MuxChannel >> 1) {
					Status = MuxInitChannel(MuxAddr[MuxIndex], MuxChannel);
					if (Status != XST_SUCCESS) {
						xil_printf("Failed to enable the MUX channel\r\n");
						return XST_FAILURE;
					}
					Status = FindEepromDevice(EepromAddr[Index]);
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
/*****************************************************************************/
/**
*
* This function checks the availability of EEPROM using slave monitor mode.
*
* @param	EEPROM address.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note 	None.
*
*******************************************************************************/
static s32 FindEepromDevice(u16 Address)
{
	u32 Index,IntrStatusReg;
	XIicPs *IicPtr = &IicInstance;


	XIicPs_EnableSlaveMonitor(&IicInstance, Address);

	Index = 0;
	/*
	 * Wait for the Slave Monitor status
	 */
	while (Index < SLV_MON_LOOP_COUNT) {
		Index++;
		/*
		 * Read the Interrupt status register.
		 */
		IntrStatusReg = XIicPs_ReadReg(IicPtr->Config.BaseAddress,
						 (u32)XIICPS_ISR_OFFSET);
		if (0U != (IntrStatusReg & XIICPS_IXR_SLV_RDY_MASK)) {
			XIicPs_DisableSlaveMonitor(&IicInstance);
			XIicPs_WriteReg(IicPtr->Config.BaseAddress,
					(u32)XIICPS_ISR_OFFSET, IntrStatusReg);
			return XST_SUCCESS;
		}
	}
	XIicPs_DisableSlaveMonitor(&IicInstance);

	return XST_FAILURE;
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
static s32 IicPsSlaveMonitor(u16 Address, u16 DeviceId)
{
	u32 Index,IntrStatusReg;
	s32 Status;
	XIicPs *IicPtr;

	/*
	 * Initialize the IIC driver so that it is ready to use.
	 */
	Status = IicPsConfig(DeviceId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	IicPtr = &IicInstance;
	XIicPs_EnableSlaveMonitor(&IicInstance, Address);

	Index = 0;
	/*
	 * Wait for the Slave Monitor Status
	 */
	while (Index < SLV_MON_LOOP_COUNT) {
		Index++;
		/*
		 * Read the Interrupt status register.
		 */
		IntrStatusReg = XIicPs_ReadReg(IicPtr->Config.BaseAddress,
						 (u32)XIICPS_ISR_OFFSET);
		if (0U != (IntrStatusReg & XIICPS_IXR_SLV_RDY_MASK)) {
			XIicPs_DisableSlaveMonitor(&IicInstance);
			XIicPs_WriteReg(IicPtr->Config.BaseAddress,
					(u32)XIICPS_ISR_OFFSET, IntrStatusReg);
			return XST_SUCCESS;
		}
	}
	XIicPs_DisableSlaveMonitor(&IicInstance);
	return XST_FAILURE;
}
