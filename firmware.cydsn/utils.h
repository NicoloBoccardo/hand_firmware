// -----------------------------------------------------------------------------
// Copyright (C)  qbrobotics. All rights reserved.
// www.qbrobotics.com
// -----------------------------------------------------------------------------

/** 
* \file 		utils.h
*
* \brief 		Declaration of utility functions.
* \date 		Feb 16, 2014
* \author 		qbrobotics
* \copyright	(C)  qbrobotics. All rights reserved.
*/


#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <globals.h>

//------------------------------------------------------------------     DEFINES

//high pass filter coefs
// #define NZEROSHP 		2
// #define NPOLESHP 		2

// #define GAINHP   		((int32)(1.022463023e+00 * 1024))
// #define GAINL_1_HP   	((int32)(-0.9565436765 * 1024))
// #define GAINL_2_HP   	((int32)(1.9555782403 * 1024))

// //low pass filter coefs
// #define NZEROSLP 		2
// #define NPOLESLP 		2

// #define GAINLP 		    ((int32)1.140829091e+04)
// #define GAINL_1_LP  	((int32)(-0.9736948720 * 1024))
// #define GAINL_2_LP    	((int32)(1.9733442498 * 1024))


#define TIMER_CLOCK 10000


//-------------------------------------------------------------     DECLARATIONS


int32 filter_ch1(int32 value);
int32 filter_ch2(int32 value);
int32 filter_i1(int32 value);
int32 filter_i2(int32 value);

uint8 BITChecksum(uint32);

void ms_delay(uint32);

void calibrate(void);
void calibration_increment(void);

#endif


/* [] END OF FILE */
