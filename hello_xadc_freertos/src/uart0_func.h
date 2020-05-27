/*
 * uart0_func.h
 *
 *  Created on: 2020Äê5ÔÂ25ÈÕ
 *      Author: xl_25
 */

#ifndef SRC_UART0_FUNC_H_
#define SRC_UART0_FUNC_H_

/***************************** Include Files *********************************/

#include "xparameters.h"
#include "xuartps.h"
#include "xil_printf.h"

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define UART_DEVICE_ID                  XPAR_XUARTPS_0_DEVICE_ID

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

int uart0_ini(void);

#endif /* SRC_UART0_FUNC_H_ */
