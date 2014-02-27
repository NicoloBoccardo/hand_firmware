 // ----------------------------------------------------------------------------
// Copyright (C)  qbrobotics. All rights reserved.
// www.qbrobotics.com
// ----------------------------------------------------------------------------

/** 
* \file 		interruptions.c
*
* \brief 		Interruption functions are in this file.
* \date 		Feb 06, 2012
* \author 		qbrobotics
* \copyright	(C)  qbrobotics. All rights reserved.
*/


//=================================================================     includes
#include <interruptions.h>
#include <command_processing.h>
#include <globals.h>
#include <utils.h>


//==============================================================================
//                                                            RS485 RX INTERRUPT
//==============================================================================
// Processing RS-485 data frame:
// 
// - 0: 	Waits for beggining characters
// - 1:		Waits for ID;
// - 2:		Data length;
// - 3:		Receive all bytes;
// - 4:		Wait for another device end of transmission;
//
//==============================================================================

CY_ISR(ISR_RS485_RX_ExInterrupt){
	 
//===============================================     local variables definition

	static uint8 	state = 0;							// state
	static struct 	st_data data_packet;				// local data packet
	static uint8 	rx_queue[3]; 						// last 3 bytes received
                                                      
	static uint8 	rx_data;   	    				 	// RS485 UART rx data
	static uint8 	rx_data_type;					 	// my id?
	
		
//==========================================================     receive routine

// get data while rx fifo is not empty
	while (UART_RS485_ReadRxStatus() & UART_RS485_RX_STS_FIFO_NOTEMPTY) {
		rx_data = UART_RS485_GetChar();				
		switch (state){
///////////////////////////   wait for frame start   ///////////////////////////			
			case 0:	

				rx_queue[0] = rx_queue[1];
				rx_queue[1] = rx_queue[2];
				rx_queue[2] = rx_data;					
						
				if((rx_queue[1] == ':') &&
				(rx_queue[2] == ':')){				
					rx_queue[0] = 0;
					rx_queue[1] = 0;
					rx_queue[2] = 0;
					state       = 1;
				}			
				else if(
				(rx_queue[0] == 63) &&		//ASCII - ?
				(rx_queue[1] == 13) &&		//ASCII - CR
				(rx_queue[2] == 10)){		//ASCII - LF
					infoSend();
				}				
				break;

///////////////////////////////   wait for id   ////////////////////////////////
			case  1:

				// packet is for my ID or is broadcast
				if(rx_data == c_mem.id || rx_data == 0) {
					rx_data_type = 0;
				} else {				//packet is for others
					rx_data_type = 1;
				}
				data_packet.length = -1;	
				state = 2;			
				break; 
			
//////////////////////////////   wait for length   /////////////////////////////
			case  2:
			
				data_packet.length = rx_data;				
				// check validity of pack length
				if(data_packet.length <= 1) {
					data_packet.length = -1;
					state = 0;			
				} else if(data_packet.length > 128) {
					data_packet.length = -1;
					state = 0;
				} else {
					data_packet.ind = 0;
					if(rx_data_type == 0) {
						state = 3;			// packet for me or boradcast
					} else {
						state = 4;			// packet for others
					}
				}
			break;
			
/////////////////////////////////   receving   /////////////////////////////////
			case 3:
			
			data_packet.buffer[data_packet.ind] = rx_data;
			data_packet.ind++;
			// check end of transmission				
			if(data_packet.ind >= data_packet.length){
				// verify if frame ID corresponded to the device ID
				if(rx_data_type == 0){
					// copying data from buffer to global packet
					memcpy(g_rx.buffer, data_packet.buffer, data_packet.length);
					g_rx.length = data_packet.length;
					g_rx.ready  = 1;	
					commProcess();
				}
				data_packet.ind    = 0;
				data_packet.length = -1;	
				state              = 0;				
			}
			break;

/////////////////////////   other device is receving    ////////////////////////
			case 4:
				if(!(--data_packet.length)) {
					data_packet.ind    = 0;
					data_packet.length = -1;
                    RS485_CTS_Write(1);
                    RS485_CTS_Write(0);    
					state              = 0;				
				}
			break;			
		}		
	}
	/* PSoC3 ES1, ES2 RTC ISR PATCH  */ 
	#if(CYDEV_CHIP_FAMILY_USED == CYDEV_CHIP_FAMILY_PSOC3)
	    #if((CYDEV_CHIP_REVISION_USED <= CYDEV_CHIP_REVISION_3A_ES2) && (ISR_RS485_RX__ES2_PATCH ))      
	        ISR_MOTORS_CONTROL_ISR_PATCH();
	    #endif
	#endif
}

//==============================================================================
//                                                      MOTORS CONTROL INTERRUPT
//==============================================================================
// Motors control
//==============================================================================

void control_and_encoder() {

	// -----   encoder variables   -----
	int i;              //iterator


	static int32 cuff_k_p      = 	-0.001 * 65536;
    static int32 cuff_k_i      =    0 * 65536;
    static int32 cuff_k_d      =    -0.002 * 65536;

	int32 data_encoder[NUM_OF_SENSORS];
	int32 value_encoder[NUM_OF_SENSORS];
	int32 aux;

	static int32 last_value_encoder[NUM_OF_SENSORS];


	// -----   control variables   -----
	static int32 input_1 = 0;
	static int32 input_2 = 0;

	
	static int32 pos_prec_1, pos_prec_2;
	int32 error_1, error_2;

	static int32 err_sum_1, err_sum_2;

	//emg threshold
	static int threshold = 20;

//==============================================================     calibration

	if (calib.enabled) {
		calibration_increment();
	}
	

//==========================================================     reading sensors


	for (i = 0; i < NUM_OF_SENSORS; i++) {
		switch(i) {
			case 0: {
				data_encoder[i] = SHIFTREG_ENC_1_ReadData();
				break;
			}
			case 1: {
				data_encoder[i] = SHIFTREG_ENC_2_ReadData();
				break;
			}
			case 2: {
				data_encoder[i] = SHIFTREG_ENC_3_ReadData();
				break;
			}
		}
		aux = data_encoder[i] & 262142;
		if ((data_encoder[i] & 0x01 ) == BITChecksum(aux))
		{
			aux = data_encoder[i] & 0x3FFC0;			// reset last 6 bit
			value_encoder[i] = (aux - 0x20000) >> 2;	// subtract half of max value
													// and shift to have 16 bit val

			value_encoder[i]  = (int16)(value_encoder[i] + g_mem.m_off[i]);

			// take care of rotations
			aux = value_encoder[i] - last_value_encoder[i];					
			if (aux > 32768)
				g_meas.rot[i]--;
			if (aux < -32768)
			 	g_meas.rot[i]++;	

			last_value_encoder[i] = value_encoder[i];	
			
	        value_encoder[i] += g_meas.rot[i] * 65536;			                 
	        //value_encoder[i] += g_mem.m_off[i];
	        value_encoder[i] *= c_mem.m_mult[i];
		}
		
	    g_meas.pos[i] = value_encoder[i];

	}

	//================= control part

	switch(c_mem.mode) {

		case INPUT_MODE_ENCODER3:
			//--- speed control in both directions ---//

	    	// motor 1
	       	if (((g_meas.pos[2] - g_ref.pos[0]) > c_mem.max_step_pos)   &&   (c_mem.max_step_pos != 0)) {
				g_ref.pos[0] += c_mem.max_step_pos;
	       	} else if (((g_meas.pos[2] - g_ref.pos[0]) < c_mem.max_step_neg)   &&   (c_mem.max_step_neg != 0)) {
				g_ref.pos[0] += c_mem.max_step_neg;
		    } else {
	       		g_ref.pos[0] = g_meas.pos[2];
		    }

		    // motor 2
		  //   if (((g_meas.pos[2] - g_ref.pos[1]) > c_mem.max_step_pos)   &&   (c_mem.max_step_pos != 0)) {
				// g_ref.pos[1] += c_mem.max_step_pos;
	   //     	} else if (((g_meas.pos[2] - g_ref.pos[1]) < c_mem.max_step_neg)   &&   (c_mem.max_step_neg != 0)) {
				// g_ref.pos[1] += c_mem.max_step_neg;
		  //   } else {
	   //     		g_ref.pos[1] = g_meas.pos[2];
		  //   }

		    if (tau_feedback < 0) {
		    	g_ref.pos[1] = 0;	
		    } else if ((tau_feedback * (50 << 8)) > c_mem.pos_lim_sup[1]) {
		    	g_ref.pos[1] = c_mem.pos_lim_sup[1];
		    } else {
		    	g_ref.pos[1] = (tau_feedback * (50 << 8));
		    }
		    

		    break;

		case INPUT_MODE_EMG_PROPORTIONAL:
			if (g_meas.emg[0] > threshold) {
	    	g_ref.pos[0] = ((g_meas.emg[0] - threshold) * dx_sx_hand * closed_hand_pos) / 1024;
		    } else {
		    	g_ref.pos[0] = 0;
		    }

		    break;

		case INPUT_MODE_EMG_INTEGRAL:
			if (g_meas.emg[0] > threshold) {
		    	g_ref.pos[0] += ((g_meas.emg[0] - threshold) * dx_sx_hand * closed_hand_pos) / 500000;
		    }
		    if (g_meas.emg[1] > threshold) {
		    	g_ref.pos[0] -= ((g_meas.emg[1] - threshold) * dx_sx_hand * closed_hand_pos) / 500000;
		    }
			break;

		default:
			break;
	}

	// position limit
    if (c_mem.pos_lim_flag) {
        if (g_ref.pos[0] < c_mem.pos_lim_inf[0]) g_ref.pos[0] = c_mem.pos_lim_inf[0];
        if (g_ref.pos[1] < c_mem.pos_lim_inf[1]) g_ref.pos[1] = c_mem.pos_lim_inf[1];

        if (g_ref.pos[0] > c_mem.pos_lim_sup[0]) g_ref.pos[0] = c_mem.pos_lim_sup[0];
        if (g_ref.pos[1] > c_mem.pos_lim_sup[1]) g_ref.pos[1] = c_mem.pos_lim_sup[1];
    }

	//////////////////////////////////////////////////////////     CONTROL_ANGLE
	
    #if (CONTROL_MODE == CONTROL_ANGLE)
    	error_1 = g_ref.pos[0] - g_meas.pos[0];
    	error_2 = g_ref.pos[1] - g_meas.pos[1];

    	err_sum_1 += error_1;
    	err_sum_2 += error_2;

    	// anti-windup
    	if (err_sum_1 > ANTI_WINDUP) {
    		err_sum_1 = ANTI_WINDUP;
    	} else if (err_sum_1 < -ANTI_WINDUP) {
    		err_sum_1 = -ANTI_WINDUP;
    	}

    	if (err_sum_2 > ANTI_WINDUP) {
    		err_sum_2 = ANTI_WINDUP;
    	} else if (err_sum_2 < -ANTI_WINDUP) {
    		err_sum_2 = -ANTI_WINDUP;
    	}

    	// Proportional
		input_1 = (int32)(c_mem.k_p * error_1) >> 16;
		input_2 = (int32)(cuff_k_p * error_2) >> 16;

		// Integrative
		input_1 += (int32)(c_mem.k_i * err_sum_1) >> 16;
		input_2 += (int32)(cuff_k_i * err_sum_2) >> 16;

		// Derivative
		input_1 += (int32)(c_mem.k_d * (pos_prec_1 - g_meas.pos[0])) >> 16;
		input_2 += (int32)(cuff_k_d * (pos_prec_2 - g_meas.pos[1])) >> 16;

		// Update measure
		pos_prec_1 = g_meas.pos[0];
		pos_prec_2 = g_meas.pos[1];

    #endif

	////////////////////////////////////////////////////////     CONTROL_CURRENT

	#if (CONTROL_MODE == CONTROL_CURRENT)
		if(g_ref.onoff & 1)
		{
			error_1 = g_ref.pos[0] - g_meas.curr[0];
			error_2 = g_ref.pos[1] - g_meas.curr[1];

			err_sum_1 += error_1;
			err_sum_2 += error_2;

			input_1 += ((c_mem.k_p * (error_1)) / 65536) + err_sum_1;
			input_2 += ((c_mem.k_p * (error_2)) / 65536) + err_sum_2;
		} 
		else
		{
			input_1 = 0;
			input_2 = 0;
		} 
		
	#endif

	////////////////////////////////////////////////////////////     CONTROL_PWM

	#if (CONTROL_MODE == CONTROL_PWM)
		input_1 = g_ref.pos[0];
		input_2 = g_ref.pos[1];
	#endif

	////////////////////////////////////////////////////////////////////////////



	if (input_1 > 0) {
		input_1 += PWM_DEAD;
	} else if (input_1 < 0) {
		input_1 -= PWM_DEAD;
	}

		
    if(input_1 >  PWM_LIMIT) input_1 =  PWM_LIMIT;
    if(input_2 >  PWM_LIMIT) input_2 =  PWM_LIMIT;
    if(input_1 < -PWM_LIMIT) input_1 = -PWM_LIMIT;
    if(input_2 < -PWM_LIMIT) input_2 = -PWM_LIMIT;

	MOTOR_DIR_Write((input_1 >= 0) + ((input_2 >= 0) << 1));
	PWM_MOTORS_WriteCompare1(abs(input_1));
	PWM_MOTORS_WriteCompare2(abs(input_2));

}

//==============================================================================
//                                                        MEASUREMENTS INTERRUPT
//==============================================================================
// TODO: DESCRIPTION
//==============================================================================

CY_ISR(ISR_MEASUREMENTS_ExInterrupt)
{
	static uint8 ind;
	int32 value;
	static int sign_1 = 1;
	static int sign_2 = 1;
	static uint16 i_counter = SAMPLES_FOR_MEAN; // Used to perform calibration over
								// the first counter values of current
	static uint16 emg_counter = 0;
	static int32 i_mean_value_1;
	static int32 i_mean_value_2;

	static int32 emg_mean_value_1;
	static int32 emg_mean_value_2;


	float f_aux;
	int32 i_aux;

	static uint8 control_and_encoder_int = 10;
	static uint8 feedback_hand_conrol_int = 25;

	if (control_and_encoder_int == 5) {
		control_and_encoder();
		control_and_encoder_int = 1;
	}

	if (feedback_hand_conrol_int == 25) {
		torque_feedback();
		feedback_hand_conrol_int = 1;
	}

	control_and_encoder_int++;
	feedback_hand_conrol_int++;

	
	ADC_StartConvert();

	if (ADC_IsEndConversion(ADC_WAIT_FOR_RESULT)) {

		value = (int32) ADC_GetResult16();
		ind = AMUXSEQ_MOTORS_GetChannel();

		ADC_StopConvert();

		switch(ind){
			// --- Input tension ---
			case 0:
				device.tension = (value - 1638) * device.tension_conv_factor;
				//until there is no valid input tension repeat this measurement
				if (device.tension < 0) {
					AMUXSEQ_MOTORS_Stop();
					emg_counter = 0;
				}

				break;

			// --- Current motor 1 ---
            case 1:
            	if (g_ref.onoff == 0x03) {
            		if (i_counter > 0) {
	            		i_mean_value_1 += value;
	            		if (i_counter == 1) {
	            			i_mean_value_1 = i_mean_value_1 / SAMPLES_FOR_MEAN;
	            		}
	            	} else {
	            		g_meas.curr[0] =  filter_i1(abs(((value - i_mean_value_1) * 5000) / i_mean_value_1));
	            	}
            	} else {
            		i_counter = SAMPLES_FOR_MEAN;
            		i_mean_value_1 = 0;
					i_mean_value_2 = 0;
					g_meas.curr[0] = 0;
            	}
				break;

			// --- Current motor 2 ---
            case 2:
            	if (g_ref.onoff == 0x03) {
	            	if (i_counter > 0) {
	            		i_mean_value_2 += value;
	            		if (i_counter == 1) {
	            			i_mean_value_2 = i_mean_value_2 / SAMPLES_FOR_MEAN;
	            		}
	            		i_counter--;
	            	} else {	
						g_meas.curr[1] =  filter_i2(abs(((value - i_mean_value_2) * 5000) / i_mean_value_2));
					}
				} else {
					g_meas.curr[1] = 0;	
				}
            	break;

            // --- EMG 1 ---
            case 3:
            	if (emg_counter > SAMPLES_FOR_EMG_MEAN) {
            		// normal execution
            		// f_aux = ((float)value * (5000.0 / 4096.0));
	            	i_aux = filter_ch1(value);
	            	i_aux = (1024 * i_aux) / emg_mean_value_1;
	            	if (i_aux < 0) {
	            		i_aux = 0;
	            	} else if (i_aux > 1024) {
	            		i_aux = 1024;
	            	}
	            	g_meas.emg[0] = i_aux;

            	} else if (emg_counter < 500) {
            		// do nothing, just to discard the first values
            		emg_counter++;
            	} else if (emg_counter < SAMPLES_FOR_EMG_MEAN) {
            		// sum all the values to calculate a max mean value
            		// f_aux = ((float)value * (5000.0 / 4096.0));
	            	emg_mean_value_1 += filter_ch1(value);
	            	LED_REG_Write(0x01);
	            	emg_counter++;
            	} else if (emg_counter == SAMPLES_FOR_EMG_MEAN) {
            		// we finished the samples for mean
            		emg_mean_value_1 = emg_mean_value_1 / (SAMPLES_FOR_EMG_MEAN - 500);
            		LED_REG_Write(0x00);
            		emg_counter++;
            		if ((c_mem.mode == INPUT_MODE_EMG_PROPORTIONAL) || (c_mem.mode == INPUT_MODE_EMG_INTEGRAL)) {
            			g_ref.onoff = c_mem.activ; 
            			MOTOR_ON_OFF_Write(g_ref.onoff);
            		}
            	}
            	
            	break;

            // --- EMG 2 ---
            case 4:
            	if (emg_counter > SAMPLES_FOR_EMG_MEAN) {
            		// normal execution
            		// f_aux = ((float)value * (5000.0 / 4096.0));
	            	i_aux = filter_ch2(value);
	            	i_aux = (1024 * i_aux) / emg_mean_value_2;
	            	if (i_aux < 0) {
	            		i_aux = 0;
	            	} else if (i_aux > 1024) {
	            		i_aux = 1024;
	            	}
	            	g_meas.emg[1] = i_aux;

            	} else if (emg_counter < 500) {
            		// do nothing, just to discard the first values
            	} else if (emg_counter < SAMPLES_FOR_EMG_MEAN) {
            		// sum all the values to calculate a max mean value
            		// f_aux = ((float)value * (5000.0 / 4096.0));
	            	emg_mean_value_2 += filter_ch2(value);
            	} else if (emg_counter == SAMPLES_FOR_EMG_MEAN) {
            		// we finished the samples for mean
            		emg_mean_value_2 = emg_mean_value_2 / (SAMPLES_FOR_EMG_MEAN - 500);
            	}
            	break;

            default:
            	break;

		}
		AMUXSEQ_MOTORS_Next();		
	}


	#if(CYDEV_CHIP_FAMILY_USED == CYDEV_CHIP_FAMILY_PSOC3)
	    #if((CYDEV_CHIP_REVISION_USED <= CYDEV_CHIP_REVISION_3A_ES2) && (ISR_MEASUREMENTS__ES2_PATCH ))      
	        ISR_MEASUREMENTS_ISR_PATCH();
	    #endif
	#endif
}


//==============================================================================
//                                                        	 CALIBRATE INTERRUPT
//==============================================================================
// TODO: DESCRIPTION
//==============================================================================


CY_ISR(ISR_CALIBRATE_ExInterrupt)
{
	int i = 0;
	int16 mean_curr_1, mean_curr_2;

	// save old PID values
	int32 old_k_p = c_mem.k_p;
    int32 old_k_i = c_mem.k_i;
    int32 old_k_d = c_mem.k_d;

	// goto to zero position
	g_ref.pos[0] = 0;
	g_ref.pos[1] = 0;

	// Activate motors
	if (!(g_ref.onoff & 0x03)) {
		MOTOR_ON_OFF_Write(0x03);	
	}
	
	// wait for motors to reach zero position
	ms_delay(1000);

	// set new temp values for PID parameters
    c_mem.k_p = 0.1 * 65536;
    c_mem.k_i = 0;
	c_mem.k_d = 0.3 * 65536;

	// increase stiffness until one of the two motors reach the threshold
	while((mean_curr_1 < MAX_CURRENT) && (abs(mean_curr_2) < MAX_CURRENT)) {
		// increment of 0.5 degree
		g_ref.pos[0] += 65536 / 720;
		g_ref.pos[1] -= 65536 / 720;

		ms_delay(100);

		// Current measurement
		mean_curr_1 = 0;
		mean_curr_2 = 0;
		for (i = 0; i < 10; i++) {
			mean_curr_1 += g_meas.curr[0];
			mean_curr_2 += g_meas.curr[1];
			ms_delay(10);
		}
		mean_curr_1 = mean_curr_1 / 10;
		mean_curr_2 = mean_curr_2 / 10;
	}

	// save current value as MAX_STIFFNESS
	g_mem.max_stiffness = g_ref.pos[0];

	// reset old values for PID parameters
	c_mem.k_p = old_k_p;
    c_mem.k_i = old_k_i;
	c_mem.k_d = old_k_d;

	// go back to zero position
	g_ref.pos[0] = 0;
	g_ref.pos[1] = 0;

	// wait for motors to reach zero position
	ms_delay(3000);

	//Deactivate motors
	// if (!(g_ref.onoff & 0x03)) {
	// 	MOTOR_ON_OFF_Write(0x00);	
	// }

	// store memory to save MAX_STIFFNESS as default value
	memStore(DEFAULT_EEPROM_DISPLACEMENT);
	memStore(0);

/* PSoC3 ES1, ES2 RTC ISR PATCH  */ 
#if(CYDEV_CHIP_FAMILY_USED == CYDEV_CHIP_FAMILY_PSOC3)
    #if((CYDEV_CHIP_REVISION_USED <= CYDEV_CHIP_REVISION_3A_ES2) && (ISR_CALIBRATE__ES2_PATCH ))      
        ISR_CALIBRATE_ISR_PATCH();
    #endif
#endif

}



/* [] END OF FILE */
