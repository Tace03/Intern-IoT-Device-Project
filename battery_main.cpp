//////////////////////////////////////////////////////////////////////////////////
// Author: Naina Gupta
// July'2021 - ModbusTCPLogger v1.0
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

// For modbus communication
modbus_t *mbBMS;
modbus_t *mbComAP;

// modbus port of the communiting device
int mbBMSPort = 1502;
int mbComAPPort = 502;
uint16_t sensor_value[2] ={0};
// For Checking if Modbus is connected for each device
static bool isBMSModbusConn = false;
static bool isComAPModbusConn = false;
static bool isInternetConn = false;
int noOfBMSModbusAttempts = 0;
int noOfComAPModbusAttempts = 0;
int noOfInternetAttempts = 0;
int noOfInternetAttemptsAllowed = 10;

static int markToSent = 0;

battery_data latest_battery_data;
battery_data current_battery_data;

sensor_data actual_sensor_data;
sensor_data db_sensor_data;

bool sendDataFaster = false;

std::vector<sensor_config*> sensor_configuration;
std::vector<sensor_config*> comap_configuration;

const char * db_data = "/home/pi/Desktop/modbustcplogger/db_sensordata.db";
sqlite3 * db;
sqlite3_stmt *stmt;

const char * db_statusdata = "/home/pi/Desktop/modbustcplogger/db_statusdata.db";
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

uint16_t resCode = 0;

// Code modified by SeowSK
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

//Used for Testing
httplib::Client cli("http://192.168.1.174:1234");

#define SLAVE_ID 1

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

void reconnectToComAPModbus()
{
    if(mbComAP!= NULL)
    {   
        modbus_close(mbComAP);
        modbus_free(mbComAP);

        mbComAP = modbus_new_tcp(sys_config.modbus_ComAP_ip.c_str(), mbComAPPort);
        modbus_set_slave(mbComAP, sys_config.modbus_ComAP_slave_id);

        if(mbComAP!= NULL)
        {
            if (modbus_connect(mbComAP) == -1) 
            {
                fprintf(stdout, "ComAP Modbus reConnection failed: %s\n", modbus_strerror(errno));
                isComAPModbusConn = false;
            }
            else
            {
                isComAPModbusConn = true;
            }
        }
        else
        {
            printf("Could not connect over ComAP MODBUS TCP. \r\n");
        }
    }
    else
    {
        printf("Do nothing to ComAP MODBUS, should come back later. \r\n");
    }
}
// End of modbus reconnection

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
    

/*
    sensor_configuration.push_back(new sensor_config(248,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_charging_relay)));
    sensor_configuration.push_back(new sensor_config(250,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_discharge_relay)));
    sensor_configuration.push_back(new sensor_config(252,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_positive_pump)));
    sensor_configuration.push_back(new sensor_config(254,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.primary_negative_pump)));

    sensor_configuration.push_back(new sensor_config(316,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.system_mode)));  		    // Added on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(318,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.system_alarm_status)));  	// Modified on 11 Aug 21
    
    sensor_configuration.push_back(new sensor_config(340,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.balancing_valve)));
    sensor_configuration.push_back(new sensor_config(342,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.positive_valve)));
    sensor_configuration.push_back(new sensor_config(344,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.negative_valve)));

    sensor_configuration.push_back(new sensor_config(398,reg_type::INPUT_REG,0.01f,&(latest_battery_data.state_of_charge)));
    
    sensor_configuration.push_back(new sensor_config(688,reg_type::INPUT_REG,1.0f,&(latest_battery_data.bcu_mode_status)));  		// Added on 11 Aug 21

    sensor_configuration.push_back(new sensor_config(690,reg_type::INPUT_REG,0.001f,&(latest_battery_data.bcu_voltage)));
    sensor_configuration.push_back(new sensor_config(692,reg_type::INPUT_REG,0.01f,&(latest_battery_data.bcu_current))); 
    sensor_configuration.push_back(new sensor_config(396,reg_type::INPUT_REG,0.01f,&(latest_battery_data.bcu_power))); 
    sensor_configuration.push_back(new sensor_config(696,reg_type::INPUT_REG,0.01f,&(latest_battery_data.bcu_state_of_charge)));

    sensor_configuration.push_back(new sensor_config(698,reg_type::INPUT_REG,0.01f,&(latest_battery_data.bcu_hydrogen_sensor))); 	// Added on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(700,reg_type::INPUT_REG,1.0f,&(latest_battery_data.bcu_leakage_sensor)));  	// Added on 11 Aug 21

    sensor_configuration.push_back(new sensor_config(702,reg_type::INPUT_REG,1.0f,&(latest_battery_data.smoke_sensor)));
    sensor_configuration.push_back(new sensor_config(704,reg_type::INPUT_REG,0.001f,&(latest_battery_data.bcu_ocv)));
    sensor_configuration.push_back(new sensor_config(708,reg_type::INPUT_REG,0.01f,&(latest_battery_data.bcu_positive_tank_temp)));
    sensor_configuration.push_back(new sensor_config(710,reg_type::INPUT_REG,0.01f,&(latest_battery_data.bcu_negative_tank_temp)));
    sensor_configuration.push_back(new sensor_config(716,reg_type::INPUT_REG,1.0f,&(latest_battery_data.positive_tank_high_level_float)));
    sensor_configuration.push_back(new sensor_config(718,reg_type::INPUT_REG,1.0f,&(latest_battery_data.negative_tank_high_level_float)));
    sensor_configuration.push_back(new sensor_config(720,reg_type::INPUT_REG,1.0f,&(latest_battery_data.positive_tank_low_level_float)));
    sensor_configuration.push_back(new sensor_config(722,reg_type::INPUT_REG,1.0f,&(latest_battery_data.negative_tank_low_level_float)));
    sensor_configuration.push_back(new sensor_config(816,reg_type::INPUT_REG,0.001f,&(latest_battery_data.primary_stack_voltage)));
    sensor_configuration.push_back(new sensor_config(818,reg_type::INPUT_REG,0.01f,&(latest_battery_data.primary_stack_current)));
    sensor_configuration.push_back(new sensor_config(826,reg_type::INPUT_REG,0.01f,&(latest_battery_data.primary_stack_positive_pressure_sensor)));
    sensor_configuration.push_back(new sensor_config(828,reg_type::INPUT_REG,0.01f,&(latest_battery_data.primary_stack_negative_pressure_sensor)));
    
    sensor_configuration.push_back(new sensor_config(830,reg_type::INPUT_REG,0.01f,&(latest_battery_data.positive_stack_pressure_delta)));  	//modified positive with primary on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(924,reg_type::INPUT_REG,0.01f,&(latest_battery_data.b1_primary_stack_pressure_delta)));  	// Added on 11 Aug 21   
    
    sensor_configuration.push_back(new sensor_config(1028,reg_type::INPUT_REG,0.01f,&(latest_battery_data.sensor_temp)));
    sensor_configuration.push_back(new sensor_config(1030,reg_type::INPUT_REG,0.01f,&(latest_battery_data.humidity)));
    
    sensor_configuration.push_back(new sensor_config(1184,reg_type::INPUT_REG,0.001f,&(latest_battery_data.pcs1_dc_volts))); 		    // Added on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(1186,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_dc_batt_current))); 	    // Added on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(1188,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_dc_inverter_power))); 	// Added on 11 Aug 21
    
    sensor_configuration.push_back(new sensor_config(1190,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_voltage)));
    sensor_configuration.push_back(new sensor_config(1192,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_current)));
    sensor_configuration.push_back(new sensor_config(1196,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_reactive_power)));
    sensor_configuration.push_back(new sensor_config(1198,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_load_power)));
    sensor_configuration.push_back(new sensor_config(1200,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_ac_supply_power)));

    sensor_configuration.push_back(new sensor_config(1202,reg_type::INPUT_REG,1.0f,&(latest_battery_data.pcs1_ac_out_status)));     // Added on 11 Aug 21 
    sensor_configuration.push_back(new sensor_config(1204,reg_type::INPUT_REG,1.0f,&(latest_battery_data.pcs1_fault_status)));      // Added on 11 Aug 21
    sensor_configuration.push_back(new sensor_config(1206,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1_fan_speed)));        // Added on 11 Aug 21

    sensor_configuration.push_back(new sensor_config(176,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.system0PVEnable)));
    sensor_configuration.push_back(new sensor_config(194,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.system0PVChargePower)));
    sensor_configuration.push_back(new sensor_config(196,reg_type::HOLDING_REG,0.01f,&(latest_battery_data.system0PVTotalPower)));
    sensor_configuration.push_back(new sensor_config(1218,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1InvFreq)));
    sensor_configuration.push_back(new sensor_config(1222,reg_type::INPUT_REG,0.01f,&(latest_battery_data.pcs1InternalTemperature)));
    */
}

void populateComAPConfiguration()
{
    sensor_configuration.push_back(new sensor_config(0,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.bcu_leakage_sensor)));
    sensor_configuration.push_back(new sensor_config(2,reg_type::HOLDING_REG,1.0f,&(latest_battery_data.positive_tank_high_level_float)));
}

void printSensorData(sensor_data data)
{
    // printf("bcu_leakage_sensor : [%f]\r\n",data.battery.bcu_leakage_sensor); // Added on 11 Aug 21
    printf("Uptime : [%ld]\r\n",data.uptime);
    printf("Timestamp : [%ld]\r\n",data.timestamp);

    printf("markToSent : [%d]\r\n",markToSent);
    
    printf("bcu_leakage_sensor : [%d]\r\n",data.battery.bcu_leakage_sensor);		// Added on 11 Aug 21
    printf("positive_tank_high_level_float : [%d]\r\n",data.battery.positive_tank_high_level_float);
    printf("negative_tank_high_level_float : [%d]\r\n",data.battery.negative_tank_high_level_float);
    printf("positive_tank_low_level_float : [%d]\r\n",data.battery.positive_tank_low_level_float);
    printf("negative_tank_low_level_float : [%d]\r\n",data.battery.negative_tank_low_level_float);
    printf("smoke_sensor : [%d]\r\n",data.battery.smoke_sensor);
    printf("bcu_voltage : [%f]\r\n",data.battery.bcu_voltage);
    printf("bcu_current : [%f]\r\n",data.battery.bcu_current);
    printf("bcu_ocv : [%f]\r\n",data.battery.bcu_ocv);
    printf("bcu_positive_tank_temp : [%f]\r\n",data.battery.bcu_positive_tank_temp);
    printf("bcu_negative_tank_temp : [%f]\r\n",data.battery.bcu_negative_tank_temp);
    printf("bcu_hydrogen_sensor : [%f]\r\n",data.battery.bcu_hydrogen_sensor);
    printf("humidity : [%f]\r\n",data.battery.humidity);
    printf("sensor_temp : [%f]\r\n",data.battery.sensor_temp);
    printf("primary_stack_positive_pressure_sensor : [%f]\r\n",data.battery.primary_stack_positive_pressure_sensor);
    printf("primary_stack_negative_pressure_sensor : [%f]\r\n",data.battery.primary_stack_negative_pressure_sensor);
    printf("primary_positive_pump : [%1.0f]\r\n",data.battery.primary_positive_pump);
    printf("primary_negative_pump : [%1.0f]\r\n",data.battery.primary_negative_pump);
    printf("positive_valve : [%1.0f]\r\n",data.battery.positive_valve);
    printf("negative_valve : [%1.0f]\r\n",data.battery.negative_valve);
    printf("balancing_valve : [%1.0f]\r\n",data.battery.balancing_valve);    
    printf("primary_charging_relay : [%1.0f]\r\n",data.battery.primary_charging_relay);
    printf("primary_discharge_relay : [%1.0f]\r\n",data.battery.primary_discharge_relay);
    printf("state_of_charge : [%f]\r\n",data.battery.bcu_state_of_charge);   printf("pcs1_dc_volts : [%f]\r\n",data.battery.pcs1_dc_volts); // Added on 11 Aug 21
    printf("pcs1_dc_batt_current : [%f]\r\n",data.battery.pcs1_dc_batt_current); 	    // Added on 11 Aug 21
    printf("pcs1_dc_inverter_power : [%f]\r\n",data.battery.pcs1_dc_inverter_power); 	// Added on 11 Aug 21

    printf("pcs1_voltage : [%f]\r\n",data.battery.pcs1_voltage);
    printf("pcs1_current : [%f]\r\n",data.battery.pcs1_current);
    printf("pcs1_reactive_power : [%f]\r\n",data.battery.pcs1_reactive_power);
    printf("pcs1_load_power : [%f]\r\n",data.battery.pcs1_load_power);
    printf("pcs1_ac_supply_power : [%f]\r\n",data.battery.pcs1_ac_supply_power);

/*
    printf("system_mode : [%1.0f]\r\n",data.battery.system_mode);                   // Added on 11 Aug 21
    printf("system_alarm_status : [%1.0f]\r\n",data.battery.system_alarm_status);   // Added on 11 Aug 21

    printf("bcu_mode_status : [%f]\r\n",data.battery.bcu_mode_status);              // Added on 11 Aug 21

    printf("bcu_power : [%f]\r\n",data.battery.bcu_power);
    printf("bcu_state_of_charge : [%f]\r\n",data.battery.bcu_state_of_charge);

    printf("primary_stack_voltage : [%f]\r\n",data.battery.primary_stack_voltage);
    printf("primary_stack_current : [%f]\r\n",data.battery.primary_stack_current);

    printf("positive_stack_pressure_delta : [%f]\r\n",data.battery.positive_stack_pressure_delta);      // Modified on 11 Aug 21
    printf("b1_primary_stack_pressure_delta : [%f]\r\n",data.battery.b1_primary_stack_pressure_delta);  // Added on 11 Aug 21

    printf("pcs1_dc_volts : [%f]\r\n",data.battery.pcs1_dc_volts);                      // Added on 11 Aug 21
    printf("pcs1_dc_batt_current : [%f]\r\n",data.battery.pcs1_dc_batt_current);        // Added on 11 Aug 21
    printf("pcs1_dc_inverter_power : [%f]\r\n",data.battery.pcs1_dc_inverter_power);    // Added on 11 Aug 21

    printf("pcs1_voltage : [%f]\r\n",data.battery.pcs1_voltage);
    printf("pcs1_current : [%f]\r\n",data.battery.pcs1_current);
    printf("pcs1_reactive_power : [%f]\r\n",data.battery.pcs1_reactive_power);
    printf("pcs1_load_power : [%f]\r\n",data.battery.pcs1_load_power);
    printf("pcs1_ac_supply_power : [%f]\r\n",data.battery.pcs1_ac_supply_power);

    printf("pcs1_ac_out_status : [%f]\r\n",data.battery.pcs1_ac_out_status);        // Added on 11 Aug 21
    printf("pcs1_fault_status : [%f]\r\n",data.battery.pcs1_fault_status);          // Added on 11 Aug 21
    printf("pcs1_fan_speed : [%f]\r\n",data.battery.pcs1_fan_speed);                // Added on 11 Aug 21
*/
}

void probeBMSSensors()
{
    while(1)
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
                    // printf(" sensor_value : [%d]\r\n",(int16_t)sensor_value[0]);
                    // *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                    // printf("*(current_reg->dest_float_ptr) : [%f]\r\n",*(current_reg->dest_float_ptr));
		            // printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                 
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
            if(resCode == sensor_configuration.size())
            {
                resCode = 0;
                noOfBMSModbusAttempts = 0;
                getCurrentTime();
               
                //mutex lock to push the new modbus data, required to avoid issues with the data send task
                mtx_access_sensor_data_db.lock();

                actual_sensor_data.uptime = difftime(timeNow,timeStarted);
                actual_sensor_data.timestamp = timeNow;

                memcpy(&(actual_sensor_data.battery) ,&latest_battery_data, sizeof(battery_data));
                printSensorData(actual_sensor_data);

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
                printf("Probed Sensors, system uptime: [%ld] isModbusConn: [%d] markToSent: [%d]\r\n", actual_sensor_data.uptime, isBMSModbusConn, markToSent);
                printSensorData(actual_sensor_data);
                markToSent = 0;
                memset(&latest_battery_data,0,sizeof(battery_data));
            }
        }
        else if(noOfBMSModbusAttempts < sys_config.noOfModbusAttemptsAllowed)
        {
            // Will be unlocked in mtx_error_msg
            mtx_error_msg.lock();
            sprintf(errorMsg, "Modbus Read Failed. Trying another attempt! Line No %d", __LINE__);
            logErrorInDB();

            printf("Modbus Read Failed. Trying another attempt!\r\n");
            noOfBMSModbusAttempts++;
        }  
        else
        {
            // we have reached the limit for no of attempts... sleep for a while and then try again
            mtx_error_msg.lock();
            sprintf(errorMsg, "Modbus might not be connected. Trying to reconnect! Line No %d", __LINE__);
            logErrorInDB();

            printf("Modbus is not connected. Trying to reconnect!\r\n");
            reconnectToBMSModbus();
            noOfBMSModbusAttempts = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.modbus_data_read_interval));
    }
}

void probeComAPSensors()
{
  while(1)
  {
        if(isComAPModbusConn)
        {
            // printf("Probing sensors to read modbus data\r\n");

            for(resCode = 0; resCode < sensor_configuration.size(); resCode++)
            {
                sensor_config* current_reg = sensor_configuration.at(resCode); 
                
                int addr = (current_reg->reg_offset);
                int readCode = 0;
                
                if(current_reg->regType == reg_type::HOLDING_REG)
                {
                    readCode = modbus_read_registers(mbComAP, addr , current_reg->noOfRegsToRead, sensor_value);
                }
                else 
                {
                    readCode = modbus_read_input_registers(mbComAP, addr , current_reg->noOfRegsToRead, sensor_value);
                }
                
                if(readCode == -1)
                {
                    printf("ERROR: %s\n", modbus_strerror(errno));
                    isComAPModbusConn = false;
                    break;
                }
                else
                {
                    // printf(" sensor_value : [%d]\r\n",(int16_t)sensor_value[0]);
                    // *(current_reg->dest_int32_ptr) = (int16_t)sensor_value[0];
                    // printf("*(current_reg->dest_float_ptr) : [%f]\r\n",*(current_reg->dest_float_ptr));
		            // printf("*(current_reg->dest_int32_ptr) : [%d]\r\n",*(current_reg->dest_int32_ptr));
                 
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
            if(resCode == sensor_configuration.size())
            {
                resCode = 0;
                noOfComAPModbusAttempts = 0;
                getCurrentTime();
               
                //mutex lock to push the new modbus data, required to avoid issues with the data send task
                mtx_access_sensor_data_db.lock();

                actual_sensor_data.uptime = difftime(timeNow,timeStarted);
                actual_sensor_data.timestamp = timeNow;

                memcpy(&(actual_sensor_data.battery) ,&latest_battery_data, sizeof(battery_data));
                printSensorData(actual_sensor_data);

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

                printf("Probed Sensors, system uptime: [%ld] isModbusConn: [%d] markToSent: [%d]\r\n", actual_sensor_data.uptime, isComAPModbusConn, markToSent);
                printSensorData(actual_sensor_data);
                markToSent = 0;
                memset(&latest_battery_data,0,sizeof(battery_data));
            }
        }
        else if(noOfComAPModbusAttempts < sys_config.noOfModbusAttemptsAllowed)
        {
            // Will be unlocked in mtx_error_msg
            mtx_error_msg.lock();
            sprintf(errorMsg, "Modbus Read Failed. Trying another attempt! Line No %d", __LINE__);
            logErrorInDB();

            printf("Modbus Read Failed. Trying another attempt!\r\n");
            noOfComAPModbusAttempts++;
        }  
        else
        {
            // we have reached the limit for no of attempts... sleep for a while and then try again
            mtx_error_msg.lock();
            sprintf(errorMsg, "Modbus might not be connected. Trying to reconnect! Line No %d", __LINE__);
            logErrorInDB();

            printf("Modbus is not connected. Trying to reconnect!\r\n");
            reconnectToComAPModbus();
            noOfComAPModbusAttempts = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sys_config.modbus_data_read_interval));
    }
}

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
// to send data, can ignore if task is not related to sending data.
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
	    int status_vft=0;
        int status_mads=0;
        if(db_sensor_data.isValid)
        {
        	encode_sensor_data_to_json(sendDataJSON, db_sensor_data, sys_config);
            // printf("JSON Str: %s\n", sendDataString);
            sendDataString = sendDataJSON.dump(4);
           	// printf("JSON Str: %s\n", sendDataString);
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
	    	// Duplicated Code for MADS
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
        }
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
	    // Duplication code block ends here
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

void read_system_config()
{
    json system_json_config;
    cout<<"Reading"<<endl;
    // read a JSON file
    std::ifstream i("/home/pi/Desktop/modbustcplogger/config.json");
    i >> system_json_config;
    
    string modbus_BMS_ip = system_json_config["modbus_BMS_ip"];
    string modbus_ComAP_ip = system_json_config["modbus_ComAP_ip"];
    string mads_auth_token = system_json_config["mads_auth_token"];
    string mads_url = system_json_config["mads_url"];

    sys_config.modbus_BMS_ip = modbus_BMS_ip;
    sys_config.modbus_BMS_slave_id = system_json_config["modbus_BMS_slave_id"];
    sys_config.modbus_ComAP_ip = modbus_ComAP_ip;
    sys_config.modbus_ComAP_slave_id = system_json_config["modbus_ComAP_slave_id"];
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

int main(void) {

    read_system_config();

    cout << "================  Modbus TCP Logger ===============" << endl;
    cout << "               Version 2.0 (April 2023)             " << endl << endl;
    cout << "----------------Current Configuration--------------" << endl;
    cout << "BMS Modbus IP address: " << sys_config.modbus_BMS_ip << endl;
    cout << "BMS Modbus Slave ID: " << sys_config.modbus_BMS_slave_id << endl;
    cout << "ComAP Modbus IP address: " << sys_config.modbus_ComAP_ip << endl;
    cout << "ComAP Modbus Slave ID: " << sys_config.modbus_ComAP_slave_id << endl;
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

    mbBMS = modbus_new_tcp(sys_config.modbus_BMS_ip.c_str(), mbBMSPort);
    modbus_set_slave(mbBMS,sys_config.modbus_BMS_slave_id);

    mbComAP = modbus_new_tcp(sys_config.modbus_ComAP_ip.c_str(), mbComAPPort);
    modbus_set_slave(mbComAP,sys_config.modbus_ComAP_slave_id);

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
        isBMSModbusConn = true;
    }

    if (modbus_connect(mbComAP) == -1) 
    {
        fprintf(stdout, "ComAP Modbus connection failed: %s\n", modbus_strerror(errno));
        isComAPModbusConn = false;

        mtx_error_msg.lock();
        sprintf(errorMsg, "ComAP Modbus connection failed: %s Line No %d", modbus_strerror(errno), __LINE__);
        logErrorInDB();
    }
    else
    {
        isComAPModbusConn = true;
    }
  
    populateSensorConfiguration();

    printf("Sensor Configuration populated\n\r");
    
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

    thread th1(probeBMSSensors);
    thread th5(probeComAPSensors); 
    thread th2(setMarkToSent);
    thread th3(sendSensorData);
    thread th4(createDBSpace);
    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();
}
