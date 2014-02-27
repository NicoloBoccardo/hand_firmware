// ----------------------------------------------------------------------------
// Copyright (C)  qbrobotics. All rights reserved.
// www.qbrobotics.com
// ----------------------------------------------------------------------------

/** 
* \file 		interruptions.h
*
* \brief 		Interruptions header file.
* \date 		Feb 06, 2012
* \author 		qbrobotics
* \copyright	(C)  qbrobotics. All rights reserved.
*/

#ifndef INTERRUPTIONS_H_INCLUDED
#define INTERRUPTIONS_H_INCLUDED
// ----------------------------------------------------------------------------

//=====================================================     function definitions
CY_ISR_PROTO(ISR_RS485_RX_ExInterrupt);
CY_ISR_PROTO(ISR_MEASUREMENTS_ExInterrupt);
CY_ISR_PROTO(ISR_CALIBRATE_ExInterrupt);

// ----------------------------------------------------------------------------
#endif

/* [] END OF FILE */