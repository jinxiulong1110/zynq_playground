/*
 * i2c_func.h
 *
 *  Created on: 2020Äê5ÔÂ25ÈÕ
 *      Author: xl_25
 */

#ifndef SRC_I2C_FUNC_H_
#define SRC_I2C_FUNC_H_

/***************************** Include Files *********************************/

#include "xparameters.h"
#include "sleep.h"
#include "xiicps.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xplatform_info.h"

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */

#define INTC_DEVICE_ID	XPAR_SCUGIC_SINGLE_DEVICE_ID

/*
 * The following constant defines the address of the IIC Slave device on the
 * IIC bus. Note that since the address is only 7 bits, this constant is the
 * address divided by 2.
 */

#define IIC_SCLK_RATE		100000
#define SLV_MON_LOOP_COUNT 0x000FFFFF	/**< Slave Monitor Loop Count*/
#define MUX_ADDR 0x50
#define MAX_CHANNELS 0x08

/*
 * The page size determines how much data should be written at a time.
 * The write function should be called with this as a maximum byte count.
 */
#define MAX_SIZE		32
#define PAGE_SIZE_16	16
#define PAGE_SIZE_32	32

/*
 * The Starting address in the IIC EEPROM on which this test is performed.
 */
#define EEPROM_START_ADDRESS	0

/**************************** Type Definitions *******************************/

/*
 * The AddressType should be u8 as the address pointer in the on-board
 * EEPROM is 1 byte.
 */
typedef u16 AddressType;

/************************** Function Prototypes ******************************/

int i2c_ini(void);
int i2c_write_u32(u32 *data, AddressType Address, char PageNr);
int i2c_read_u32(u32 *data, AddressType Address, char PageNr);

#endif /* SRC_I2C_FUNC_H_ */
