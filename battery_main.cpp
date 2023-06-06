//////////////////////////////////////////////////////////////////////////////////
// Author: Naina Gupta
// July'2021 - ModbusTCPLogger v1.0
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Documentation: Written by SeowSK
//
// 1) Adding Multiple Servers (For data migration):
//      1.1) Add a namespace in "Section 1".
//          - Each namespace represent a server
//      1.2) Declare a new int variable for status checking in "Section 2.1"
//      1.3) Copy "Section 2.2" and "Section 2.3" and paste it on the same indent
//          - Modify all status variable to what you have declared on (1.2)
//
// 2) Removing transmission to MADS server
//      2.1) Remove namespace containing MADS URL in "Section 1"
//      2.2) Remove code block "Duplicated code for MADS" up to "End of duplicated code for MADS"
//          - There are two code block as of 6th April 2023
//
// 3) Adding a modbus connection
//      3.1) Add a modbus_t, port, bool, and noOfAttempts for your new modbus in "Section 3"
//      3.2) Add Sensor configuration for your new modbus in "Section 3"
//      3.3) Add a reconnection function in "Section 4" (Copy the block of function)
//          - Modify your variables to what was declared in (3.1) and (3.2)
//      3.4) Declare all of your registers in a new populateConfiguration function in "Section 5"
//          - (Reason) Reading of sensors is dependent on sensor_config vector size
//      3.5) Add a new function to print your data of the new modbus connection in "Section 6"
//          - This is optional
//          - Adding a new function is not required, but a modbus connection may fail.
//          - Wants to stop the modbus that is not connected from printing.
//      3.6) Add a new probe function in "Section 7"
//          - Modify all variables declared on (3.1)
//          - This function will read from modbus and store in memory (not database)
//      3.7) Add your probe function in "Section 8"
//          - This function will store the read values into database for sending to server
//          - Remember to add checking for connection and vector size for your new modbus
//          - Mainly Section 8.1, 8.2, and 8.3
//      3.8) Add your modbus connection, reconnection, and sensor population in "Section 9"
//      3.9) Add a new double array of your new modbus variables in "Section 10", battery_json.cpp
//          - This section is only used to convert data to JSON before sending to gateway.
//      3.10) Declare the variables of your new modbus connection in "Section 11", battery_json.h
//          - This section is used to setup your <sensor_config> vector
//      3.11) Add variable name to insert and a ? to values in "Section 12", sql_queries.cpp
//          - This will allow the insertion of data after reading from probeSensors.
//      3.12) Add a code chunk from "Section 13", sql_queries.cpp for your newly added variable in your new modbus to "Section 13"
//          - Note that the variable type could be double or int.
//      3.13) Add a fetch for your variable in your new modbus in "Section 14", sql_queries.cpp
//          - Note that the order of your fetch is important.
//          - This is used to retrieve data from the database.
//      3.14) Use a tool to insert columns into the database. You can use DB Browser (SQLite)
//      3.15) Update read config function is "Section 15"
//          - Add modbus IP
//          - Add modbus slave
//      3.16) Update config.json on Rasberry PI
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <modbus.h>
#include <stdlib.h>

#include <string.h>
#include <time.h>
#include <errno.h>
#include <sqlite3.h>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
 
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"
#include "sql_queries.h"

// g++ -o battery_main battery_main.cpp `pkg-config --cflags --libs libmodbus` -pthread -lsqlite3
// sudo apt-get install libsqlite3-dev

using namespace std;
using namespace std::chrono_literals;

json sendDataJSON;
string sendDataString;

system_config sys_config;

//////////////////////////////////////////////////////////////////////////////////
// Section 3
// For modbus communication
modbus_t *mbBMS;
modbus_t *mbBESS;
modbus_t *mbLit;
modbus_t *mbMG;

// modbus port of the communiting device
int mbBMSPort       = 1502;
int mbBESSPort      = 502;
int mbLitPort       = 503;
int mbMGPort        = 503;

// For Checking if Modbus is connected for each device
static bool isBMSModbusConn     = false;
static bool isBESSModbusConn    = false;
static bool isLitModbusConn     = false;
static bool isMGModbusConn      = false;
int noOfBMSModbusAttempts       = 0;
int noOfBESSModbusAttempts      = 0;
int noOfLitModbusAttempts       = 0;
int noOfMGModbusAttempts        = 0;

std::vector<sensor_config*> sensor_configuration;
std::vector<sensor_config*> BESS_configuration;
std::vector<sensor_config*> Lit_configuration;
std::vector<sensor_config*> MG_configuration;

// For checking if all modbus variables are read
uint16_t resCode    = 0;
uint16_t resCode2   = 0;
uint16_t resCode3   = 0;
uint16_t resCode4   = 0;
// End of Section 3
//////////////////////////////////////////////////////////////////////////////////
uint16_t sensor_value[2] ={0};
static bool isInternetConn = false;
int noOfInternetAttempts = 0;
int noOfInternetAttemptsAllowed = 10;

static int markToSent = 0;

battery_data latest_battery_data;
battery_data current_battery_data;

sensor_data actual_sensor_data;
sensor_data db_sensor_data;

bool sendDataFaster = false;

const char * db_data = "/home/adv/Desktop/modbustcplogger/db_sensordata.db";
sqlite3 * db;
sqlite3_stmt *stmt;

const char * db_statusdata = "/home/adv/Desktop/modbustcplogger/db_statusdata.db";
sqlite3 * dbErr;
sqlite3_stmt *stmtErr;
char errorMsg[255];

error_data curr_error;
time_t timeNow;
time_t timeStarted;
struct tm *ts;
char datetime[20];

std::mutex mtx_access_sensor_data_db; // mutex for pushing or popping data from battery data queue
std::mutex mtx_access_error_data_db;
std::mutex mtx_mark_to_sent;
std::mutex mtx_curr_error;
std::mutex mtx_error_msg;

//////////////////////////////////////////////////////////////////////////////////
// Section 1
namespace madsPlt
{
	httplib::Client madsRequest("https://datakrewtech.com");
	std::string mads_auth_token = "n03d1jv2wlfuygsshssygugdugsssgvb";
	std::string mads_url = "/iot/orgs/3/projects/70/gateways/28/data_dump";
}
namespace vftPlt
{
	httplib::Client madsRequest("https://backend.vflowtechiot.com");
}
// End of Section 1
//////////////////////////////////////////////////////////////////////////////////

//Used for Testing
httplib::Client cli("http://192.168.1.174:1234");

#define SLAVE_ID 1

//////////////////////////////////////////////////////////////////////////////////
// Section 4
// For reconnecting to Modbus, can ignore after settings are properly set.
void reconnectToBMSModbus()
{
    if(mbBMS!= NULL)
    {
        modbus_close(mbBMS);
        modbus_free(mbBMS);

        mbBMS = modbus_new_tcp(sys_config.modbus_BMS_ip.c_str(), mbBMSPort);
        modbus_set_slave(mbBMS,sys_config.modbus_BMS_slave_id);

        if(mbBMS != NULL)
        {
            if (modbus_connect(mbBMS) == -1)
            {
                fprintf(stdout, "BMS Modbus reConnection failed: %s\n", modbus_strerror(errno));
                isBMSModbusConn = false;
            }
            else
            {
                printf("BMS Modbus connection successful.");
                isBMSModbusConn = true;
            }
        }
        else
        {
            printf("Could not connect over BMS MODBUS TCP. \r\n");
        }
    }
    else
    {
        printf("Do nothing to BMS MODBUS, should come back later. \r\n");
    }
}

void reconnectToBESSModbus()
{
    if(mbBESS!= NULL)
    {
        modbus_close(mbBESS);
        modbus_free(mbBESS);

        mbBESS = modbus_new_tcp(sys_config.modbus_BESS_ip.c_str(), mbBESSPort);
        modbus_set_slave(mbBESS, sys_config.modbus_BESS_slave_id);

        if(mbBESS!= NULL)
        {
            if (modbus_connect(mbBESS) == -1)
            {
                fprintf(stdout, "BESS Modbus reConnection failed: %s\n", modbus_strerror(errno));
                isBESSModbusConn = false;
            }
            else
            {
                printf("BESS Modbus connection successful.");
                isBESSModbusConn = true;
            }
        }
        else
        {
            printf("Could not connect over BESS MODBUS TCP. \r\n");
        }
    }
    else
    {
        printf("Do nothing to BESS MODBUS, should come back later. \r\n");
    }
}

void reconnectToLitModbus()
{
    if(mbLit!= NULL)
    {
        modbus_close(mbLit);
        modbus_free(mbLit);

        mbLit = modbus_new_tcp(sys_config.modbus_Lit_ip.c_str(), mbLitPort);
        modbus_set_slave(mbLit, sys_config.modbus_Lit_slave_id);

        if(mbLit!= NULL)
        {
            if (modbus_connect(mbLit) == -1)
            {
                fprintf(stdout, "Lit Modbus reConnection failed: %s\n", modbus_strerror(errno));
                isLitModbusConn = false;
            }
            else
            {
                printf("Lit Modbus connection successful.");
                isLitModbusConn = true;
            }
        }
        else
        {
            printf("Could not connect over Lit MODBUS TCP. \r\n");
        }
    }
    else
    {
        printf("Do nothing to Lit MODBUS, should come back later. \r\n");
    }
}

void reconnectToMGModbus()
{
    if(mbMG!= NULL)
    {
        modbus_close(mbMG);
        modbus_free(mbMG);

        mbMG = modbus_new_tcp(sys_config.modbus_MG_ip.c_str(), mbMGPort);
        modbus_set_slave(mbMG, sys_config.modbus_MG_slave_id);

        if(mbMG!= NULL)
        {
            if (modbus_connect(mbMG) == -1)
            {
                fprintf(stdout, "MG Modbus reConnection failed: %s\n", modbus_strerror(errno));
                isMGModbusConn = false;
            }
            else
            {
                printf("MG Modbus connection successful.");
                isMGModbusConn = true;
            }
        }
        else
        {
            printf("Could not connect over MG MODBUS TCP. \r\n");
        }
    }
    else
    {
        printf("Do nothing to MG MODBUS, should come back later. \r\n");
    }
}
// End of Section 4
//////////////////////////////////////////////////////////////////////////////////

void getCurrentTime(void)
{
    timeNow = time(NULL);
    ts = localtime(&timeNow);
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M", ts);
}

void logErrorInDB()
{
    mtx_access_error_data_db.lock();
    getCurrentTime();
    logErrors(dbErr, stmtErr, timeNow, errorMsg, strlen(errorMsg)+1);
    mtx_access_error_data_db.unlock();
    mtx_error_msg.unlock();
}

int x;
void checkInternetConnectivity()
{
    x = system("ping -c1 -w1 8.8.8.8 > /dev/null 2>&1");
    if(x == 0)
    {
        isInternetConn = true;
    }
    else
    {
        isInternetConn = false;
        printf("Internet Connection Failed\r\n");
    }
}

//////////////////////////////////////////////////////////////////////////////////
// Section 5
void populateSensorConfiguration()
{
    sensor_configuration.push_back(new sensor_config(0,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.bcu_leakage_sensor)));
    sensor_configuration.push_back(new sensor_config(2,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.positive_tank_high_level_float)));
    sensor_configuration.push_back(new sensor_config(3,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.negative_tank_high_level_float)));
    sensor_configuration.push_back(new sensor_config(4,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.positive_tank_low_level_float)));
    sensor_configuration.push_back(new sensor_config(5,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.negative_tank_low_level_float)));
    sensor_configuration.push_back(new sensor_config(6,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.smoke_sensor)));
    sensor_configuration.push_back(new sensor_config(7,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.bcu_voltage)));
    sensor_configuration.push_back(new sensor_config(9,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.bcu_current)));  
    sensor_configuration.push_back(new sensor_config(11,reg_type::HOLDING_REG,0.001f,&(latest_battery_data.bcu_ocv)));
    sensor_configuration.push_back(new sensor_config(12,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.bcu_positive_tank_temp)));
    sensor_configuration.push_back(new sensor_config(13,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.bcu_negative_tank_temp)));
    sensor_configuration.push_back(new sensor_config(14,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.bcu_hydrogen_sensor)));
    sensor_configuration.push_back(new sensor_config(15,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.humidity)));
    sensor_configuration.push_back(new sensor_config(16,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.sensor_temp)));
    sensor_configuration.push_back(new sensor_config(17,reg_type::HOLDING_REG,0.1f,&(latest_battery_data.primary_stack_positive_pressure_sensor)));
    sensor_configuration.push_back(new sensor_config(18,reg_type::HOLDING_REG,0.1f,&(latest_battery_data.primary_stack_negative_pressure_sensor)));
    sensor_configuration.push_back(new sensor_config(21,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_positive_pump)));
    sensor_configuration.push_back(new sensor_config(22,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_negative_pump)));
    sensor_configuration.push_back(new sensor_config(23,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.positive_valve)));
    sensor_configuration.push_back(new sensor_config(24,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.negative_valve)));
    sensor_configuration.push_back(new sensor_config(25,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.balancing_valve)));    
    sensor_configuration.push_back(new sensor_config(26,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_charging_relay)));
    sensor_configuration.push_back(new sensor_config(27,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_discharge_relay)));
    sensor_configuration.push_back(new sensor_config(50,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.bcu_state_of_charge)));
    sensor_configuration.push_back(new sensor_config(50,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.state_of_charge))); 

    sensor_configuration.push_back(new sensor_config(61,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.pcs1_dc_volts))); 		    // Added on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(62,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.pcs1_dc_batt_current))); 	    // Added on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(64,reg_type::HOLDING_REG,0.1f,&(latest_battery_data.pcs1_dc_inverter_power))); 	// Added on 11 Aug 21

    sensor_configuration.push_back(new sensor_config(65,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.pcs1_voltage)));
    sensor_configuration.push_back(new sensor_config(66,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.pcs1_current)));
    sensor_configuration.push_back(new sensor_config(69,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.pcs1_reactive_power)));
    sensor_configuration.push_back(new sensor_config(74,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.pcs1_load_power)));
    sensor_configuration.push_back(new sensor_config(68,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.pcs1_ac_supply_power)));
}

void populateBESSConfiguration()
{
    BESS_configuration.push_back(new sensor_config(3098,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.bess_voltage_gain)));
    BESS_configuration.push_back(new sensor_config(3099,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.bess_voltage_int)));
}

void populateLitConfiguration()
{
    Lit_configuration.push_back(new sensor_config(3098,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.lit_voltage_gain)));
    Lit_configuration.push_back(new sensor_config(3099,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.lit_voltage_int)));
}

void populateMGConfiguration()
{
    MG_configuration.push_back(new sensor_config(3098,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.mg_voltage_gain)));
    MG_configuration.push_back(new sensor_config(3099,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.mg_voltage_int)));
}
// End of Section 5
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Section 6
void printSensorData(sensor_data data)
{
    std::cout << std::right;
    std::cout << std::setfill('-');
    std::cout << std::setw(45) << "Sensor Data" << std::endl;
    std::cout << std::setfill(' ');
    std::cout << std::left;
    std::cout << std::setw(40) << "Uptime" << std::setw(15) << data.uptime << std::endl;
    std::cout << std::setw(40) << "Timestamp" << std::setw(15) << data.timestamp << std::endl;
    std::cout << std::setw(40) << "markToSent" << std::setw(15) << markToSent << std::endl;
    std::cout << std::setw(40) << "bcu_leakage_sensor" << std::setw(15) << data.battery.bcu_leakage_sensor << std::endl;
    std::cout << std::setw(40) << "positive_tank_high_level_float" << std::setw(15) << data.battery.positive_tank_high_level_float << std::endl;
    std::cout << std::setw(40) << "negative_tank_high_level_float" << std::setw(15) << data.battery.negative_tank_high_level_float << std::endl;
    std::cout << std::setw(40) << "positive_tank_low_level_float" << std::setw(15) << data.battery.positive_tank_low_level_float << std::endl;
    std::cout << std::setw(40) << "negative_tank_low_level_float" << std::setw(15) << data.battery.negative_tank_low_level_float << std::endl;
    std::cout << std::setw(40) << "smoke_sensor" << std::setw(15) << data.battery.smoke_sensor << std::endl;
    std::cout << std::setw(40) << "bcu_voltage" << std::setw(15) << data.battery.bcu_voltage << std::endl;
    std::cout << std::setw(40) << "bcu_current" << std::setw(15) << data.battery.bcu_current << std::endl;
    std::cout << std::setw(40) << "bcu_ocv" << std::setw(15) << data.battery.bcu_ocv << std::endl;
    std::cout << std::setw(40) << "bcu_positive_tank_temp" << std::setw(15) << data.battery.bcu_positive_tank_temp << std::endl;
    std::cout << std::setw(40) << "bcu_negative_tank_temp" << std::setw(15) << data.battery.bcu_negative_tank_temp << std::endl;
    std::cout << std::setw(40) << "bcu_hydrogen_sensor" << std::setw(15) << data.battery.bcu_hydrogen_sensor << std::endl;
    std::cout << std::setw(40) << "humidity" << std::setw(15) << data.battery.humidity << std::endl;
    std::cout << std::setw(40) << "sensor_temp" << std::setw(15) << data.battery.sensor_temp << std::endl;
    std::cout << std::setw(40) << "primary_stack_positive_pressure_sensor" << std::setw(15) << data.battery.primary_stack_positive_pressure_sensor << std::endl;
    std::cout << std::setw(40) << "primary_stack_negative_pressure_sensor" << std::setw(15) << data.battery.primary_stack_negative_pressure_sensor << std::endl;
    std::cout << std::setw(40) << "primary_positive_pump" << std::setw(15) << data.battery.primary_positive_pump << std::endl;
    std::cout << std::setw(40) << "primary_negative_pump" << std::setw(15) << data.battery.primary_negative_pump << std::endl;
    std::cout << std::setw(40) << "positive_valve" << std::setw(15) << data.battery.positive_valve << std::endl;
    std::cout << std::setw(40) << "negative_valve" << std::setw(15) << data.battery.negative_valve << std::endl;
    std::cout << std::setw(40) << "balancing_valve" << std::setw(15) << data.battery.balancing_valve << std::endl;
    std::cout << std::setw(40) << "primary_charging_relay" << std::setw(15) << data.battery.primary_charging_relay << std::endl;
    std::cout << std::setw(40) << "primary_discharge_relay" << std::setw(15) << data.battery.primary_discharge_relay << std::endl;
    std::cout << std::setw(40) << "state_of_charge" << std::setw(15) << data.battery.state_of_charge << std::endl;
    std::cout << std::setw(40) << "pcs1_dc_batt_current" << std::setw(15) << data.battery.pcs1_dc_batt_current << std::endl;
    std::cout << std::setw(40) << "pcs1_dc_inverter_power" << std::setw(15) << data.battery.pcs1_dc_inverter_power << std::endl;
    std::cout << std::setw(40) << "pcs1_voltage" << std::setw(15) << data.battery.pcs1_voltage << std::endl;
    std::cout << std::setw(40) << "pcs1_current" << std::setw(15) << data.battery.pcs1_current << std::endl;
    std::cout << std::setw(40) << "pcs1_reactive_power" << std::setw(15) << data.battery.pcs1_reactive_power << std::endl;
    std::cout << std::setw(40) << "pcs1_load_power" << std::setw(15) << data.battery.pcs1_load_power << std::endl;
    std::cout << std::setw(40) << "pcs1_ac_supply_power" << std::setw(15) << data.battery.pcs1_ac_supply_power << std::endl;
    std::cout << "==================================================" << std::endl;
}
void printBESSData(sensor_data data)
{
    std::cout << std::right;
    std::cout << std::setfill('-');
    std::cout << std::setw(45) << "BESS Data" << std::endl;
    std::cout << std::setfill(' ');
    std::cout << std::left;
    std::cout << std::setw(40) << "voltage_gain" << std::setw(15) << data.battery.bess_voltage_gain << std::endl;
    std::cout << std::setw(40) << "voltage_int" << std::setw(15) << data.battery.bess_voltage_int << std::endl;
    std::cout << "==================================================" << std::endl;
}
void printLitData(sensor_data data)
{
    std::cout << std::right;
    std::cout << std::setfill('-');
    std::cout << std::setw(45) << "Lit Data" << std::endl;
    std::cout << std::setfill(' ');
    std::cout << std::left;
    std::cout << std::setw(40) << "voltage_gain" << std::setw(15) << data.battery.lit_voltage_gain << std::endl;
    std::cout << std::setw(40) << "voltage_int" << std::setw(15) << data.battery.lit_voltage_int << std::endl;
    std::cout << "==================================================" << std::endl;
}
void printMGData(sensor_data data)
{
    std::cout << std::right;
    std::cout << std::setfill('-');
    std::cout << std::setw(45) << "MG Data" << std::endl;
    std::cout << std::setfill(' ');
    std::cout << std::left;
    std::cout << std::setw(40) << "voltage_gain" << std::setw(15) << data.battery.mg_voltage_gain << std::endl;
    std::cout << std::setw(40) << "voltage_int" << std::setw(15) << data.battery.mg_voltage_int << std::endl;
    std::cout << "==================================================" << std::endl;
}
// End of Section 6
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Section 7
void probeBMSSensors()
{
    if(isBMSModbusConn)
    {
        // printf("Probing sensors to read modbus data\r\n");
        for(resCode = 0; resCode < sensor_configuration.size(); resCode++)
        {
            sensor_config* current_reg = sensor_configuration.at(resCode);
            int addr = (current_reg->reg_offset);
            int readCode = 0;

            if(current_reg->regType == reg_type::HOLDING_REG)
            {
                readCode = modbus_read_registers(mbBMS, addr , current_reg->noOfRegsToRead, sensor_value);
            }
            else
            {
                readCode = modbus_read_input_registers(mbBMS, addr , current_reg->noOfRegsToRead, sensor_value);
            }

            if(readCode == -1)
            {
                printf("ERROR: %s\n", modbus_strerror(errno));
                isBMSModbusConn = false;
                break;
            }
            else
            {
                if(current_reg->noOfRegsToRead == 1)
                {
                    float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)sensor_value[0];
                    }
                    if(current_reg->valueType == data_type::DT_INT16)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                        // printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);  
                        // *(current_reg->dest_float_ptr) = (float)sensor_value[0];
                        *(current_reg->dest_float_ptr) = scaledValue;
                        // printf("*(current_reg->dest_float_ptr) : [%f]\r\n",*(current_reg->dest_float_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_INT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                        // printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                }
                else if(current_reg->noOfRegsToRead == 2)
                {
                    float scaledValue = ((((int32_t)sensor_value[0] << 16) | sensor_value[1])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_UINT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int32_t)scaledValue;
                    }
                    else if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        *(current_reg->dest_float_ptr) = scaledValue;
                    } 
                    else if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)scaledValue;
                    }
                }
            }
        }
    }
    else if(noOfBMSModbusAttempts < sys_config.noOfModbusAttemptsAllowed)
    {
        // Will be unlocked in mtx_error_msg
        mtx_error_msg.lock();
        sprintf(errorMsg, "BMS Modbus Read Failed. Trying another attempt! Line No %d", __LINE__);
        logErrorInDB();

        printf("BMS Modbus Read Failed. Trying another attempt!\r\n");
        noOfBMSModbusAttempts++;
    }  
    else
    {
        // we have reached the limit for no of attempts... sleep for a while and then try again
        mtx_error_msg.lock();
        sprintf(errorMsg, "BMS Modbus might not be connected. Trying to reconnect! Line No %d", __LINE__);
        logErrorInDB();

        printf("BMS Modbus is not connected. Trying to reconnect!\r\n");
        reconnectToBMSModbus();
        noOfBMSModbusAttempts = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.modbus_data_read_interval));
}

void probeBESSSensors()
{
    if(isBESSModbusConn)
    {
        // printf("Probing sensors to read modbus data\r\n");

        for(resCode2 = 0; resCode2 < BESS_configuration.size(); resCode2++)
        {
            sensor_config* current_reg = BESS_configuration.at(resCode2); 
            int addr = (current_reg->reg_offset);
            int readCode = 0;

            if(current_reg->regType == reg_type::HOLDING_REG)
            {
                readCode = modbus_read_registers(mbBESS, addr , current_reg->noOfRegsToRead, sensor_value);
            }
            else 
            {
                readCode = modbus_read_input_registers(mbBESS, addr , current_reg->noOfRegsToRead, sensor_value);
            }
            if(readCode == -1)
            {
                printf("ERROR: %s\n", modbus_strerror(errno));
                isBESSModbusConn = false;
                break;
            }
            else
            {
                if(current_reg->noOfRegsToRead == 1)
                {
                    float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)sensor_value[0];
                    }
                    if(current_reg->valueType == data_type::DT_INT16)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                    //   printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);  
                        // *(current_reg->dest_float_ptr) = (float)sensor_value[0];
                        *(current_reg->dest_float_ptr) = scaledValue;
                        // printf("*(current_reg->dest_float_ptr) : [%f]\r\n",*(current_reg->dest_float_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_INT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                        // printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                }
                else if(current_reg->noOfRegsToRead == 2)
                {
                    float scaledValue = ((((int32_t)sensor_value[0] << 16) | sensor_value[1])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_UINT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int32_t)scaledValue;
                    }
                    else if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        *(current_reg->dest_float_ptr) = scaledValue;
                    } 
                    else if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)scaledValue;
                    }
                }
            }
        }
    }
    else if(noOfBESSModbusAttempts < sys_config.noOfModbusAttemptsAllowed)
    {
        // Will be unlocked in mtx_error_msg
        mtx_error_msg.lock();
        sprintf(errorMsg, "BESS Modbus Read Failed. Trying another attempt! Line No %d", __LINE__);
        logErrorInDB();

        printf("BESS Modbus Read Failed. Trying another attempt!\r\n");
        noOfBESSModbusAttempts++;
    }  
    else
    {
        // we have reached the limit for no of attempts... sleep for a while and then try again
        mtx_error_msg.lock();
        sprintf(errorMsg, "BESS Modbus might not be connected. Trying to reconnect! Line No %d", __LINE__);
        logErrorInDB();

        printf("BESS Modbus is not connected. Trying to reconnect!\r\n");
        reconnectToBESSModbus();
        noOfBESSModbusAttempts = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.modbus_data_read_interval));
}

void probeLitSensors()
{
    if(isLitModbusConn)
    {
        // printf("Probing sensors to read modbus data\r\n");

        for(resCode3 = 0; resCode3 < Lit_configuration.size(); resCode3++)
        {
            sensor_config* current_reg = Lit_configuration.at(resCode3); 
            int addr = (current_reg->reg_offset);
            int readCode = 0;

            if(current_reg->regType == reg_type::HOLDING_REG)
            {
                readCode = modbus_read_registers(mbLit, addr , current_reg->noOfRegsToRead, sensor_value);
            }
            else 
            {
                readCode = modbus_read_input_registers(mbLit, addr , current_reg->noOfRegsToRead, sensor_value);
            }
            if(readCode == -1)
            {
                printf("ERROR: %s\n", modbus_strerror(errno));
                isLitModbusConn = false;
                break;
            }
            else
            {
                if(current_reg->noOfRegsToRead == 1)
                {
                    float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)sensor_value[0];
                    }
                    if(current_reg->valueType == data_type::DT_INT16)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                    //   printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);  
                        // *(current_reg->dest_float_ptr) = (float)sensor_value[0];
                        *(current_reg->dest_float_ptr) = scaledValue;
                        // printf("*(current_reg->dest_float_ptr) : [%f]\r\n",*(current_reg->dest_float_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_INT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                        // printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                }
                else if(current_reg->noOfRegsToRead == 2)
                {
                    float scaledValue = ((((int32_t)sensor_value[0] << 16) | sensor_value[1])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_UINT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int32_t)scaledValue;
                    }
                    else if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        *(current_reg->dest_float_ptr) = scaledValue;
                    } 
                    else if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)scaledValue;
                    }
                }
            }
        }
    }
    else if(noOfLitModbusAttempts < sys_config.noOfModbusAttemptsAllowed)
    {
        // Will be unlocked in mtx_error_msg
        mtx_error_msg.lock();
        sprintf(errorMsg, "Lit Modbus Read Failed. Trying another attempt! Line No %d", __LINE__);
        logErrorInDB();

        printf("Lit Modbus Read Failed. Trying another attempt!\r\n");
        noOfLitModbusAttempts++;
    }  
    else
    {
        // we have reached the limit for no of attempts... sleep for a while and then try again
        mtx_error_msg.lock();
        sprintf(errorMsg, "Lit Modbus might not be connected. Trying to reconnect! Line No %d", __LINE__);
        logErrorInDB();

        printf("Lit Modbus is not connected. Trying to reconnect!\r\n");
        reconnectToLitModbus();
        noOfLitModbusAttempts = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.modbus_data_read_interval));
}

void probeMGSensors()
{
    if(isMGModbusConn)
    {
        // printf("Probing sensors to read modbus data\r\n");

        for(resCode4 = 0; resCode4 < MG_configuration.size(); resCode4++)
        {
            sensor_config* current_reg = MG_configuration.at(resCode4); 
            int addr = (current_reg->reg_offset);
            int readCode = 0;

            if(current_reg->regType == reg_type::HOLDING_REG)
            {
                readCode = modbus_read_registers(mbMG, addr , current_reg->noOfRegsToRead, sensor_value);
            }
            else 
            {
                readCode = modbus_read_input_registers(mbMG, addr , current_reg->noOfRegsToRead, sensor_value);
            }
            if(readCode == -1)
            {
                printf("ERROR: %s\n", modbus_strerror(errno));
                isMGModbusConn = false;
                break;
            }
            else
            {
                if(current_reg->noOfRegsToRead == 1)
                {
                    float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)sensor_value[0];
                    }
                    if(current_reg->valueType == data_type::DT_INT16)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                    //   printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        float scaledValue = (((int16_t)sensor_value[0])*current_reg->scale);  
                        // *(current_reg->dest_float_ptr) = (float)sensor_value[0];
                        *(current_reg->dest_float_ptr) = scaledValue;
                        // printf("*(current_reg->dest_float_ptr) : [%f]\r\n",*(current_reg->dest_float_ptr));
                    }
                    if(current_reg->valueType == data_type::DT_INT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                        // printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                    }
                }
                else if(current_reg->noOfRegsToRead == 2)
                {
                    float scaledValue = ((((int32_t)sensor_value[0] << 16) | sensor_value[1])*current_reg->scale);
                    if(current_reg->valueType == data_type::DT_UINT32)
                    {
                        *(current_reg->dest_int32_ptr) = (int32_t)scaledValue;
                    }
                    else if(current_reg->valueType == data_type::DT_FLOAT)
                    {
                        *(current_reg->dest_float_ptr) = scaledValue;
                    } 
                    else if(current_reg->valueType == data_type::DT_BOOL)
                    {
                        *(current_reg->dest_bool_ptr) = (bool)scaledValue;
                    }
                }
            }
        }
    }
    else if(noOfMGModbusAttempts < sys_config.noOfModbusAttemptsAllowed)
    {
        // Will be unlocked in mtx_error_msg
        mtx_error_msg.lock();
        sprintf(errorMsg, "MG Modbus Read Failed. Trying another attempt! Line No %d", __LINE__);
        logErrorInDB();

        printf("MG Modbus Read Failed. Trying another attempt!\r\n");
        noOfMGModbusAttempts++;
    }  
    else
    {
        // we have reached the limit for no of attempts... sleep for a while and then try again
        mtx_error_msg.lock();
        sprintf(errorMsg, "MG Modbus might not be connected. Trying to reconnect! Line No %d", __LINE__);
        logErrorInDB();

        printf("MG Modbus is not connected. Trying to reconnect!\r\n");
        reconnectToMGModbus();
        noOfMGModbusAttempts = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.modbus_data_read_interval));
}
// End of Section 7
//////////////////////////////////////////////////////////////////////////////////

void setMarkToSent()
{
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.normal_packet_sent_interval));
        mtx_mark_to_sent.lock();
        markToSent = 1;
        mtx_mark_to_sent.unlock();
    }
}
//////////////////////////////////////////////////////////////////////////////////
// Section 2
void sendSensorData()
{
    while(1)
    {
        mtx_access_sensor_data_db.lock();
        int count = getRowCount(db);
        if(count > 1) // there is more data, then send data to server faster
        {
            printf("Pending Data count: [%d]\r\n",count);
            sendDataFaster = true;
        }
        else if(count == -1)
        {
            mtx_error_msg.lock();
            sprintf(errorMsg, "Could not get RowCount from DB, Line No %d", __LINE__);
            logErrorInDB();
        }
        else //// there is less data, reduce the sending time
        {
            printf("Data count: [%d]\r\n",count);
            sendDataFaster = false;
        }
        mtx_curr_error.lock();
        curr_error = getSensorDataEntry(db, &db_sensor_data);
        mtx_access_sensor_data_db.unlock();

        if(curr_error.lineNo != -1)
        {
            mtx_error_msg.lock();
            sprintf(errorMsg, "Could not get Sensor Data from DB, SQL RC [%d], Line No [%d]", curr_error.rc, curr_error.lineNo);
            mtx_curr_error.unlock();
            logErrorInDB();
        }
        else
        {
            mtx_curr_error.unlock();
        }
//////////////////////////////////////////////////////////////////////////////////
// Section 2.1
        int status_vft=0;
        int status_mads=0;
// End of Section 2.1
//////////////////////////////////////////////////////////////////////////////////
        if(db_sensor_data.isValid)
        {
            encode_sensor_data_to_json(sendDataJSON, db_sensor_data, sys_config);
            // printf("JSON Str: %s\n", sendDataString);
            sendDataString = sendDataJSON.dump(4);
            // printf("JSON Str: %s\n", sendDataString);
//////////////////////////////////////////////////////////////////////////////////
// Section 2.2
            try
            {
                // send a post request
                vftPlt::madsRequest.set_bearer_token_auth(sys_config.mads_auth_token.c_str());
                auto madsResponseVFT = vftPlt::madsRequest.Post(sys_config.mads_url.c_str(), sendDataString, "application/json");  

                if(madsResponseVFT!=nullptr) //NULL earlier
                {
                    if(madsResponseVFT->body.empty())
                    {
                        std::cout <<"BODY is null";
                    }
                    else
                    {   
                        status_vft = madsResponseVFT->status;   
                        std::cout << std::string(madsResponseVFT->body.begin(), madsResponseVFT->body.end()) << '\n'; // print the result
                    }
                }
                else
                {
                    std::cout << "VFT error" << madsResponseVFT.error();   
                    mtx_error_msg.lock();
                    sprintf(errorMsg, "Internet is connected, but VFT server might not. Line No [%d], status_vft [%d]", __LINE__, status_vft);
                    logErrorInDB();
                }
            }
            catch (const std::exception& e)
            {
            std::cerr << "Request failed, error: " << e.what() << '\n';
            }
// End of Section 2.2
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Duplicated code for MADS
            try
            {
                // send a post request
                madsPlt::madsRequest.set_bearer_token_auth(madsPlt::mads_auth_token.c_str());
                auto madsResponse = madsPlt::madsRequest.Post(madsPlt::mads_url.c_str(), sendDataString, "application/json");  

                if(madsResponse!=nullptr) //NULL earlier
                {
                    if(madsResponse->body.empty())
                    {
                        std::cout <<"BODY is null";
                    }
                    else
                    {   
                        status_mads = madsResponse->status;   
                        std::cout << std::string(madsResponse->body.begin(), madsResponse->body.end()) << '\n'; // print the result
                    }
                }
                else
                {
                    std::cout << "MADS error" << madsResponse.error();   
                    mtx_error_msg.lock();
                    sprintf(errorMsg, "Internet is connected, but MADS server might not. Line No [%d], status_mads [%d]", __LINE__, status_mads);
                    logErrorInDB();
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Request failed, error: " << e.what() << '\n';
            }
// End of duplicated code for MADS
//////////////////////////////////////////////////////////////////////////////////
        }
        // else
        // {
        //     printf("A debug, invalid database.\n\b");
        // }
//////////////////////////////////////////////////////////////////////////////////
// Section 2.3
        if(status_vft == 202)  
        {
            mtx_access_sensor_data_db.lock();
            mtx_curr_error.lock();
            curr_error = updateSensorDataEntry(db);
            mtx_access_sensor_data_db.unlock();

            if(curr_error.lineNo != -1)
            {
                mtx_error_msg.lock();
                sprintf(errorMsg, "Could not update Sensor Data in DB, SQL RC [%d], Line No [%d]", curr_error.rc, curr_error.lineNo);
                mtx_curr_error.unlock();
                logErrorInDB();
            }
            else
            {
                mtx_curr_error.unlock();
            }
            noOfInternetAttempts = 0;
            printf("Updating DATABASE System\r\n");
        }
        else
        {   
            if(noOfInternetAttempts < noOfInternetAttemptsAllowed)
            {
                checkInternetConnectivity();
                if(!isInternetConn)
                {
                    noOfInternetAttempts++;
                    mtx_error_msg.lock();
                    sprintf(errorMsg, "Internet might not be connected, Trying another attempt! Line No [%d]", __LINE__);
                    logErrorInDB();
                }
                else
                {
                    mtx_error_msg.lock();
                    sprintf(errorMsg, "Internet is connected, but server might not. Will trying another attempt! Line No [%d], status_mads [%d]", __LINE__, status_mads);
                    logErrorInDB();
                }
            }
            else
            {
                //no of attempts to try internet connectivity expired, restart raspberry pi.
                noOfInternetAttempts = 0;
                mtx_error_msg.lock();
                sprintf(errorMsg, "No of attempts to try internet connectivity expired, reboot raspberry pi! Line No [%d]", __LINE__);
                logErrorInDB();
                printf("Rebooting System\r\n");
                sqlite3_close(db);  
                sqlite3_close(dbErr);  
                system("shutdown -r now");
            }
        }
// End of Section 2.3
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Duplicated code for MADS
        if(status_mads == 202)  
        {
            mtx_access_sensor_data_db.lock();
            mtx_curr_error.lock();
            curr_error = updateSensorDataEntry(db);
            mtx_access_sensor_data_db.unlock();

            if(curr_error.lineNo != -1)
            {
                mtx_error_msg.lock();
                sprintf(errorMsg, "Could not update Sensor Data in DB, SQL RC [%d], Line No [%d]", curr_error.rc, curr_error.lineNo);
                mtx_curr_error.unlock();
                logErrorInDB();
            }
            else
            {
                mtx_curr_error.unlock();
            }
            noOfInternetAttempts = 0;
            printf("Updating DATABASE System\r\n");
        }
        else
        {   
            if(noOfInternetAttempts < noOfInternetAttemptsAllowed)
            {
                checkInternetConnectivity();
                if(!isInternetConn)
                {
                    noOfInternetAttempts++;
                    mtx_error_msg.lock();
                    sprintf(errorMsg, "Internet might not be connected, Trying another attempt! Line No [%d]", __LINE__);
                    logErrorInDB();
                }
                else
                {
                    mtx_error_msg.lock();
                    sprintf(errorMsg, "Internet is connected, but server might not. Will trying another attempt! Line No [%d], status_vft [%d]", __LINE__, status_vft);
                    logErrorInDB();
                }
            }
            else
            {
                //no of attempts to try internet connectivity expired, restart raspberry pi.
                noOfInternetAttempts = 0;
                mtx_error_msg.lock();
                sprintf(errorMsg, "No of attempts to try internet connectivity expired, reboot raspberry pi! Line No [%d]", __LINE__);
                logErrorInDB();
                printf("Rebooting System\r\n");
                sqlite3_close(db);  
                sqlite3_close(dbErr);  
                system("shutdown -r now");
            }
        }
// End of duplicated code for MADS
//////////////////////////////////////////////////////////////////////////////////
        ////////////else part of sent succeed//////// check for internet connectivity
        if(sendDataFaster)
        {
            noOfInternetAttemptsAllowed = sys_config.fastNoOfInternetAttemptsAllowed;
            std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.fast_packet_sent_interval));
        }
        else
        {
            noOfInternetAttemptsAllowed = sys_config.normalNoOfInternetAttemptsAllowed;
            std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.normal_packet_sent_interval));
        }
    }
}
// End of Section 2
//////////////////////////////////////////////////////////////////////////////////

void save_system_config()
{
    json system_conf;

    system_conf["modbus_BMS_ip"] = "192.168.0.50";
    system_conf["modbus_BMS_slave_id"] = 1;
    system_conf["modbus_ComAP_ip"] = "192.168.0.41";
    system_conf["modbus_ComAP_slave_id"] = 1;
    system_conf["noOfModbusAttemptsAllowed"] = 3;
    system_conf["modbus_data_read_interval"] = 5000;
    system_conf["mads_auth_token"] = "n03d1jv2wlfuygsshssygugdugsssgvb";
    system_conf["mads_url"] = "/iot/orgs/3/projects/70/gateways/28/data_dump";
    system_conf["normal_packet_sent_interval"] = 6000;
    system_conf["normalNoOfInternetAttemptsAllowed"] = 60;
    system_conf["fast_packet_sent_interval"] = 1000;
    system_conf["fastNoOfInternetAttemptsAllowed"] = 360;
    system_conf["asset_id"] = 2;

    // write configuration JSON to another file
    std::ofstream o("config.json");
    o << std::setw(4) << system_conf << std::endl;  

}
//////////////////////////////////////////////////////////////////////////////////
// Section 15
void read_system_config()
{
    json system_json_config;
    cout<<"Reading"<<endl;
    // read a JSON file
    std::ifstream i("/home/adv/Desktop/modbustcplogger/config.json");
    i >> system_json_config;

    string modbus_BMS_ip = system_json_config["modbus_BMS_ip"];
    string modbus_BESS_ip = system_json_config["modbus_BESS_ip"];
    string modbus_Lit_ip = system_json_config["modbus_Lit_ip"];
    string modbus_MG_ip = system_json_config["modbus_MG_ip"];
    string mads_auth_token = system_json_config["mads_auth_token"];
    string mads_url = system_json_config["mads_url"];

    sys_config.modbus_BMS_ip = modbus_BMS_ip;
    sys_config.modbus_BMS_slave_id = system_json_config["modbus_BMS_slave_id"];
    sys_config.modbus_BESS_ip = modbus_BESS_ip;
    sys_config.modbus_BESS_slave_id = system_json_config["modbus_BESS_slave_id"];
    sys_config.modbus_Lit_ip = modbus_Lit_ip;
    sys_config.modbus_Lit_slave_id = system_json_config["modbus_Lit_slave_id"];
    sys_config.modbus_MG_ip = modbus_MG_ip;
    sys_config.modbus_MG_slave_id = system_json_config["modbus_MG_slave_id"];
    sys_config.noOfModbusAttemptsAllowed = system_json_config["noOfModbusAttemptsAllowed"];
    sys_config.modbus_data_read_interval = system_json_config["modbus_data_read_interval"];
    sys_config.mads_auth_token = mads_auth_token;
    sys_config.mads_url = mads_url;
    sys_config.normal_packet_sent_interval = system_json_config["normal_packet_sent_interval"];
    sys_config.normalNoOfInternetAttemptsAllowed = system_json_config["normalNoOfInternetAttemptsAllowed"];
    sys_config.fast_packet_sent_interval = system_json_config["fast_packet_sent_interval"];
    sys_config.fastNoOfInternetAttemptsAllowed = system_json_config["fastNoOfInternetAttemptsAllowed"];
    sys_config.asset_id = system_json_config["asset_id"];
}
// End of Section 15
//////////////////////////////////////////////////////////////////////////////////
sqlite3_stmt *stmtDel;

void createDBSpace()
{
    while(1)
    {
        printf("Running DB cleanup task\r\n");

        long timestampInterval = 6 * 30 * 24 * 3600; // denotes seconds corresponding to 6 months

        int recordCount = getRecordCount(db);

        if(recordCount > 5000000) //corresponds to approx 950 MB data <- 156 MB required for 8 lakhs records 
        {
            printf("Total records: [%d]\r\n",recordCount);

            long recentTimestamp = getRecentTimestamp(db);
            long oldTimestamp = recentTimestamp - timestampInterval;

            printf("Curr timestamp: [%ld], Old timestamp: [%ld]\r\n",recentTimestamp, oldTimestamp);

            int deleteRecordCount = getDeleteRecordCount(db, stmtDel, oldTimestamp);
            if(deleteRecordCount > 0)
            {
                //delete 6 months data from now
                deleteSensorDataEntries(db, stmtDel, oldTimestamp);
                executeVacuum(db);

                printf("Deleted records: [%d]\r\n",deleteRecordCount);

                mtx_error_msg.lock();
                sprintf(errorMsg, "Deleted records %d from DB, Line No %d", deleteRecordCount, __LINE__);
                logErrorInDB();
            }   
        }
        else if(recordCount == -1)
        {
            mtx_error_msg.lock();
            sprintf(errorMsg, "Could not delete older records from DB, Line No %d", __LINE__);
            logErrorInDB();
        }
        std::this_thread::sleep_for(3600000ms); //corresponds to 1 hr
    }
}
//////////////////////////////////////////////////////////////////////////////////
// Section 8
void sensorsProbing (void)
{
    while(1)
    {
//////////////////////////////////////////////////////////////////////////////////
// Section 8.1
        probeBMSSensors();
        probeBESSSensors();
        probeLitSensors();
        probeMGSensors();
// End of Section 8.1
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Section 8.2
        if((resCode == sensor_configuration.size() && isBMSModbusConn) || 
            (resCode2 == BESS_configuration.size() && isBESSModbusConn) || 
            (resCode3 == Lit_configuration.size() && isLitModbusConn) ||
            (resCode3 == MG_configuration.size() && isMGModbusConn))
// End of Section 8.2
//////////////////////////////////////////////////////////////////////////////////
        {
            getCurrentTime();
//////////////////////////////////////////////////////////////////////////////////
// Section 8.3
            if(resCode == sensor_configuration.size())
            {
                resCode = 0;
                noOfBMSModbusAttempts = 0;

                printf("Probed Sensors, system uptime: [%ld] isBMSModbusConn: [%d] markToSent: [%d]\r\n", actual_sensor_data.uptime, isBMSModbusConn, markToSent);
                printSensorData(actual_sensor_data);
            }
            if(resCode2 == BESS_configuration.size())
            {
                resCode2 = 0;
                noOfBESSModbusAttempts = 0;

                printf("Probed Sensors, system uptime: [%ld] isBESSModbusConn: [%d] markToSent: [%d]\r\n", actual_sensor_data.uptime, isBESSModbusConn, markToSent);
                printBESSData(actual_sensor_data);
            }
            if(resCode3 == Lit_configuration.size())
            {
                resCode3 = 0;
                noOfLitModbusAttempts = 0;

                printf("Probed Sensors, system uptime: [%ld] isLitModbusConn: [%d] markToSent: [%d]\r\n", actual_sensor_data.uptime, isLitModbusConn, markToSent);
                printLitData(actual_sensor_data);
            }
            if(resCode4 == MG_configuration.size())
            {
                resCode4 = 0;
                noOfMGModbusAttempts = 0;

                printf("Probed Sensors, system uptime: [%ld] isMGModbusConn: [%d] markToSent: [%d]\r\n", actual_sensor_data.uptime, isMGModbusConn, markToSent);
                printMGData(actual_sensor_data);
            }
// End of Section 8.3
//////////////////////////////////////////////////////////////////////////////////
            //mutex lock to push the new modbus data, required to avoid issues with the data send task
            mtx_access_sensor_data_db.lock();

            actual_sensor_data.uptime = difftime(timeNow,timeStarted);
            actual_sensor_data.timestamp = timeNow;

            memcpy(&(actual_sensor_data.battery) ,&latest_battery_data, sizeof(battery_data));

            mtx_curr_error.lock();
            curr_error = logValues(db, stmt, &actual_sensor_data, markToSent); 

            //////// call the sql query to insert
            mtx_access_sensor_data_db.unlock();

            if(curr_error.lineNo != -1)
            {
                mtx_error_msg.lock();
                sprintf(errorMsg, "Unable to log data in DB. SQL RC [%d], Line_No [%d]",curr_error.rc, curr_error.lineNo);
                mtx_curr_error.unlock();
                logErrorInDB();
            }
            else
            {
                mtx_curr_error.unlock();
            }
            markToSent = 0;
            memset(&latest_battery_data,0,sizeof(battery_data));
        }
    }
}
// End of Section 8
//////////////////////////////////////////////////////////////////////////////////

int main(void) 
{
    read_system_config();

    cout << "================  Modbus TCP Logger ===============" << endl;
    cout << "               Version 2.0 (April 2023)             " << endl << endl;
    cout << "----------------Current Configuration--------------" << endl;
    cout << "BMS Modbus IP address: " << sys_config.modbus_BMS_ip << endl;
    cout << "BMS Modbus Slave ID: " << sys_config.modbus_BMS_slave_id << endl;
    cout << "BESS Modbus IP address: " << sys_config.modbus_BESS_ip << endl;
    cout << "BESS Modbus Slave ID: " << sys_config.modbus_BESS_slave_id << endl;
    cout << "LiT Modbus IP address: " << sys_config.modbus_Lit_ip << endl;
    cout << "LiT Modbus Slave ID: " << sys_config.modbus_Lit_slave_id << endl;
    cout << "MG Modbus IP address: " << sys_config.modbus_MG_ip << endl;
    cout << "MG Modbus Slave ID: " << sys_config.modbus_MG_slave_id << endl;
    cout << "No of Modbus attempts allowed before reconnecting: " << sys_config.noOfModbusAttemptsAllowed << endl;
    cout << "Modbus data read interval: " << sys_config.modbus_data_read_interval << endl;
    cout << "MADS Authorization Bearer Token: " << sys_config.mads_auth_token << endl;
    cout << "MADS URL: " << sys_config.mads_url << endl;
    cout << "Normal packet sent interval: " << sys_config.normal_packet_sent_interval << endl; 
    cout << "No of attempts allowed to send data before system restart in normal mode: " << sys_config.normalNoOfInternetAttemptsAllowed << endl;
    cout << "Fast packet sent interval (logged data): " << sys_config.fast_packet_sent_interval << endl;
    cout << "No of attempts allowed to send data before system restart in fast mode: " << sys_config.fastNoOfInternetAttemptsAllowed << endl;
    cout << "Asset ID: " << sys_config.asset_id << endl;
    cout << "====================================================" << endl;
//////////////////////////////////////////////////////////////////////////////////
// Section 9
    mbBMS = modbus_new_tcp(sys_config.modbus_BMS_ip.c_str(), mbBMSPort);
    modbus_set_slave(mbBMS,sys_config.modbus_BMS_slave_id);

    mbBESS = modbus_new_tcp(sys_config.modbus_BESS_ip.c_str(), mbBESSPort);
    modbus_set_slave(mbBESS,sys_config.modbus_BESS_slave_id);

    mbLit = modbus_new_tcp(sys_config.modbus_Lit_ip.c_str(), mbLitPort);
    modbus_set_slave(mbLit,sys_config.modbus_Lit_slave_id);

    mbMG = modbus_new_tcp(sys_config.modbus_MG_ip.c_str(), mbMGPort);
    modbus_set_slave(mbMG,sys_config.modbus_MG_slave_id);

    if (modbus_connect(mbBMS) == -1) 
    {
        fprintf(stdout, "BMS Modbus connection failed: %s\n", modbus_strerror(errno));
        isBMSModbusConn = false;

        mtx_error_msg.lock();
        sprintf(errorMsg, "BMS Modbus connection failed: %s Line No %d", modbus_strerror(errno), __LINE__);
        logErrorInDB();
    }
    else
    {
        printf("Sucessful BMS Modbus connection.\n");
        isBMSModbusConn = true;
    }

    if (modbus_connect(mbBESS) == -1) 
    {
        fprintf(stdout, "BESS Modbus connection failed: %s\n", modbus_strerror(errno));
        isBESSModbusConn = false;

        mtx_error_msg.lock();
        sprintf(errorMsg, "BESS Modbus connection failed: %s Line No %d", modbus_strerror(errno), __LINE__);
        logErrorInDB();
    }
    else
    {
        printf("Sucessful BESS Modbus connection.\n");
        isBESSModbusConn = true;
    }

    if (modbus_connect(mbLit) == -1) 
    {
        fprintf(stdout, "Lit Modbus connection failed: %s\n", modbus_strerror(errno));
        isLitModbusConn = false;

        mtx_error_msg.lock();
        sprintf(errorMsg, "Lit Modbus connection failed: %s Line No %d", modbus_strerror(errno), __LINE__);
        logErrorInDB();
    }
    else
    {
        printf("Sucessful Lit Modbus connection.\n");
        isLitModbusConn = true;
    }

    if (modbus_connect(mbMG) == -1) 
    {
        fprintf(stdout, "MG Modbus connection failed: %s\n", modbus_strerror(errno));
        isMGModbusConn = false;

        mtx_error_msg.lock();
        sprintf(errorMsg, "MG Modbus connection failed: %s Line No %d", modbus_strerror(errno), __LINE__);
        logErrorInDB();
    }
    else
    {
        printf("Sucessful MG Modbus connection.\n");
        isMGModbusConn = true;
    }

    populateSensorConfiguration();
    populateBESSConfiguration();
    populateLitConfiguration();
    populateMGConfiguration();

    printf("Sensor Configuration populated\n\r");
// End of Section 9
//////////////////////////////////////////////////////////////////////////////////

    int result=sqlite3_open(db_data,&db);

    if (result != SQLITE_OK) 
    {
        printf("Failed to open database \n\r");
        sqlite3_close(db);

        mtx_error_msg.lock();
        sprintf(errorMsg, "Failed to open sensordata DB, Line No %d",  __LINE__);
        logErrorInDB();

        exit(1);
    }

    printf("Database %s opened\n\r",db_data);

    result=sqlite3_open(db_statusdata,&dbErr);

    if (result != SQLITE_OK) 
    {
        printf("Failed to open database db_statusdata\n\r");
        sqlite3_close(dbErr);

        mtx_error_msg.lock();
        sprintf(errorMsg, "Failed to open statusdata DB, Line No %d",  __LINE__);
        logErrorInDB();

        exit(1);
    }

    printf("Database %s opened\n\r",db_statusdata);

    timeStarted = time(NULL); 

    thread th1(sensorsProbing);
    thread th2(setMarkToSent);
    thread th3(sendSensorData);
    thread th4(createDBSpace);
    th1.join();
    th2.join();
    th3.join();
    th4.join();
}
