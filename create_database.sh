#!/bin/bash

echo "Creating SENSOR DATABASE"
sqlite3 -echo db_sensordata.db <db_sensordata.sql
sqlite3 -echo db_statusdata.db <db_statusdata.sql