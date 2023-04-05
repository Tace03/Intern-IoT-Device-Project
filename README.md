//////////////////////////////////////////////////////////////////////////////////
// Author: Naina Gupta
// July'2021 - ModbusTCPLogger v1.0
//////////////////////////////////////////////////////////////////////////////////

Steps for code deployment
------------------------------------------------------------------------

I) Perform following on the remote raspberry pi machine
	
1) Create a .desktop file => to automatically start the application on pi reboot
	a) mkdir /home/pi/.config/autostart
	b) nano /home/pi/.config/autostart/modbustcplogger.desktop
	c) Add the following contents verbatim in the file
	   [Desktop Entry]
	   Type = Application
	   Name = ModbusTCPLogger
	   Exec = xterm -hold -e '/home/pi/Desktop/modbustcplogger/modbustcplogger'
	e) Exit and save the above file

	
2) 	Create directory and prepare pi for code execution
	a) cd Desktop
	b) Run mkdir modbustcplogger
	c) Run sudo apt-get install libmodbus-dev
	d) Run sudo apt-get install libsqlite3-dev
	e) Run sudo apt-get install sqlite3
	f) Run sudo apt-get install nlohmann-json-dev
	g) Run sudo apt-get install libssl-dev
	
	
II) Perform following on a local raspberry pi machine

1) Compiling the code 
	a) cd Desktop
	b) Run mkdir modbustcplogger, cd modbustcplogger and copy the code inside this directory.
	c) Run sudo apt-get install libmodbus-dev
	d) Run sudo apt-get install libsqlite3-dev
	e) Run sudo apt-get install sqlite3
	f) Run sudo apt-get install nlohmann-json-dev
	g) Run sudo apt-get install libssl-dev
	h) Run make command to compile the code
	
2) Creating database files for deployment
	a) Run sh ./create_database.sh command to create sensordata and statusdata databases.
	
3) Modify config.json according to current battery (Do not change the format, only modify the values)
   Normal Mode -> when internet is working fine
   Fast Mode -> when internet might have been turned off, and there is previous data records which needs to be sent to the server
	a) asset_id => Battery Asset ID
	b) fastNoOfInternetAttemptsAllowed => Indicates how many attempts for sending the packet to the server are allowed in fast mode before restarting
	c) fast_packet_sent_interval => Indicates the packet sent interval in fast mode
	d) mads_auth_token => MADS platform access token (Go to the battery details tab on MADS for this value)
	e) mads_url => Copy paste the MADS url path after https://datakrewtech.com (Go to the battery details tab on MADS for this value)
	f) modbus_data_read_interval => Indicates register read interval from MODBUS
	g) modbus_ip => MODBUS IP address
	h) modbus_slave_id => MODBUS slave id
	i) noOfModbusAttemptsAllowed => Indicates no of modbus attempts allowed before trying reconnection to modbus
	j) normalNoOfInternetAttemptsAllowed => Indicates how many attempts for sending the packet to the server are allowed in normal mode before restarting
	k) normal_packet_sent_interval => Indicates the packet sent interval in normal mode
	
4) Send files to remote machine for execution
	a) scp -P <PortNo> ./modbustcplogger pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger
	b) scp -P <PortNo> ./db_sensordata.db pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger
	c) scp -P <PortNo> ./db_statusdata.db pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger
	d) scp -P <PortNo> ./config.json pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger
	
III) Ensure MADS platform is setup and mapping is correct
	Parameter Mapping
	prcr -> primary_charging_relay
	prdr -> primary_discharge_relay
	prpp -> primary_positive_pump
	prnp -> primary_negative_pump
	bv -> balancing_valve
	pv -> positive_valve
	nv -> negative_valve
	soc -> state_of_charge
	bvolt -> bcu_voltage
	bcurr -> bcu_current
	bpow -> bcu_power
	bsoc -> bcu_state_of_charge
	ss -> smoke_sensor
	bocv -> bcu_ocv
	bptt -> bcu_positive_tank_temp
	bntt -> bcu_negative_tank_temp
	pthlf -> positive_tank_high_level_float
	nthlf -> negative_tank_high_level_float
	ptllf -> positive_tank_low_level_float
	ntllf -> negative_tank_low_level_float
	prvolt -> primary_stack_voltage
	prcurr -> primary_stack_current
	prspps -> primary_stack_positive_pressure_sensor
	prsnps -> primary_stack_negative_pressure_sensor
	pspd -> positive_stack_pressure_delta
	temp -> sensor_temp
	hum -> humiditiy
	pcvolt -> pcs1_voltage
	pccurr -> pcs1_current
	pcrpow -> pcs1_reactive_power
	pclpow -> pcs1_load_power
	pcacpow -> pcs1_ac_supply_power
	
IV) Restart remote raspberry pi machine to apply changes and execute the code (verify the code is executing after boot-up)


