#ifndef SECRETS_H
#define SECRETS_H

/**
 * @file Secrets.h
 * @brief Device identification parameters.
 * 
 * @note SENSOR_ID constraints:
 * - SENSOR_ID must be 15 characters or fewer.
 * - It is broadcast inside the BLE Scan Response packet alongside the 12-byte telemetry payload.
 * - Legacy BLE Advertising caps scan response packets at 31 bytes total.
 * - 4B AD headers + 12B telemetry payload + N bytes name <= 31B total -> N <= 15 chars max.
 * - SENSOR_ID is set at build time via deploy.sh into Secrets.cpp.
 */
extern const char* SENSOR_ID;

#endif // SECRETS_H