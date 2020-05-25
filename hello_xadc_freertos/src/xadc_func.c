/*
 * xadc_func.c
 *
 *  Created on: 2020Äê5ÔÂ24ÈÕ
 *      Author: xl_25
 */

#include "xadc_func.h"

/************************** Variable Definitions ****************************/



static XAdcPs XAdcInst; 	  	/* XADC driver instance */
static XScuGic InterruptController; 	/* Instance of the GIC driver */


/*
 * Shared variables used to test the callbacks.
 */
static volatile  int TemperatureIntr = FALSE; 	/* Temperature alarm intr */
static volatile  int VccpauxIntr = FALSE;	/* VCCPAUX alarm interrupt */

static int XAdcSetupInterruptSystem(XScuGic *IntcInstancePtr,
				      XAdcPs *XAdcPtr,
				      u16 IntrId );
static void XAdcInterruptHandler(void *CallBackRef);


int xadc_ini()
{
	int Status;
	XAdcPs_Config *ConfigPtr;
	u16 TempData;
	u16 VccpauxData;
	u32 IntrStatus;

	/*
	 * Initialize the XAdc driver.
	 */
	ConfigPtr = XAdcPs_LookupConfig(XADC_DEVICE_ID);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}
	XAdcPs_CfgInitialize(&XAdcInst, ConfigPtr, ConfigPtr->BaseAddress);

	/*
	 * Self Test the XADC/ADC device.
	 */
	Status = XAdcPs_SelfTest(&XAdcInst);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Set XADC in default mode so that alarms are inactive
	 */
	XAdcPs_SetSequencerMode(&XAdcInst, XADCPS_SEQ_MODE_SAFE);

	/* Disable all alarms */
	XAdcPs_SetAlarmEnables(&XAdcInst,0x0000);

	/*
	 * Set up Alarm threshold registers for the on-chip temperature and
	 * VCCPAUX High limit and lower limit so that the alarms occur.
	 */
	TempData = XAdcPs_GetAdcData(&XAdcInst, XADCPS_CH_TEMP);
	XAdcPs_SetAlarmThreshold(&XAdcInst, XADCPS_ATR_TEMP_UPPER,(TempData-0x07FF));
	XAdcPs_SetAlarmThreshold(&XAdcInst, XADCPS_ATR_TEMP_LOWER,(TempData+0x07FF));

	VccpauxData = XAdcPs_GetAdcData(&XAdcInst, XADCPS_CH_VCCPAUX);
	XAdcPs_SetAlarmThreshold(&XAdcInst, XADCPS_ATR_VCCPAUX_UPPER,(VccpauxData-0x07FF));
	XAdcPs_SetAlarmThreshold(&XAdcInst, XADCPS_ATR_VCCPAUX_LOWER,(VccpauxData+0x07FF));

	/*
	 * Setup the interrupt system.
	 */
	Status = XAdcSetupInterruptSystem(&InterruptController, &XAdcInst, XADC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Clear any bits set in the Interrupt Status Register.
	 */
	IntrStatus = XAdcPs_IntrGetStatus(&XAdcInst);
	XAdcPs_IntrClear(&XAdcInst, IntrStatus);


	/*
	 * Enable Alarm 0 interrupt for on-chip temperature and Alarm 5
	 * interrupt for on-chip VCCPAUX.
	 */
	XAdcPs_IntrEnable(&XAdcInst,
			(XADCPS_INTX_ALM5_MASK | XADCPS_INTX_ALM0_MASK));

	/*
	 * Enable Alarm 0 for on-chip temperature and Alarm 5 for on-chip
	 * VCCPAUX in the Configuration Register 1.
	 */
	XAdcPs_SetAlarmEnables(&XAdcInst, (XADCPS_CFR1_ALM_VCCPAUX_MASK
											| XADCPS_CFR1_ALM_TEMP_MASK));


	XAdcPs_SetSequencerMode(&XAdcInst, XADCPS_SEQ_MODE_INDEPENDENT);

	return XST_SUCCESS;
}

int xadc_monitor_acq(SysMon_Info_Acq *MonInfo)
{



	XAdcPs *XAdcInstPtr = &XAdcInst;

	/*
	 * Read the on-chip Temperature Data (Current/Maximum/Minimum)
	 * from the ADC data registers.
	 */
	MonInfo->TempRawData.CurData = XAdcPs_GetAdcData(XAdcInstPtr, XADCPS_CH_TEMP);
	MonInfo->TempData.CurData = XAdcPs_RawToTemperature(MonInfo->TempRawData.CurData);
	printf("\r\nThe Current Temperature is %0d.%03d Centigrades.\r\n",
				(int)(MonInfo->TempData.CurData), XAdcFractionToInt(MonInfo->TempData.CurData));


	MonInfo->TempRawData.MaxData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr, XADCPS_MAX_TEMP);
	MonInfo->TempData.MaxData = XAdcPs_RawToTemperature(MonInfo->TempRawData.MaxData);
	printf("The Maximum Temperature is %0d.%03d Centigrades. \r\n",
				(int)(MonInfo->TempData.MaxData), XAdcFractionToInt(MonInfo->TempData.MaxData));

	MonInfo->TempRawData.MinData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr, XADCPS_MIN_TEMP);
	MonInfo->TempData.MinData = XAdcPs_RawToTemperature(MonInfo->TempRawData.MinData & 0xFFF0);
	printf("The Minimum Temperature is %0d.%03d Centigrades. \r\n",
				(int)(MonInfo->TempData.MinData), XAdcFractionToInt(MonInfo->TempData.MinData));

	/*
	 * Read the VccPint Votage Data (Current/Maximum/Minimum) from the
	 * ADC data registers.
	 */
	MonInfo->VccPintRawData.CurData = XAdcPs_GetAdcData(XAdcInstPtr, XADCPS_CH_VCCPINT);
	MonInfo->VccPintData.CurData = XAdcPs_RawToVoltage(MonInfo->VccPintRawData.CurData);
	printf("\r\nThe Current VCCPINT is %0d.%03d Volts. \r\n",
			(int)(MonInfo->VccPintData.CurData), XAdcFractionToInt(MonInfo->VccPintData.CurData));

	MonInfo->VccPintRawData.MaxData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr,
							XADCPS_MAX_VCCPINT);
	MonInfo->VccPintData.MaxData = XAdcPs_RawToVoltage(MonInfo->VccPintRawData.MaxData);
	printf("The Maximum VCCPINT is %0d.%03d Volts. \r\n",
			(int)(MonInfo->VccPintData.MaxData), XAdcFractionToInt(MonInfo->VccPintData.MaxData));

	MonInfo->VccPintData.MinData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr,
							XADCPS_MIN_VCCPINT);
	MonInfo->VccPintData.MinData = XAdcPs_RawToVoltage(MonInfo->VccPintData.MinData);
	printf("The Minimum VCCPINT is %0d.%03d Volts. \r\n",
			(int)(MonInfo->VccPintData.MinData), XAdcFractionToInt(MonInfo->VccPintData.MinData));

	/*
	 * Read the VccPaux Votage Data (Current/Maximum/Minimum) from the
	 * ADC data registers.
	 */
	MonInfo->VccPauxRawData.CurData = XAdcPs_GetAdcData(XAdcInstPtr, XADCPS_CH_VCCPAUX);
	MonInfo->VccPauxData.CurData = XAdcPs_RawToVoltage(MonInfo->VccPauxRawData.CurData);
	printf("\r\nThe Current VCCPAUX is %0d.%03d Volts. \r\n",
			(int)(MonInfo->VccPauxData.CurData), XAdcFractionToInt(MonInfo->VccPauxData.CurData));

	MonInfo->VccPauxRawData.MaxData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr,
								XADCPS_MAX_VCCPAUX);
	MonInfo->VccPauxData.MaxData = XAdcPs_RawToVoltage(MonInfo->VccPauxRawData.MaxData);
	printf("The Maximum VCCPAUX is %0d.%03d Volts. \r\n",
				(int)(MonInfo->VccPauxData.MaxData), XAdcFractionToInt(MonInfo->VccPauxData.MaxData));


	MonInfo->VccPauxRawData.MinData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr,
								XADCPS_MIN_VCCPAUX);
	MonInfo->VccPauxData.MinData = XAdcPs_RawToVoltage(MonInfo->VccPauxRawData.MinData);
	printf("The Minimum VCCPAUX is %0d.%03d Volts. \r\n\r\n",
				(int)(MonInfo->VccPauxData.MinData), XAdcFractionToInt(MonInfo->VccPauxData.MinData));


	/*
	 * Read the VccPdro Votage Data (Current/Maximum/Minimum) from the
	 * ADC data registers.
	 */
	MonInfo->VccPdroRawData.CurData = XAdcPs_GetAdcData(XAdcInstPtr, XADCPS_CH_VCCPDRO);
	MonInfo->VccPintData.CurData = XAdcPs_RawToVoltage(MonInfo->VccPdroRawData.CurData);
	printf("\r\nThe Current VCCPDDRO is %0d.%03d Volts. \r\n",
			(int)(MonInfo->VccPintData.CurData), XAdcFractionToInt(MonInfo->VccPintData.CurData));

	MonInfo->VccPdroRawData.MaxData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr,
							XADCPS_MAX_VCCPDRO);
	MonInfo->VccPdroData.MaxData = XAdcPs_RawToVoltage(MonInfo->VccPdroRawData.MaxData);
	printf("The Maximum VCCPDDRO is %0d.%03d Volts. \r\n",
			(int)(MonInfo->VccPdroData.MaxData), XAdcFractionToInt(MonInfo->VccPdroData.MaxData));

	MonInfo->VccPdroRawData.MinData = XAdcPs_GetMinMaxMeasurement(XAdcInstPtr,
							XADCPS_MIN_VCCPDRO);
	MonInfo->VccPdroData.MinData = XAdcPs_RawToVoltage(MonInfo->VccPdroRawData.MinData);
	printf("The Minimum VCCPDDRO is %0d.%03d Volts. \r\n",
			(int)(MonInfo->VccPdroData.MinData), XAdcFractionToInt(MonInfo->VccPdroData.MinData));


	printf("Exiting the XAdc Polled Example. \r\n");

	return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* This function sets up the interrupt system so interrupts can occur for the
* XADC/ADC.  The function is application-specific since the actual
* system may or may not have an interrupt controller. The XADC/ADC
* device could be directly connected to a processor without an interrupt
* controller. The user should modify this function to fit the application.
*
* @param	IntcInstancePtr is a pointer to the Interrupt Controller
*		driver Instance.
* @param	XAdcPtr is a pointer to the driver instance for the System
* 		Monitor device which is going to be connected to the interrupt
*		controller.
* @param	IntrId is XPAR_<INTC_instance>_<SYSMON_ADC_instance>_VEC_ID
*		value from xparameters.h.
*
* @return	XST_SUCCESS if successful, or XST_FAILURE.
*
* @note		None.
*
*
****************************************************************************/
static int XAdcSetupInterruptSystem(XScuGic *IntcInstancePtr,
				      XAdcPs *XAdcPtr,
				      u16 IntrId )
{

	int Status;
	XScuGic_Config *IntcConfig; /* Instance of the interrupt controller */


	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, IntrId,
				(Xil_InterruptHandler)XAdcInterruptHandler,
				(void *)XAdcPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the XADC device.
	 */
	XScuGic_Enable(IntcInstancePtr, IntrId);

	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler) XScuGic_InterruptHandler,
				IntcInstancePtr);
	/*
	 * Enable exceptions.
	 */
	Xil_ExceptionEnable();


	return XST_SUCCESS;

}

/*****************************************************************************/
/**
*
* This function is the Interrupt Service Routine for the XADC device.
* It will be called by the processor whenever an interrupt is asserted
* by the device.
*
* There are 8 different interrupts supported
*	- Over Temperature
*	- ALARM 0 to ALARM 7
*
* This function only handles ALARM 0 and ALARM 2 interrupts. User of this
* code may need to modify the code to meet needs of the application.
*
* @param	CallBackRef is the callback reference passed from the Interrupt
*		controller driver, which in our case is a pointer to the
*		driver instance.
*
* @return	None.
*
* @note		This function is called within interrupt context.
*
******************************************************************************/
static void XAdcInterruptHandler(void *CallBackRef)
{
	u32 IntrStatusValue;
	XAdcPs *XAdcPtr = (XAdcPs *)CallBackRef;

	/*
	 * Get the interrupt status from the device and check the value.
	 */

	IntrStatusValue = XAdcPs_IntrGetStatus(XAdcPtr);

	if (IntrStatusValue & XADCPS_INTX_ALM0_MASK) {
		/*
		 * Set Temperature interrupt flag so the code
		 * in application context can be aware of this interrupt.
		 */
		TemperatureIntr = TRUE;

		/* Disable Temperature interrupt */
		XAdcPs_IntrDisable(XAdcPtr,XADCPS_INTX_ALM0_MASK);

	}
	if (IntrStatusValue & XADCPS_INTX_ALM5_MASK) {
			/*
			 * Set Temperature interrupt flag so the code
			 * in application context can be aware of this interrupt.
			 */
		VccpauxIntr = TRUE;

		/* Disable VccPAUX interrupt */
		XAdcPs_IntrDisable(XAdcPtr,XADCPS_INTX_ALM5_MASK);
	}

	/*
	 * Clear all bits in Interrupt Status Register.
	 */
	XAdcPs_IntrClear(XAdcPtr, IntrStatusValue);
 }

/****************************************************************************/
/**
*
* This function converts the fraction part of the given floating point number
* (after the decimal point)to an integer.
*
* @param	FloatNum is the floating point number.
*
* @return	Integer number to a precision of 3 digits.
*
* @note
* This function is used in the printing of floating point data to a STDIO device
* using the xil_printf function. The xil_printf is a very small foot-print
* printf function and does not support the printing of floating point numbers.
*
*****************************************************************************/
int XAdcFractionToInt(float FloatNum)
{
	float Temp;

	Temp = FloatNum;
	if (FloatNum < 0) {
		Temp = -(FloatNum);
	}

	return( ((int)((Temp -(float)((int)Temp)) * (1000.0f))));
}
