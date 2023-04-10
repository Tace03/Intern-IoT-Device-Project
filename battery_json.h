//////////////////////////////////////////////////////////////////////////////////
// Author: Naina Gupta
// July'2021 - ModbusTCPLogger v1.0
//////////////////////////////////////////////////////////////////////////////////

#ifndef BATTERY_JSON_H
#define BATTERY_JSON_H

#include <stdint.h>
#include <string>

#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;


//using nlohmann::json; /////// download library in ubuntu

typedef enum __attribute__((__packed__)) _packet_status 
{
    ST_ERROR=100,
    ST_DATA_NOT_AVAILABLE,
    ST_SENSOR_NOT_AVAILABLE,
    ST_DEVICE_NOT_AVAILABLE,
    ST_OK
} packet_status;


typedef struct _system_config
{
    std::string modbus_BMS_ip;
    int modbus_BMS_slave_id;
    std::string modbus_ComAP_ip;
    int modbus_ComAP_slave_id;
    std::string modbus_ComAP2_ip;
    int modbus_ComAP2_slave_id;
    int noOfModbusAttemptsAllowed;
    int modbus_data_read_interval;
    std::string mads_auth_token;
    std::string mads_url;
    int normalNoOfInternetAttemptsAllowed;
    int normal_packet_sent_interval;
    int fastNoOfInternetAttemptsAllowed;
    int fast_packet_sent_interval;
    int asset_id;
} system_config;

//////////////////////////////////////////////////////////////////////////////////
// Section 11
typedef struct __attribute__((__packed__)) _battery_data 
{
    float primary_charging_relay;               //prcr
    float primary_discharge_relay;              //prdr
    float primary_positive_pump;                //prpp
    float primary_negative_pump;                //prnp
    float system_mode;                          //264 # Added on 11 Aug 21
    float system_alarm_status;                  //266 # Added on 11 Aug 21
    float balancing_valve;                      //bv
    float positive_valve;                       //pv
    float negative_valve;                       //nv
    float state_of_charge;
    float bcu_voltage;
    float bcu_current;
    float bcu_power;
    float bcu_state_of_charge;
    float bcu_mode_status;                      //608 # Added on 11 Aug 21
    float bcu_hydrogen_sensor;                  //618 # Added on 11 Aug 21
    int bcu_leakage_sensor;                     //620 # Added on 11 Aug 21
    int smoke_sensor;
    float bcu_ocv;
    float bcu_positive_tank_temp;
    float bcu_negative_tank_temp;
    int positive_tank_high_level_float;         //pthlf
    int negative_tank_high_level_float;         //nthlf
    int positive_tank_low_level_float;          //ptllf
    int negative_tank_low_level_float;          //ntllf
    float primary_stack_voltage;
    float primary_stack_current;
    float primary_stack_positive_pressure_sensor; 
    float primary_stack_negative_pressure_sensor;
    // float b0_primary_stack_pressure_delta;
    float positive_stack_pressure_delta;
    float b1_primary_stack_pressure_delta;

    float sensor_temp;
    float humidity; 
    float pcs1_dc_volts;          //1102 # Added on 11 Aug 21
    float pcs1_dc_batt_current;   //1104 # Added on 11 Aug 21
    float pcs1_dc_inverter_power; //1106 # Added on 11 Aug 21
    float pcs1_voltage;
    float pcs1_current;
    float pcs1_reactive_power;
    float pcs1_load_power;
    float pcs1_ac_supply_power;
    float pcs1_ac_out_status;     //1120 # Added on 11 Aug 21
    float pcs1_fault_status;      //1122 # Added on 11 Aug 21
    float pcs1_fan_speed;         //1124 # Added on 11 Aug 21

    float system0PVEnable;
    float system0PVChargePower;
    float system0PVTotalPower;
    float pcs1InvFreq;
    float pcs1InternalTemperature;

    float bess1_voltage_gain;             // Added by SeowSK
    float bess1_voltage_int;              // Added by SeowSK

    float bess2_voltage_gain;             // Added by SeowSK
    float bess2_voltage_int;              // Added by SeowSK
} battery_data;
// End of Section 11
//////////////////////////////////////////////////////////////////////////////////

typedef struct __attribute__((__packed__)) _sensor_data
{
    long uptime;
    long timestamp;                 // timestamp is unix timestamp
    battery_data battery;
    bool isValid = false;
} sensor_data;


typedef enum __attribute__((__packed__)) _data_type 
{
    DT_UINT16,
    DT_INT16,
    DT_UINT32,
    DT_INT32,
    DT_FLOAT,
    DT_BOOL
} data_type;

typedef enum __attribute__((__packed__)) _reg_type 
{
    HOLDING_REG,
    INPUT_REG
} reg_type;

typedef struct __attribute__((__packed__)) _error_data
{
    int lineNo = -1;
    int rc = -1;  
} error_data;

class sensor_config
{
    public: 
        int reg_offset;
        int noOfRegsToRead;
        data_type valueType;
        reg_type regType;
        float scale;
        int32_t *dest_int32_ptr; //int32_t
        float *dest_float_ptr;
        bool *dest_bool_ptr;

        sensor_config(){}

        sensor_config(int reg_offset, reg_type regType, float scale, int32_t *dest_int32_ptr, int noOfReg=1)//ptr int32
        {
            this->reg_offset = reg_offset;
            this->noOfRegsToRead = noOfReg;
            this->valueType= data_type::DT_INT32;
            this->regType = regType;
            this->scale = scale;
            this->dest_int32_ptr = dest_int32_ptr;
            this->dest_float_ptr = NULL;
            this->dest_bool_ptr = NULL;
        }

        sensor_config(int reg_offset, reg_type regType, float scale, bool *dest_bool_ptr)
        {
            this->reg_offset = reg_offset;
            this->noOfRegsToRead = 2;
            this->valueType= data_type::DT_BOOL;
            this->regType = regType;
            this->scale = scale;
            this->dest_bool_ptr = dest_bool_ptr;
            this->dest_float_ptr = NULL;
            this->dest_int32_ptr = NULL;
        }

        sensor_config(int reg_offset, reg_type regType, float scale, float *dest_float_ptr, int noOfReg=1)
        {
            this->reg_offset = reg_offset;
            this->noOfRegsToRead = noOfReg;
            this->valueType= data_type::DT_FLOAT;
            this->regType = regType;
            this->scale = scale;
            if (this->valueType == DT_INT32)
            {
                this->dest_int32_ptr = (int *) dest_float_ptr;
            }
            else if ( this->valueType == DT_FLOAT)
            {
                this->dest_float_ptr = dest_float_ptr;
            }
            // this->dest_int32_ptr = NULL;
            // this->dest_float_ptr = dest_float_ptr;
            this->dest_bool_ptr = NULL;
        }
};

void encode_sensor_data_to_json(json& out_data, const sensor_data& inp_data, system_config& sys_config);
size_t encode_sensor_data(sensor_data inp_data, uint8_t* encoded_buffer, int buffer_size);

#endif
