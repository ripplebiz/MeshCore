/**
 * @file environment.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialization and reading of BME680 sensor
 * @version 0.1
 * @date 2021-08-21
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <Arduino.h>
#include "simple_sensor.h"

Adafruit_BME680 bme;

bool init_bme(void)
{
	if (!bme.begin(0x76, false))
	{
		Serial.println("Could not find a valid BME680 sensor, check wiring!");
		return false;
	}
	// Set up oversampling and filter initialization
	bme.setTemperatureOversampling(BME680_OS_8X);
	bme.setHumidityOversampling(BME680_OS_2X);
	bme.setPressureOversampling(BME680_OS_4X);
	bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
	bme.setGasHeater(320, 150); // 320*C for 150 ms

	return true;
}

bool read_bme(void)
{
	bme.beginReading();
	time_t wait_start = millis();
	bool read_success = false;
	while ((millis() - wait_start) < 5000)
	{
		if (bme.endReading())
		{
			read_success = true;
			break;
		}
	}

	if (!read_success)
	{
		return false;
	}

	// Manual simple payload creation
	// int16_t temp_int = (int16_t)(bme.temperature * 10.0);
	// uint16_t humid_int = (uint16_t)(bme.humidity * 2);
	// uint16_t press_int = (uint16_t)(bme.pressure / 10);
	// uint16_t gasres_int = (uint16_t)(bme.gas_resistance / 10);

	// g_sensor_payload[0] = (uint8_t)(humid_int);
	// g_sensor_payload[1] = (uint8_t)(temp_int >> 8);
	// g_sensor_payload[2] = (uint8_t)(temp_int);
	// g_sensor_payload[3] = (uint8_t)(press_int >> 8);
	// g_sensor_payload[4] = (uint8_t)(press_int);
	// g_sensor_payload[5] = (uint8_t)(gasres_int >> 8);
	// g_sensor_payload[6] = (uint8_t)(gasres_int);
	// g_sensor_payload_len = 7;

	// Using Cayenne LPP format for the payload
	g_solution_data.addRelativeHumidity(LPP_CHANNEL_HUMID_2, (float)bme.humidity);
	g_solution_data.addTemperature(LPP_CHANNEL_TEMP_2, (float)bme.temperature);
	g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS_2, (float)(bme.pressure) / 100.0);
	return true;
}