/*
 * uart0_func.c
 *
 *  Created on: 2020Äê5ÔÂ25ÈÕ
 *      Author: xl_25
 */

#include "uart0_func.h"

/************************** Variable Definitions *****************************/

XUartPs Uart_Ps;		/* The instance of the UART Driver */



int uart0_ini(void)
{
	int Status;
	u8 HelloWorld[] = "Hello World,I'm looking for Arduino via UART0";
	int SentCount = 0;

	XUartPs_Config *Config;

	xil_printf( "Start UART0 INI\r\n" );

	/*
	 * Initialize the UART driver so that it's ready to use
	 * Look up the configuration in the config table and then initialize it.
	 */
	Config = XUartPs_LookupConfig(UART_DEVICE_ID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	Status = XUartPs_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XUartPs_SetBaudRate(&Uart_Ps, 115200);

	while (SentCount < (sizeof(HelloWorld) - 1)) {
		/* Transmit the data */
		SentCount += XUartPs_Send(&Uart_Ps,
					   &HelloWorld[SentCount], 1);
	}

	return Status;

}
