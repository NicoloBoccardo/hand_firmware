// -----------------------------------------------------------------------------
// Copyright (C)  qbrobotics. All rights reserved.
// www.qbrobotics.com
// -----------------------------------------------------------------------------

/** 
* \file 		utils.h
*
* \brief 		Definition of utility functions.
* \date 		Feb 16, 2014
* \author 		qbrobotics
* \copyright	(C)  qbrobotics. All rights reserved.
*/

#include <utils.h>
#include <math.h>

//--------------------------------------------------------------     DEFINITIONS


float max_FDS=0.001;
float max_EDC=0.001;

float min_FDS=0.3;
float min_EDC=1;
float min_ave=0;
float min_ave_ctr=0;
float coeff_ave=0;

// double max_FDS=0.0;
// double max_EDC=0.0;

float pos_ave=0;
float pos_ave_ctr=0;
float pos_coeff_ave=0;

//==============================================================================
//																	  FILTER CH1
//==============================================================================

float filter_ch1(float value) {
	// high pass
	static float xvHP1[NZEROSLP+1]={0};
	static float yvHP1[NPOLESLP+1]={0};

	// low pass
	static float xvLP1[NZEROSLP+1]={0};
	static float yvLP1[NPOLESLP+1]={0};


	// xvHP1[0] = xvHP1[1];
	// xvHP1[1] = xvHP1[2];
	// xvHP1[2] = (value) / GAINHP;

	// yvHP1[0] = yvHP1[1];
	// yvHP1[1] = yvHP1[2]; 
	// yvHP1[2] =    (xvHP1[0] + xvHP1[2]) - 2 * xvHP1[1]
	//             + ( GAINL_1_HP  * yvHP1[0]) + (  GAINL_2_HP  * yvHP1[1]);




	xvLP1[0] = xvLP1[1]; xvLP1[1] = xvLP1[2]; 
	// xvLP1[2] = fabs(yvHP1[2]) / GAINLP;
	xvLP1[2] = fabs(value) / GAINLP;
	yvLP1[0] = yvLP1[1]; yvLP1[1] = yvLP1[2]; 
	yvLP1[2] =    (xvLP1[0] + xvLP1[2]) + 2 * xvLP1[1]
	            + ( GAINL_1_LP * yvLP1[0]) + (  GAINL_2_LP * yvLP1[1]);



	return yvLP1[2];
}


//==============================================================================
//																	  FILTER CH2
//==============================================================================

float filter_ch2(float value) {
	// high pass
	static float xvHP2[NZEROSLP+1]={0};
	static float yvHP2[NPOLESLP+1]={0};

	// low pass
	static float xvLP2[NZEROSLP+1]={0};
	static float yvLP2[NPOLESLP+1]={0};


	// xvHP2[0] = xvHP2[1];
	// xvHP2[1] = xvHP2[2]; 
	// xvHP2[2] = (value) / GAINHP;

	// yvHP2[0] = yvHP2[1];
	// yvHP2[1] = yvHP2[2]; 
	// yvHP2[2] =    (xvHP2[0] + xvHP2[2]) - 2 * xvHP2[1]
	//             + ( GAINL_1_HP  * yvHP2[0]) + (  GAINL_2_HP  * yvHP2[1]);

	

	xvLP2[0] = xvLP2[1]; xvLP2[1] = xvLP2[2]; 
	xvLP2[2] = fabs(value) / GAINLP;
	// xvLP2[2] = fabs(yvHP2[2]) / GAINLP;
	yvLP2[0] = yvLP2[1]; yvLP2[1] = yvLP2[2]; 
	yvLP2[2] =    (xvLP2[0] + xvLP2[2]) + 2 * xvLP2[1]
	            + ( GAINL_1_LP * yvLP2[0]) + (  GAINL_2_LP * yvLP2[1]);


	return yvLP2[2];
}


//==============================================================================
//																	BIT CHECKSUM
//==============================================================================


uint8 BITChecksum(uint32 mydata) {
	uint8 i;
	uint8 checksum = 0;
	for(i = 0; i < 31; ++i)
	{
       	checksum = checksum ^ (mydata & 1);
		mydata = mydata >> 1;
	}
	return checksum;
}

//==============================================================================
//																	    MS_DELAY
//==============================================================================


void ms_delay(uint32 ms) {
	uint32 period = (TIMER_CLOCK / 1000) * ms;
    MY_TIMER_WritePeriod(period);
    MY_TIMER_Start(); // start the timeout counter
    while(MY_TIMER_ReadPeriod());
    MY_TIMER_Stop();
}


/* [] END OF FILE */
