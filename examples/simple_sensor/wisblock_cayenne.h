/**
 * @file wisblock_cayenne.h
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Extend CayenneLPP class with custom channels
 * @version 0.1
 * @date 2022-01-29
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef WISBLOCK_CAYENNE_H
#define WISBLOCK_CAYENNE_H

#include <Arduino.h>
// #include <ArduinoJson.h>
#include <CayenneLPP.h>

#define LPP_GPS4 136 // 3 byte lon/lat 0.0001 °, 3 bytes alt 0.01 meter (Cayenne LPP default)
#define LPP_GPS6 137 // 4 byte lon/lat 0.000001 °, 3 bytes alt 0.01 meter (Customized Cayenne LPP)
#define LPP_VOC 138	 // 2 byte VOC index
#define LPP_MESHCORE_ID 255 // 6 bytes MeshCore pub key

// Only Data Size
#define LPP_GPS4_SIZE 9
#define LPP_GPS6_SIZE 11
#define LPP_GPSH_SIZE 14
#define LPP_GPST_SIZE 10
#define LPP_VOC_SIZE 2
#define LPP_MESHCORE_ID_SIZE 6

class WisCayenne : public CayenneLPP
{
public:
	WisCayenne(uint8_t size) : CayenneLPP(size) {}

	uint8_t addGNSS_4(uint8_t channel, int32_t latitude, int32_t longitude, int32_t altitude);
	uint8_t addGNSS_6(uint8_t channel, int32_t latitude, int32_t longitude, int32_t altitude);
	uint8_t addGNSS_H(int32_t latitude, int32_t longitude, int16_t altitude, int16_t accuracy, int16_t battery);
	uint8_t addGNSS_T(int32_t latitude, int32_t longitude, int16_t altitude, float accuracy, int8_t sats);
	uint8_t addVoc_index(uint8_t channel, uint32_t voc_index);
	uint8_t addMeshCoreID(uint8_t channel, uint8_t *dev_id);

private:
};
#endif