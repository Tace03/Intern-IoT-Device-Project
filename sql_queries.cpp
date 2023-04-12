//////////////////////////////////////////////////////////////////////////////////
// Author: Naina Gupta
// July'2021 - ModbusTCPLogger v1.0
//////////////////////////////////////////////////////////////////////////////////

#include "sql_queries.h"

using namespace std;

int rc;
const char *sql;

int rowCount = 0;
int recordCount = 0;
// Save any error messages
char *zErrMsg = 0;

error_data error;

int sqlite_bind_double_status(sqlite3_stmt* stmt, int location, double sensor_value)
{
    if (sqlite3_bind_double(stmt, location, sensor_value) != SQLITE_OK) 
    { 
        printf("SQL   : Could not bind value[%f]\n", sensor_value);  
        return (-1);
    } 
    return 0;
}

int sqlite_bind_int_status(sqlite3_stmt* stmt, int location, int sensor_value)
{
    if (sqlite3_bind_int(stmt, location, sensor_value) != SQLITE_OK) 
    { 
        printf("SQL   : Could not bind value[%d]\n", sensor_value);  
        return (-1);
    } 
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// Section 12
error_data logValues(sqlite3 *db, sqlite3_stmt* stmt, sensor_data* current_sensor_data, int markToSent) 
{	
    error.lineNo = -1;
    error.rc = -1;
    unsigned char loc_count = 1;
    //modified on 11 Aug 21
/*
    sql = "INSERT INTO dataLog(uptime,timeStamp,primary_charging_relay,primary_discharge_relay,primary_positive_pump,primary_negative_pump,balancing_valve,positive_valve,+
        "negative_valve," +
        "state_of_charge," +
        "bcu_voltage," +
        "bcu_current," +
        "bcu_power," +
        "bcu_state_of_charge," +
        "smoke_sensor," +
        "bcu_ocv," +
        "bcu_positive_tank_temp," +
        "bcu_negative_tank_temp," +
        "positive_tank_high_level_float," +
        "negative_tank_high_level_float," +
        "positive_tank_low_level_float," +
        "negative_tank_low_level_float," +
        "primary_stack_voltage," +
        "primary_stack_current," +
        "primary_stack_positive_pressure_sensor," +
        "primary_stack_negative_pressure_sensor," +
        "primary_stack_pressure_delta," +
        "sensor_temp," +
        "humidity," +
        "pcs1_voltage," +
        "pcs1_current," +
        "pcs1_reactive_power," +
        "pcs1_load_power," +
        "pcs1_ac_supply_power," +
        "sentToServer," +
        "markToSent," +
        "system_mode," +
        "system_alarm_status," +
        "bcu_mode_status," +
        "bcu_hydrogen_sensor," +
        "bcu_leakage_sensor," +
        "b1_primary_stack_pressure_delta," +
        "pcs1_dc_volts," +
        "pcs1_dc_batt_current," +
        "pcs1_dc_inverter_power," +
        "pcs1_ac_out_status," +
        "pcs1_fault_status," +
        "pcs1_fan_speed," +
        "system0PVEnable," +
        "system0PVChargePower," +
        "system0PVTotalPower," +
        "pcs1InvFreq," +
        "pcs1InternalTemperature," +
        "bess1_voltage_gain," +
        "bess1_voltage_int," +
        "bess2_voltage_gain," +
        "bess2_voltage_int)" + 
        "VALUES" + 
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
*/
    sql = "INSERT INTO dataLog(uptime,timeStamp,primary_charging_relay,primary_discharge_relay,primary_positive_pump,primary_negative_pump,balancing_valve,positive_valve,negative_valve,state_of_charge,bcu_voltage,bcu_current,bcu_power,bcu_state_of_charge,smoke_sensor,bcu_ocv,bcu_positive_tank_temp,bcu_negative_tank_temp,positive_tank_high_level_float,negative_tank_high_level_float,positive_tank_low_level_float,negative_tank_low_level_float,primary_stack_voltage,primary_stack_current,primary_stack_positive_pressure_sensor,primary_stack_negative_pressure_sensor,primary_stack_pressure_delta,sensor_temp,humidity,pcs1_voltage,pcs1_current,pcs1_reactive_power,pcs1_load_power,pcs1_ac_supply_power,sentToServer,markToSent,system_mode,system_alarm_status,bcu_mode_status,bcu_hydrogen_sensor,bcu_leakage_sensor,b1_primary_stack_pressure_delta,pcs1_dc_volts,pcs1_dc_batt_current,pcs1_dc_inverter_power,pcs1_ac_out_status,pcs1_fault_status,pcs1_fan_speed,system0PVEnable,system0PVChargePower,system0PVTotalPower,pcs1InvFreq,pcs1InternalTemperature) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    rc = sqlite3_prepare_v2(db, sql, -1,  &stmt,  0);
// End of Section 12
//////////////////////////////////////////////////////////////////////////////////
    if ( rc != SQLITE_OK) 
    { 
        printf("SQLLITEOK:%d \r\n", SQLITE_OK);
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
//////////////////////////////////////////////////////////////////////////////////
// Section 13
    rc = sqlite3_bind_double(stmt, loc_count, current_sensor_data->uptime);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;

    rc = sqlite3_bind_int(stmt,loc_count, current_sensor_data->timestamp);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.primary_charging_relay); 
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;	
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.primary_discharge_relay);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.primary_positive_pump);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.primary_negative_pump);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.balancing_valve);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.positive_valve);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.negative_valve);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.state_of_charge);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_voltage);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_current);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_power);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_state_of_charge);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.smoke_sensor);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_ocv);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_positive_tank_temp);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_negative_tank_temp);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.positive_tank_high_level_float);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.negative_tank_high_level_float);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.positive_tank_low_level_float);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.negative_tank_low_level_float);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.primary_stack_voltage);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.primary_stack_current);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.primary_stack_positive_pressure_sensor);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.primary_stack_negative_pressure_sensor);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.positive_stack_pressure_delta);// Modified on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.sensor_temp);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.humidity);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_voltage);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_current);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_reactive_power);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_load_power);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_ac_supply_power);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite3_bind_int(stmt, loc_count, 0);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite3_bind_int(stmt, loc_count, markToSent);
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.system_mode); // Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.system_alarm_status); // Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_mode_status);// Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_hydrogen_sensor);// Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.bcu_leakage_sensor);// Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.b1_primary_stack_pressure_delta); // Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_dc_volts); // Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_dc_batt_current); // Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_dc_inverter_power); // Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_ac_out_status);// Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_fault_status);// Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1_fan_speed);// Added on 11 Aug 21
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_int_status(stmt, loc_count, current_sensor_data->battery.system0PVEnable);// Added 06-12-2021
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.system0PVChargePower);// Added 06-12-2021
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.system0PVTotalPower);// Added 06-12-2021
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1InvFreq);// Added 06-12-2021
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    loc_count++;
    
    rc = sqlite_bind_double_status(stmt, loc_count, current_sensor_data->battery.pcs1InternalTemperature);// Added 06-12-2021
    if ( rc != SQLITE_OK) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
// End of Section 13
//////////////////////////////////////////////////////////////////////////////////

    rc = sqlite3_step(stmt);
    if ( (rc != SQLITE_DONE)) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }

    rc = sqlite3_finalize(stmt);
    if ( (rc != SQLITE_OK)) 
    { 
        error.lineNo = __LINE__;
        error.rc = rc;
        return error;
    }
    return error;
}

// Create a callback function  
int rowCountCallback(void *NotUsed, int argc, char **argv, char **azColName)
{
    if(strcmp(azColName[0],"COUNT(*)") == 0)
    {        
        rowCount = atoi(argv[0]);
    }
    else
    {
        rowCount = -1;
    }
    return 0;
}

int getRowCount(sqlite3 *db)
{
    sql = "SELECT COUNT(*) FROM 'dataLog' WHERE (sentToServer=0 AND markToSent=1);";

    // Run the SQL (convert the string to a C-String with c_str() )
    rc = sqlite3_exec(db, sql, rowCountCallback, 0, &zErrMsg);

    return rowCount;
}

//////////////////////////////////////////////////////////////////////////////////
// Section 14
// Create a callback function  
int sensorDataCallback(void *sensorDataPtr, int argc, char **argv, char **azColName)
{
    sensor_data* fetched_sensor_data = (sensor_data*) sensorDataPtr;
    int i = 0;

    // int argc: holds the number of results
    // (array) azColName: holds each column returned
    // (array) argv: holds each value

    for(i = 0; i < argc; i++) {


    if(strcmp(azColName[i],"uptime") == 0) //strings are same
        fetched_sensor_data->uptime = atol(argv[i]);
    else if(strcmp(azColName[i],"timeStamp") == 0) 
        fetched_sensor_data->timestamp = atol(argv[i]);
    else if(strcmp(azColName[i],"primary_charging_relay") == 0) 
        fetched_sensor_data->battery.primary_charging_relay = atoi(argv[i]);
    else if(strcmp(azColName[i],"primary_discharge_relay") == 0) 
        fetched_sensor_data->battery.primary_discharge_relay = atoi(argv[i]);
    else if(strcmp(azColName[i],"primary_positive_pump") == 0) 
        fetched_sensor_data->battery.primary_positive_pump = atoi(argv[i]);
    else if(strcmp(azColName[i],"primary_negative_pump") == 0) 
        fetched_sensor_data->battery.primary_negative_pump = atoi(argv[i]);

    else if(strcmp(azColName[i],"system_mode") == 0) // Added on 11 Aug 21
    {
        fetched_sensor_data->battery.system_mode = atoi(argv[i]);
        printf("system_mode %lf\r\n",fetched_sensor_data->battery.system_mode);
    }
    else if(strcmp(azColName[i],"system_alarm_status") == 0) // Added on 11 Aug 21
        fetched_sensor_data->battery.system_alarm_status = atoi(argv[i]);

    else if(strcmp(azColName[i],"balancing_valve") == 0) 
        fetched_sensor_data->battery.balancing_valve = atoi(argv[i]);
    else if(strcmp(azColName[i],"positive_valve") == 0) 
        fetched_sensor_data->battery.positive_valve = atoi(argv[i]);
    else if(strcmp(azColName[i],"negative_valve") == 0) 
        fetched_sensor_data->battery.negative_valve = atoi(argv[i]);

    else if(strcmp(azColName[i],"state_of_charge") == 0) 
        fetched_sensor_data->battery.state_of_charge = atof(argv[i]);

    else if(strcmp(azColName[i],"bcu_mode_status") == 0)  // Added on 11 Aug 21
        fetched_sensor_data->battery.bcu_mode_status = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_hydrogen_sensor") == 0)  // Added on 11 Aug 21
        fetched_sensor_data->battery.bcu_hydrogen_sensor = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_leakage_sensor") == 0)  // Added on 11 Aug 21
        fetched_sensor_data->battery.bcu_leakage_sensor = atof(argv[i]);    

    else if(strcmp(azColName[i],"bcu_voltage") == 0) 
        fetched_sensor_data->battery.bcu_voltage = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_current") == 0) 
        fetched_sensor_data->battery.bcu_current = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_power") == 0) 
        fetched_sensor_data->battery.bcu_power = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_state_of_charge") == 0) 
        fetched_sensor_data->battery.bcu_state_of_charge = atof(argv[i]);
    else if(strcmp(azColName[i],"smoke_sensor") == 0) 
        fetched_sensor_data->battery.smoke_sensor = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_ocv") == 0) 
        fetched_sensor_data->battery.bcu_ocv = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_positive_tank_temp") == 0) 
        fetched_sensor_data->battery.bcu_positive_tank_temp = atof(argv[i]);
    else if(strcmp(azColName[i],"bcu_negative_tank_temp") == 0) 
        fetched_sensor_data->battery.bcu_negative_tank_temp = atof(argv[i]);
    else if(strcmp(azColName[i],"positive_tank_high_level_float") == 0) 
        fetched_sensor_data->battery.positive_tank_high_level_float = atoi(argv[i]);
    else if(strcmp(azColName[i],"negative_tank_high_level_float") == 0) 
        fetched_sensor_data->battery.negative_tank_high_level_float = atoi(argv[i]);
    else if(strcmp(azColName[i],"positive_tank_low_level_float") == 0) 
        fetched_sensor_data->battery.positive_tank_low_level_float = atoi(argv[i]);
    else if(strcmp(azColName[i],"negative_tank_low_level_float") == 0) 
        fetched_sensor_data->battery.negative_tank_low_level_float = atoi(argv[i]);
    else if(strcmp(azColName[i],"primary_stack_voltage") == 0) 
        fetched_sensor_data->battery.primary_stack_voltage = atof(argv[i]);
    else if(strcmp(azColName[i],"primary_stack_current") == 0) 
        fetched_sensor_data->battery.primary_stack_current = atof(argv[i]);
    else if(strcmp(azColName[i],"primary_stack_positive_pressure_sensor") == 0) 
        fetched_sensor_data->battery.primary_stack_positive_pressure_sensor = atof(argv[i]);
    else if(strcmp(azColName[i],"primary_stack_negative_pressure_sensor") == 0) 
        fetched_sensor_data->battery.primary_stack_negative_pressure_sensor = atof(argv[i]);

    else if(strcmp(azColName[i],"positive_stack_pressure_delta") == 0)   // Modified on 11 Aug 21
        fetched_sensor_data->battery.positive_stack_pressure_delta = atof(argv[i]);
    else if(strcmp(azColName[i],"primary_stack_pressure_delta") == 0)   // Added on 11 Aug 21
        fetched_sensor_data->battery.b1_primary_stack_pressure_delta = atof(argv[i]);

    else if(strcmp(azColName[i],"sensor_temp") == 0) 
        fetched_sensor_data->battery.sensor_temp = atof(argv[i]);
    else if(strcmp(azColName[i],"humidity") == 0) 
        fetched_sensor_data->battery.humidity = atof(argv[i]);

    else if(strcmp(azColName[i],"pcs1_dc_volts") == 0)    // Added on 11 Aug 21
        fetched_sensor_data->battery.pcs1_dc_volts = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_dc_batt_current") == 0)    // Added on 11 Aug 21
        fetched_sensor_data->battery.pcs1_dc_batt_current = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_dc_inverter_power") == 0)    // Added on 11 Aug 21
        fetched_sensor_data->battery.pcs1_dc_inverter_power = atof(argv[i]);

    else if(strcmp(azColName[i],"pcs1_voltage") == 0) 
        fetched_sensor_data->battery.pcs1_voltage = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_current") == 0) 
        fetched_sensor_data->battery.pcs1_current = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_reactive_power") == 0) 
        fetched_sensor_data->battery.pcs1_reactive_power = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_load_power") == 0) 
        fetched_sensor_data->battery.pcs1_load_power = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_ac_supply_power") == 0) 
        fetched_sensor_data->battery.pcs1_ac_supply_power = atof(argv[i]);

    else if(strcmp(azColName[i],"pcs1_ac_out_status") == 0)    // Added on 11 Aug 21 
        fetched_sensor_data->battery.pcs1_ac_out_status = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_fault_status") == 0)     // Added on 11 Aug 21
        fetched_sensor_data->battery.pcs1_fault_status = atof(argv[i]);
    else if(strcmp(azColName[i],"pcs1_fan_speed") == 0)     // Added on 11 Aug 21
        fetched_sensor_data->battery.pcs1_fan_speed = atof(argv[i]);

    else if(strcmp(azColName[i],"system0PVEnable") == 0)                            // Added 06-12-2021
        fetched_sensor_data->battery.system0PVEnable = atof(argv[i]);               // Added 06-12-2021
    else if(strcmp(azColName[i],"system0PVChargePower") == 0)                       // Added 06-12-2021
        fetched_sensor_data->battery.system0PVChargePower = atof(argv[i]);          // Added 06-12-2021
    else if(strcmp(azColName[i],"system0PVTotalPower") == 0)                        // Added 06-12-2021
        fetched_sensor_data->battery.system0PVTotalPower = atof(argv[i]);           // Added 06-12-2021
    else if(strcmp(azColName[i],"pcs1InvFreq") == 0)                                // Added 06-12-2021
        fetched_sensor_data->battery.pcs1InvFreq = atof(argv[i]);                   // Added 06-12-2021
    else if(strcmp(azColName[i],"pcs1InternalTemperature") == 0)                    // Added 06-12-2021
        fetched_sensor_data->battery.pcs1InternalTemperature = atof(argv[i]);       // Added 06-12-2021

    if(i==argc)
        fetched_sensor_data->isValid = true;
    // Insert a newline
    std::cout << endl;

    // Return successful
    return 0;
}
// End of Section 14
//////////////////////////////////////////////////////////////////////////////////
error_data getSensorDataEntry(sqlite3 *db, sensor_data* sensorData)
{
    sql = "SELECT * FROM 'dataLog' WHERE (sentToServer=0 AND markToSent=1) LIMIT 1;";
    sensorData->isValid = false;

    // Run the SQL (convert the string to a C-String with c_str() )
    rc = sqlite3_exec(db, sql, sensorDataCallback, sensorData, &zErrMsg); //sql.c_str()

    //printf("Query Result %d\r\n",rc);
    if(rc != SQLITE_OK)
    {
        error.lineNo = __LINE__;
        error.rc = rc;
    }
    else
    {   
        error.lineNo = -1;
        error.rc = -1;
    }
    return error;
}

error_data updateSensorDataEntry(sqlite3 *db)
{
    sql = "UPDATE 'dataLog' SET sentToServer=1 WHERE (sentToServer=0 AND markToSent=1) LIMIT 1;";

    // Run the SQL (convert the string to a C-String with c_str() )
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if(rc != SQLITE_OK)
    {
        error.lineNo = __LINE__;
        error.rc = rc;
    }
    else
    {   
        error.lineNo = -1;
        error.rc = -1;
    }
    return error;
}


int logErrors(sqlite3 *db, sqlite3_stmt* stmt, long timestamp, char *errorStr, int size) 
{
    sql = "INSERT INTO errorLog (timeStamp,event) VALUES (?,?);";
    if ( sqlite3_prepare_v2(db, sql, -1,  &stmt,  0) != SQLITE_OK) 
    { 
        printf("SQL   : Could not prepare statement - Log Data\n"); 
        return (-1);
    }

    if (sqlite3_bind_int(stmt,1, timestamp) != SQLITE_OK) 
    { 
        printf("SQL   : Could not bind timeStamp[%ld]\n", timestamp);  
        return (-1);
    }	

    int rc1 = sqlite3_bind_text(stmt,2, errorStr, size, SQLITE_STATIC);  //size

    if ( rc1 != SQLITE_OK) 
    { 
        printf("SQL   : Could not Bind errorStr [%d] [%s]\n", rc1, errorStr);  
        return (-1);
    }

    rc = sqlite3_step(stmt);
    printf("Statement Run\r\n");
    sqlite3_finalize(stmt);

    return 0;
}


// Create a callback function  
int recordCountCallback(void *NotUsed, int argc, char **argv, char **azColName)
{
    if(strcmp(azColName[0],"COUNT(*)") == 0)
    {        
        recordCount = atoi(argv[0]);
    }
    else
    {
        recordCount = -1;
    }
    return 0;
}

int getRecordCount(sqlite3 *db)
{
    sql = "SELECT COUNT(*) FROM 'dataLog';";

    rc = sqlite3_exec(db, sql, recordCountCallback, 0, &zErrMsg);

    return recordCount;
}

int deleteRecordCount;

int getDeleteRecordCount(sqlite3 *db, sqlite3_stmt* stmt, long timeDel)
{
    sql = "SELECT COUNT(*) FROM 'dataLog' where timeStamp < (?);";

    if ( sqlite3_prepare_v2(db, sql, -1,  &stmt,  0) != SQLITE_OK) 
    { 
        printf("SQL   : Could not prepare statement - Get Delete Record Count\n"); 
        return (-1);
    }

    if (sqlite3_bind_int(stmt,1, timeDel) != SQLITE_OK) 
    { 
        printf("SQL   : Could not bind timeStamp[%ld]\n", timeDel);  
        return (-1);
    }	

    // printf("SQL   : Statement Ready...");
    rc = sqlite3_step(stmt);

    deleteRecordCount = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return deleteRecordCount;
}


long recent_timestamp;

int recentTimestampCallback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i = 0;

    for(i = 0; i < argc; i++) 
    {
        if(strcmp(azColName[i],"timeStamp") == 0) 
        {
            recent_timestamp = atol(argv[i]);
        } 
        else
        {   
            recent_timestamp = -1;
        }
    }

    // Insert a newline
    std::cout << endl;
    // Return successful
    return 0;
}

long getRecentTimestamp(sqlite3 *db)
{
    sql = "SELECT timeStamp FROM 'dataLog' order by timeStamp DESC LIMIT 1;";
    // Run the SQL (convert the string to a C-String with c_str() )
    rc = sqlite3_exec(db, sql, recentTimestampCallback, 0, &zErrMsg); //sql.c_str()
    printf("Query Result %d\r\n",rc);
    return recent_timestamp;
}

error_data deleteSensorDataEntries(sqlite3 *db, sqlite3_stmt* stmt, long timeDel)
{
    sql = "delete from 'dataLog' where timeStamp < (?);";

    if ( sqlite3_prepare_v2(db, sql, -1,  &stmt,  0) != SQLITE_OK) 
    { 
        printf("SQL   : Could not prepare statement - Delete Data\n"); 
        //return (-1);
    }

    if (sqlite3_bind_int(stmt,1, timeDel) != SQLITE_OK) 
    { 
        printf("SQL   : Could not bind timeStamp[%ld]\n", timeDel);  
        //return (-1);
    }

    rc = sqlite3_step(stmt);
    rc = sqlite3_finalize(stmt);

    if(rc != SQLITE_OK)
    {
        error.lineNo = __LINE__;
        error.rc = rc;
    }
    else
    {   
        error.lineNo = -1;
        error.rc = -1;
    }
    return error;
}

error_data executeVacuum(sqlite3 *db)
{
    // Run the SQL (convert the string to a C-String with c_str() )

    rc = sqlite3_exec(db, "VACUUM", 0, 0, 0);

    if(rc != SQLITE_OK)
    {
        error.lineNo = __LINE__;
        error.rc = rc;
    }
    else
    {   
        error.lineNo = -1;
        error.rc = -1;
    }
    printf("Vacuum Query Result %d\r\n",rc);
    return error;
}
