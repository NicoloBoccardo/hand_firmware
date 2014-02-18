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
#define NZEROSHP 		2
#define NPOLESHP 		2
#define GAINHP   		1.022463023e+00
#define GAINL_1_HP   	-0.9565436765
#define GAINL_2_HP   	1.9555782403

//low pass filter coefs
#define NZEROSLP 		2
#define NPOLESLP 		2
#define GAINLP 		    1.140829091e+04

#define GAINL_1_LP  	-0.9736948720
#define GAINL_2_LP    	1.9733442498


#define TIMER_CLOCK 10000


//-------------------------------------------------------------     DECLARATIONS


float filter_ch1(float value);
float filter_ch2(float value);

uint8 BITChecksum(uint32);

void ms_delay(uint32);

#endif


/* [] END OF FILE */
