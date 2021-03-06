/*
 * Servo_control.c
 *
 * Created: 12/7/2017 3:33:13 PM
 *  Author:  Raghunath Jangam
 * Description: Controls sensor driver
 */ 
/********************************************************************************************/
#include <asf.h>
#include "sensor_control.h"
#include "sensor.h"

/********************************************************************************************/
static const  uint8_t sensor_analog_inputs[NO_SENSORS] = SENSORS_AI;

/********************************************************************************************/
#define	  BATTERY_MONITOR_SENSOR_VALUE	3
#define THRESHOLD_NOISE			5
#define THRESHOLD_OUT_RANGE		3

/********************************************************************************************/
//0-- sensor-1 , 1-- sensor-2 , 2-- sensor-3 , 3- Volatge_mtr
uint8_t read_sensor(uint8_t sensor_no)
{
	uint8_t output_single_adc = 0;
	output_single_adc = adc_start_read_result(sensor_analog_inputs[sensor_no]);
	return output_single_adc;
}
/********************************************************************************************
filter_volatge_monitor:
Due to huge fluctuations when the servos are moving, it's essential that we need to filter out
voltage reading. Read only the max values of the filtered values. Check if the value received is 
in limits of the previous  if not add to the overall value, if not ignore the value for 5 times before
adding it to the main value.
********************************************************************************************/
uint8_t filter_volatge_monitor(uint8_t sensor_input)
{
	uint8_t sensor_output = 0;
	static uint8_t sensor_output_inf_filter = 0;
	static uint8_t init = false; 
	static uint8_t out_range = 0;
	if(init == false)
	{
		init = true;
		sensor_output_inf_filter = sensor_input;
	}
	//sensor_output_inf_filter = (9*sensor_output_inf_filter + sensor_input)/10;
	if((sensor_input < (sensor_output_inf_filter + THRESHOLD_NOISE)) && (sensor_input > (sensor_output_inf_filter - THRESHOLD_NOISE)))
	{
		sensor_output_inf_filter = (9*sensor_output_inf_filter + sensor_input)/10;
		out_range = 0;
	}
	else
	{
		out_range++;
		if(out_range == THRESHOLD_OUT_RANGE)
		{
			sensor_output_inf_filter = (9*sensor_output_inf_filter + sensor_input)/10;
		}
	}
	
	sensor_output   = sensor_output_inf_filter;
	return sensor_output;
}


void read_all_sensors()
{
	uint8_t temp_sensor_outputs[4];
	uint32_t output_multiple_adc = 0;
	static uint16_t test_count=0;
	static uint16_t overall_count=0;
	static uint32_t total_sum = 0;
	static uint8_t result = 0;
	static volatile uint8_t sensor_output_temp[250];
	static volatile uint8_t overall_output[250];
	static bool init = false;
	static uint8_t overall_max_value = 0;
	static uint8_t max_value = 0;
	static uint8_t count_max_value = 0;
	
	uint8_t i = 0;
	
	for(i= 0; i< NO_SENSORS; i++)
	{
		temp_sensor_outputs[i] = adc_start_read_result(sensor_analog_inputs[i]);
		if(i==BATTERY_MONITOR_SENSOR_VALUE)
		{
			//look for maximum values to remove the noise
			max_value = filter_volatge_monitor(temp_sensor_outputs[i]);
			if(overall_max_value <= max_value)
			{
				if(overall_max_value == max_value)
				{
					count_max_value++;
				}
				else
				{
					count_max_value = 0;
					overall_max_value = max_value;
				}
			}
			
			if(init == false)
			{
				temp_sensor_outputs[i] = temp_sensor_outputs[i] ;
			}
			else
			{
				temp_sensor_outputs[i] = battery_voltage ;
			}				
		}
		//To make sure firmware version is sent uncorrupted during the start when micro::bit asks for it 
		if(firmware_check == false)
		{
			sensor_outputs[i]      = temp_sensor_outputs[i] ;
		}
		
	}
	sensor_outputs[4] = temp_sensor_outputs[0];
	sensor_outputs[5] = temp_sensor_outputs[1];
	//make sure we have enough values we update the battery value
	test_count++;
	if(test_count == 1000)
	{
		init			=	true ;
		battery_voltage = overall_max_value;
		test_count		=	0;
		overall_max_value = 0;
	}
	
}

