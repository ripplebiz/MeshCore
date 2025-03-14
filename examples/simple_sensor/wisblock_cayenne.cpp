/**
 * @file wisblock_cayenne.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Extend CayenneLPP class with custom channels
 * @version 0.1
 * @date 2022-01-29
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "wisblock_cayenne.h"

/** uint16_t level union */
union int_union_s
{
	uint16_t val16 = 0;
	uint8_t val8[2];
};
/** Latitude/Longitude value union */
union latLong_s
{
	int32_t val32;
	int8_t val8[4];
};

/**
 * @brief Add GNSS data in Cayenne LPP standard format
 *
 * @param channel LPP channel
 * @param latitude Latitude as read from the GNSS receiver
 * @param longitude Longitude as read from the GNSS receiver
 * @param altitude Altitude as read from the GNSS receiver
 * @return uint8_t bytes added to the data packet
 */
uint8_t WisCayenne::addGNSS_4(uint8_t channel, int32_t latitude, int32_t longitude, int32_t altitude)
{
	// check buffer overflow
	if ((_cursor + LPP_GPS4_SIZE + 2) > _maxsize)
	{
		_error = LPP_ERROR_OVERFLOW;
		return 0;
	}
	_buffer[_cursor++] = channel;
	_buffer[_cursor++] = LPP_GPS4;

	latLong_s pos_union;

	// Save default Cayenne LPP precision
	pos_union.val32 = latitude / 1000; // Cayenne LPP 0.0001 ° Signed MSB
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[0];

	pos_union.val32 = longitude / 1000; // Cayenne LPP 0.0001 ° Signed MSB
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[0];

	pos_union.val32 = altitude / 10; // Cayenne LPP 0.01 meter Signed MSB
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[0];

	return _cursor;
}

/**
 * @brief Add GNSS data in custom Cayenne LPP format
 *        Requires changed decoder in LNS and visualization
 *        Does not work with Cayenne LPP MyDevices
 *
 * @param channel LPP channel
 * @param latitude Latitude as read from the GNSS receiver
 * @param longitude Longitude as read from the GNSS receiver
 * @param altitude Altitude as read from the GNSS receiver
 * @return uint8_t bytes added to the data packet
 */
uint8_t WisCayenne::addGNSS_6(uint8_t channel, int32_t latitude, int32_t longitude, int32_t altitude)
{
	// check buffer overflow
	if ((_cursor + LPP_GPS6_SIZE + 2) > _maxsize)
	{
		_error = LPP_ERROR_OVERFLOW;
		return 0;
	}
	_buffer[_cursor++] = channel;
	_buffer[_cursor++] = LPP_GPS6;

	latLong_s pos_union;

	pos_union.val32 = latitude / 10; // Custom 0.000001 ° Signed MSB
	_buffer[_cursor++] = pos_union.val8[3];
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[0];

	pos_union.val32 = longitude / 10; // Custom 0.000001 ° Signed MSB
	_buffer[_cursor++] = pos_union.val8[3];
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[0];

	pos_union.val32 = altitude / 10; // Cayenne LPP 0.01 meter Signed MSB
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[0];

	return _cursor;
}

/**
 * @brief Add GNSS data in Helium Mapper format
 *
 * @param channel LPP channel
 * @param latitude Latitude as read from the GNSS receiver
 * @param longitude Longitude as read from the GNSS receiver
 * @param altitude Altitude as read from the GNSS receiver
 * @param accuracy Accuracy of reading from the GNSS receiver
 * @param battery Device battery voltage in V
 * @return uint8_t bytes added to the data packet
 */

uint8_t WisCayenne::addGNSS_H(int32_t latitude, int32_t longitude, int16_t altitude, int16_t accuracy, int16_t battery)
{
	// check buffer overflow
	if ((_cursor + LPP_GPSH_SIZE) > _maxsize)
	{
		_error = LPP_ERROR_OVERFLOW;
		return 0;
	}

	latLong_s pos_union;

	pos_union.val32 = latitude / 100; // Custom 0.00001 ° Signed MSB
	_buffer[_cursor++] = pos_union.val8[0];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[3];

	pos_union.val32 = longitude / 100; // Custom 0.00001 ° Signed MSB
	_buffer[_cursor++] = pos_union.val8[0];
	_buffer[_cursor++] = pos_union.val8[1];
	_buffer[_cursor++] = pos_union.val8[2];
	_buffer[_cursor++] = pos_union.val8[3];

	pos_union.val32 = altitude / 1000;
	_buffer[_cursor++] = pos_union.val8[0];
	_buffer[_cursor++] = pos_union.val8[1];

	pos_union.val32 = accuracy;
	_buffer[_cursor++] = pos_union.val8[0];
	_buffer[_cursor++] = pos_union.val8[1];

	int_union_s batt_level;

	batt_level.val16 = battery;
	_buffer[_cursor++] = batt_level.val8[0];
	_buffer[_cursor++] = batt_level.val8[1];

	return _cursor;
}

/**
 * @brief Add GNSS data in Field Tester format
 *
 * @param latitude Latitude as read from the GNSS receiver
 * @param longitude Longitude as read from the GNSS receiver
 * @param altitude Altitude as read from the GNSS receiver
 * @param accuracy Accuracy of reading from the GNSS receiver
 * @param sats Number of satellites of reading from the GNSS receiver
 * @return uint8_t bytes added to the data packet
 */
uint8_t WisCayenne::addGNSS_T(int32_t latitude, int32_t longitude, int16_t altitude, float accuracy, int8_t sats)
{
	// check buffer overflow
	if ((_cursor + LPP_GPST_SIZE) > _maxsize)
	{
		_error = LPP_ERROR_OVERFLOW;
		return 0;
	}

	// latitude = latitude / 1000;
	// longitude = longitude / 1000;
	altitude = altitude / 1000;

	uint64_t t = 0;
	uint64_t l = 0;
	if (longitude < 0)
	{
		t |= 0x800000000000L;
		l = -longitude;
	}
	else
	{
		l = longitude;
	}
	if (l / 10000000 >= 180)
	{
		l = 8372093;
	}
	else
	{
		if (l < 107)
		{
			l = 0;
		}
		else
		{
			l = (l - 107) / 215;
		}
	}
	t |= (l & 0x7FFFFF);

	if (latitude < 0)
	{
		t |= 0x400000000000L;
		l = -latitude;
	}
	else
	{
		l = latitude;
	}
	if (l / 10000000 >= 90)
	{
		l = 8333333;
	}
	else
	{
		if (l < 53)
		{
			l = 0;
		}
		else
		{
			l = (l - 53) / 108;
		}
	}
	t |= (l << 23) & 0x3FFFFF800000;

	// Add the location to the package
	_buffer[_cursor++] = (t >> 40) & 0xFF;
	_buffer[_cursor++] = (t >> 32) & 0xFF;
	_buffer[_cursor++] = (t >> 24) & 0xFF;
	_buffer[_cursor++] = (t >> 16) & 0xFF;
	_buffer[_cursor++] = (t >> 8) & 0xFF;
	_buffer[_cursor++] = (t) & 0xFF;
	_buffer[_cursor++] = ((altitude + 1000) >> 8) & 0xFF;
	_buffer[_cursor++] = ((altitude + 1000)) & 0xFF;
	_buffer[_cursor++] = (uint8_t)(accuracy * 10.0);
	_buffer[_cursor++] = sats;

	return _cursor;
}

/**
 * @brief Add the VOC index
 *
 * @param channel VOC channel
 * @param voc_index VOC index
 * @return uint8_t bytes added to the data packet
 */
uint8_t WisCayenne::addVoc_index(uint8_t channel, uint32_t voc_index)
{
	// check buffer overflow
	if ((_cursor + LPP_VOC_SIZE + 2) > _maxsize)
	{
		_error = LPP_ERROR_OVERFLOW;
		return 0;
	}
	_buffer[_cursor++] = channel;
	_buffer[_cursor++] = LPP_VOC;

	int_union_s voc_union;

	voc_union.val16 = voc_index; // VOC index
	_buffer[_cursor++] = voc_union.val8[1];
	_buffer[_cursor++] = voc_union.val8[0];

	return _cursor;
}

uint8_t WisCayenne::addMeshCoreID(uint8_t channel, uint8_t *dev_id)
{
	// check buffer overflow
	if ((_cursor + LPP_MESHCORE_ID_SIZE + 2) > _maxsize)
	{
		_error = LPP_ERROR_OVERFLOW;
		return 0;
	}
	_buffer[_cursor++] = channel;
	_buffer[_cursor++] = LPP_MESHCORE_ID;

	_buffer[_cursor++] = dev_id[0];
	_buffer[_cursor++] = dev_id[1];
	_buffer[_cursor++] = dev_id[2];
	_buffer[_cursor++] = dev_id[3];
	_buffer[_cursor++] = dev_id[4];
	_buffer[_cursor++] = dev_id[5];

	return _cursor;
}