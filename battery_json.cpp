///////////////////////////////////////////////////////////////////////////////////
// Author: Naina Gupta
// July'2021 - ModbusTCPLogger v1.0
//////////////////////////////////////////////////////////////////////////////////

#include "battery_json.h"

float get_charge_level(float soc)
{
	float cl = (soc - 10) * 100/78.5;
	if (cl < 0)
		cl = 0;
	if (cl > 100)
		cl = 100;
	printf("Charging level : [%f]\r\n", cl);
	return cl;
}

float state_of_charge_config(float soc)
{
	float sc = (soc*100)/88.5;
	if (sc < 0)
		sc = 0;
	if (sc > 100)
		sc = 100;
	return sc;
}

//////////////////////////////////////////////////////////////////////////////////
// Section 10
// This is just converting to JSON, do not matter where it come from
void encode_sensor_data_to_json(json& out_data, const sensor_data& inp_data, system_config& sys_config)
{
	out_data["timestamp"] = inp_data.timestamp;
	out_data["uptime"] = inp_data.uptime;

	out_data["assets_params"]["asset_id"] = sys_config.asset_id;

	out_data["assets_params"]["prcr"] = inp_data.battery.primary_charging_relay;
	out_data["assets_params"]["prdr"] = inp_data.battery.primary_discharge_relay;
	out_data["assets_params"]["prpp"] = inp_data.battery.primary_positive_pump;
	out_data["assets_params"]["prnp"] = inp_data.battery.primary_negative_pump;

	out_data["assets_params"]["sm"] = inp_data.battery.system_mode;									// Added on 11 Aug 21
	out_data["assets_params"]["sas"] = inp_data.battery.system_alarm_status;						// Added on 11 Aug 21

	out_data["assets_params"]["bv"] = inp_data.battery.balancing_valve;
	out_data["assets_params"]["pv"] = inp_data.battery.positive_valve;
	out_data["assets_params"]["nv"] = inp_data.battery.negative_valve;
	out_data["assets_params"]["soc"] = (float)inp_data.battery.state_of_charge;
	// out_data["assets_params"]["soc"] = state_of_charge_config(get_charge_level((float)inp_data.battery.state_of_charge));
	out_data["assets_params"]["bcmst"] = inp_data.battery.bcu_mode_status;							// Added on 11 Aug 21
	out_data["assets_params"]["hs"] = (float)inp_data.battery.bcu_hydrogen_sensor;					// Added on 11 Aug 21
	out_data["assets_params"]["ls"] = inp_data.battery.bcu_leakage_sensor;							// Added on 11 Aug 21

	out_data["assets_params"]["bvolt"] = (float)inp_data.battery.bcu_voltage;
	out_data["assets_params"]["bcurr"] = (float)inp_data.battery.bcu_current;
	out_data["assets_params"]["bpow"] = (float)inp_data.battery.bcu_power;

	out_data["assets_params"]["bsoc"] = (float)inp_data.battery.bcu_state_of_charge;
	out_data["assets_params"]["ss"] = inp_data.battery.smoke_sensor;
	out_data["assets_params"]["bocv"] = (float)inp_data.battery.bcu_ocv;
	out_data["assets_params"]["bptt"] = (float)inp_data.battery.bcu_positive_tank_temp;
	out_data["assets_params"]["bntt"] = (float)inp_data.battery.bcu_negative_tank_temp;
	out_data["assets_params"]["pthlf"] = inp_data.battery.positive_tank_high_level_float;
	out_data["assets_params"]["nthlf"] = inp_data.battery.negative_tank_high_level_float;
	out_data["assets_params"]["ptllf"] = inp_data.battery.positive_tank_low_level_float;
	out_data["assets_params"]["ntllf"] = inp_data.battery.negative_tank_low_level_float;
	out_data["assets_params"]["prvolt"] = (float)inp_data.battery.primary_stack_voltage;
	out_data["assets_params"]["prcurr"] = (float)inp_data.battery.primary_stack_current;
	out_data["assets_params"]["prspps"] = (float)inp_data.battery.primary_stack_positive_pressure_sensor;
	out_data["assets_params"]["prsnps"] = (float)inp_data.battery.primary_stack_negative_pressure_sensor;
	out_data["assets_params"]["pspd"] = (float)inp_data.battery.positive_stack_pressure_delta;		//Modified on 11 Aug 21
	out_data["assets_params"]["pspd1"] = (float)inp_data.battery.b1_primary_stack_pressure_delta;	//Added on 11 Aug 21
	out_data["assets_params"]["temp"] = (float)inp_data.battery.sensor_temp;
	out_data["assets_params"]["hum"] = (float)inp_data.battery.humidity;

	out_data["assets_params"]["p1dcv"] = (float)inp_data.battery.pcs1_dc_volts;						// Added on 11 Aug 21
	out_data["assets_params"]["p1dbc"] = (float)inp_data.battery.pcs1_dc_batt_current;				// Added on 11 Aug 21
	out_data["assets_params"]["p1invpow"] = (float)inp_data.battery.pcs1_dc_inverter_power;			// Added on 11 Aug 21

	out_data["assets_params"]["pcvolt"] = (float)inp_data.battery.pcs1_voltage;
	out_data["assets_params"]["pccurr"] = (float)inp_data.battery.pcs1_current;
	out_data["assets_params"]["pcrpow"] = (float)inp_data.battery.pcs1_reactive_power;
	out_data["assets_params"]["pclpow"] = (float)inp_data.battery.pcs1_load_power;
	out_data["assets_params"]["pcacpow"] = (float)inp_data.battery.pcs1_ac_supply_power;

	out_data["assets_params"]["p1acost"] = inp_data.battery.pcs1_ac_out_status;						// Added on 11 Aug 21
	out_data["assets_params"]["p1fst"] = inp_data.battery.pcs1_fault_status;						// Added on 11 Aug 21
	out_data["assets_params"]["p1fsp"] = (float)inp_data.battery.pcs1_fan_speed;					// Added on 11 Aug 21

	out_data["assets_params"]["charge_level"] = get_charge_level((float)inp_data.battery.state_of_charge);		// Added on 16 Aug 21

	out_data["assets_params"]["PVStat"] = (float)inp_data.battery.system0PVEnable;
	out_data["assets_params"]["PVBP"] = (float)inp_data.battery.system0PVChargePower;
	out_data["assets_params"]["PVSP"] = (float)inp_data.battery.system0PVTotalPower;
	out_data["assets_params"]["Freq"] = (float)inp_data.battery.pcs1InvFreq;
	out_data["assets_params"]["InvTemp"] = (float)inp_data.battery.pcs1InternalTemperature;

	out_data["comap_params"]["voltg"] = (float)inp_data.battery.bess1_voltage_gain;					// Added by SeowSK
	out_data["comap_params"]["voltint"] = (float)inp_data.battery.bess1_voltage_int;					// Added by SeowSK

	out_data["comap2_params"]["voltg"] = (float)inp_data.battery.bess2_voltage_gain;					// Added by SeowSK
	out_data["comap2_params"]["voltint"] = (float)inp_data.battery.bess2_voltage_int;					// Added by SeowSK
}
// End of Section 10
//////////////////////////////////////////////////////////////////////////////////