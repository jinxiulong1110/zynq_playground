/*
 * xadc_func.h
 *
 *  Created on: 2020Äê5ÔÂ24ÈÕ
 *      Author: xl_25
 */

#ifndef SRC_XADC_FUNC_H_
#define SRC_XADC_FUNC_H_

/***************************** Include Files ********************************/

#include "xparameters.h"
#include "xadcps.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xstatus.h"
#include "stdio.h"

/************************** Constant Definitions ****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */

#define XADC_DEVICE_ID		XPAR_XADCPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define INTR_ID				XPAR_XADCPS_INT_ID
#define printf				xil_printf

typedef struct {
	float CurData;
	float MaxData;
	float MinData;
} SysMon_Info_type;

typedef struct {
	u32 CurData;
	u32 MaxData;
	u32 MinData;
} SysMon_RawInfo_type;

typedef struct {
	SysMon_RawInfo_type TempRawData;
	SysMon_RawInfo_type VccPintRawData;
	SysMon_RawInfo_type VccPauxRawData;
	SysMon_RawInfo_type VccPdroRawData;
	SysMon_Info_type TempData;
	SysMon_Info_type VccPintData;
	SysMon_Info_type VccPauxData;
	SysMon_Info_type VccPdroData;

} SysMon_Info_Acq;

int xadc_ini(void);
int xadc_monitor_acq(SysMon_Info_Acq *MonInfo);


#endif /* SRC_XADC_FUNC_H_ */
