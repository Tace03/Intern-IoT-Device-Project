//////////////////////////////////////////////////////////////////////////////////
// Author: Naina Gupta
// July'2021 - ModbusTCPLogger v1.0
//////////////////////////////////////////////////////////////////////////////////

#ifndef SQL_QUERIES_H
#define SQL_QUERIES_H

#include <sqlite3.h>
#include <string.h>

#include "battery_json.h"

error_data logValues(sqlite3 *db, sqlite3_stmt* stmt, sensor_data* current_sensor_data, int markToSent);
error_data updateSensorDataEntry(sqlite3 *db);
error_data getSensorDataEntry(sqlite3 *db, sensor_data* sensorData);
int getRowCount(sqlite3 *db);

int logErrors(sqlite3 *db, sqlite3_stmt* stmt, long timestamp, char *errorStr, int size);

int getRecordCount(sqlite3 *db);
long getRecentTimestamp(sqlite3 *db);

int getDeleteRecordCount(sqlite3 *db, sqlite3_stmt* stmt, long timeDel);

error_data executeVacuum(sqlite3 *db);
error_data deleteSensorDataEntries(sqlite3 *db, sqlite3_stmt* stmt, long timeDel);

#endif