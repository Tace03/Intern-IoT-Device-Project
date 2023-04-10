# Author: Naina Gupta
July'2021 - ModbusTCPLogger v1.0

Steps for code deployment
------------------------------------------------------------------------

I) Perform following on the remote raspberry pi machine
	
1) Create a .desktop file => to automatically start the application on pi reboot  
	&emsp;a) mkdir /home/pi/.config/autostart  
	&emsp;b) nano /home/pi/.config/autostart/modbustcplogger.desktop  
	&emsp;c) Add the following contents verbatim in the file  
	&emsp&emsp;   [Desktop Entry]  
	&emsp&emsp;   Type = Application  
	&emsp&emsp;   Name = ModbusTCPLogger  
	&emsp&emsp;   Exec = xterm -hold -e '/home/pi/Desktop/modbustcplogger/modbustcplogger'  
	&emsp;e) Exit and save the above file  

	
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

# Notes SeowSK
Documentation: Written by SeowSK

 1) Adding Multiple Servers (For data migration):  
      1.1) Add a namespace in "Section 1".  
          - Each namespace represent a server  
      1.2) Declare a new int variable for status checking in "Section 2.1"  
      1.3) Copy "Section 2.2" and "Section 2.3" and paste it on the same indent  
          - Modify all status variable to what you have declared on (1.2)  

 2) Removing transmission to MADS server  
      2.1) Remove namespace containing MADS URL in "Section 1"  
      2.2) Remove code block "Duplicated code for MADS" up to "End of duplicated code for MADS"  
          - There are two code block as of 6th April 2023  

 3) Adding a modbus connection  
      3.1) Add a modbus_t, port, bool, and noOfAttempts for your new modbus in "Section 3"  
      3.2) Add Sensor configuration for your new modbus in "Section 3"  
      3.3) Add a reconnection function in "Section 4" (Copy the block of function)  
          - Modify your variables to what was declared in (3.1) and (3.2)  
      3.4) Declare all of your registers in a new populateConfiguration function in "Section 5"  
          - (Reason) Reading of sensors is dependent on sensor_config vector size  
      3.5) Add a new function to print your data of the new modbus connection in "Section 6"  
          - This is optional  
          - Adding a new function is not required, but a modbus connection may fail.  
          - Wants to stop the modbus that is not connected from printing.  
      3.6) Add a new probe function in "Section 7"  
          - Modify all variables declared on (3.1)  
          - This function will read from modbus and store in memory (not database)  
      3.7) Add your probe function in "Section 8"  
          - This function will store the read values into database for sending to server  
          - Remember to add checking for connection and vector size for your new modbus  
          - Mainly Section 8.1, 8.2, and 8.3  
      3.8) Add your modbus connection, reconnection, and sensor population in "Section 9"  
      3.9) Add a new double array of your new modbus variables in "Section 10", battery_json.cpp  
          - This section is only used to convert data to JSON before sending to gateway.  
      3.10) Declare the variables of your new modbus connection in "Section 11", battery_json.h  
          - This section is used to setup your <sensor_config> vector  
      3.11) Add variable name to insert and a ? to values in "Section 12", sql_queries.cpp  
          - This will allow the insertion of data after reading from probeSensors.  
      3.12) Add a code chunk from "Section 13", sql_queries.cpp for your newly added variable in your new modbus to "Section 13"  
          - Note that the variable type could be double or int.  
      3.13) Add a fetch for your variable in your new modbus in "Section 14", sql_queries.cpp  
          - Note that the order of your fetch is important.  
          - This is used to retrieve data from the database.  
      3.14) Use a tool to insert columns into the database. You can use DB Browser (SQLite)  