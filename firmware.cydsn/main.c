// ----------------------------------------------------------------------------
// Copyright (C)  qbrobotics. All rights reserved.
// www.qbrobotics.com
// ----------------------------------------------------------------------------


/** 
* \file 		main.c
*
* \brief 		Firmware main file.
* \date 		May 16, 2012
* \author 		qbrobotics
* \copyright	(C)  qbrobotics. All rights reserved.
*/

/**
* \mainpage		Firmware
* \brief		This is the firmware of the qb move.
* \version		0.1 beta 4
*
* \author		_qbrobotics_
*
* \date 		May 16, 2012
*
* \details		This is the firmware of the qb move.
*
* \copyright	(C)  qbrobotics. All rights reserved.
*
*/


// ----------------------------------------------------------------------------
// This version changes:
//		- not reported


//=================================================================     includes

#include <device.h>
#include <globals.h> // ALL GLOBAL DEFINITIONS, STRUCTURES AND MACROS HERE
#include <interruptions.h>
#include <command_processing.h>	
#include <utils.h>

//==============================================================================
//                                                                 MAIN FUNCTION
//==============================================================================

int i;			//iterator


void main()
{
//====================================     initializations - psoc and components
	
	// EEPROM
	
	EEPROM_Start();
	memRecall();                                     	// recall configuration

	// RS485
	
	UART_RS485_Stop();									// stop UART
	UART_RS485_Start();									// start UART
	UART_RS485_Init();

	UART_RS485_ClearRxBuffer();
	UART_RS485_ClearTxBuffer();

	ISR_RS485_RX_StartEx(ISR_RS485_RX_ExInterrupt);		// RS485 isr function    

	// PWM
    
	PWM_MOTORS_Start();
	ISR_MOTORS_CONTROL_StartEx(ISR_MOTORS_CONTROL_ExInterrupt);
	PWM_MOTORS_WriteCompare1(0);
	PWM_MOTORS_WriteCompare2(0);
	MOTOR_DIR_Write(0);
	MOTOR_ON_OFF_Write(0x00);	
	
	// SSI encoder initializations
	
	COUNTER_ENC_Start();
	SHIFTREG_ENC_1_Start();
	SHIFTREG_ENC_2_Start();
	SHIFTREG_ENC_3_Start();
		
	// ADC
	
    ADC_Start();                                     // start ADC
    ADC_StartConvert();
	// measurements isr function
    ISR_MEASUREMENTS_StartEx(ISR_MEASUREMENTS_ExInterrupt);
    AMUXSEQ_MOTORS_Start();                              // start mux

    // Calibrate interrupt init
    ISR_CALIBRATE_StartEx(ISR_CALIBRATE_ExInterrupt);

    // Timer init
    MY_TIMER_Init();
    MY_TIMER_Stop();
	
    RS485_CTS_Write(0);   

    CYGlobalIntEnable; 									// enable interrupts        

//========================================     initializations - clean variables


    for (i = 0; i < NUM_OF_MOTORS; i++) {
    	g_ref.pos[i] = 0;	
    }
	g_ref.onoff = c_mem.activ;

	for (i = 0; i < NUM_OF_SENSORS; i++) {
		g_meas.pos[i] = 0;
		g_meas.rot[i] = 0;	
	}

	g_rx.length			= 0;
	g_rx.ready			= 0;
	
	// Activating motors
	g_ref.pos[0] = g_meas.pos[0];
	g_ref.pos[1] = g_meas.pos[1];
	MOTOR_ON_OFF_Write(g_ref.onoff);

	// Calculate conversion factor
	device.tension_conv_factor = ((0.25 * 101.0 * 1000) / 1638.4); //derives from datasheet calculations

	


//=========================================================     application loop

	for(;;)
	{
	    if(UART_RS485_ReadRxStatus() & UART_RS485_RX_STS_SOFT_BUFF_OVER)
            UART_RS485_ClearRxBuffer();
	}
}



/* [] END OF FILE */