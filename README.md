# Original Author: Naina Gupta
July'2021 - ModbusTCPLogger v1.0
## Contributing Author: Sin Kiat, Saravanan

Steps for code deployment
------------------------------------------------------------------------

Steps to deploy to linux machine.
- The following steps will use a Raspberry PI machine as examples.

**1)  Auto Start Up of device**  

**Method a):**  
```
mkdir /home/pi/.config/autostart  
```
```
nano /home/pi/.config/autostart/modbustcplogger.desktop
```
Adding the following content into the file.
```
[Desktop Entry]  
Type = Application  
Name = ModbusTCPLogger  
Exec = xterm -hold -e '/home/pi/Desktop/modbustcplogger/modbustcplogger'  
```
```
ctrl+s  
ctrl+x  
```
**Method b):**  
Check for rc-local. All linux system have rc-local service, thus check for the file location or rc.local
```
systemctl status rc-local
```
Get the file location of rc.local and modify it. The file may or may not exist but the executed directory can be found int the above status command. Modify the below directory to that of your system.
```
sudo nano /etc/rc.local
```
Copy and paste the following content into the file.
```
#!/bin/sh -e
#
# rc.local
#
# This script is executed at the end of each multiuser runlevel.
# Make sure that the script will "exit 0" on success or any other
# value on error.
#
# In order to enable or disable this script just change the execution
# bits.
#
# By default this script does nothing.

# Print the IP address
_IP=$(hostname -I) || true
if [ "$_IP" ]; then
  printf "My IP address is %s\n" "$_IP"
fi
sleep 10
/home/pi/Desktop/modbustcplogger/start.sh
exit 0
```
Modify the file to be executatble
```
sudo chmod +x /etc/rc.local
```
The startup script in the directory copied from the above file content.
```
sudo nano /home/pi/Desktop/modbustcplogger/start.sh
```
Copy and paste the following content into the file.
```
sudo tmux new-session -d -s start 'sudo /home/pi/Desktop/modbustcplogger/modbustcplogger'
sudo tmux set-option -t start:0 remain-on-exit
```
Modify the file to be executable
```
sudo chmod +x /home/pi/Desktop/modbustcplogger/start.sh
```

**2) 	Create directory and prepare your linux machine to compile and execute the program.**  
```
cd Desktop
```
```
mkdir modbustcplogger
```
```
sudo apt-get install libmodbus-dev
```
```
sudo apt-get install libsqlite3-dev
```
```
sudo apt-get install sqlite3
```
```
sudo apt-get install nlohmann-json-dev  
```
```
sudo apt-get install libssl-dev  
```	
```
sudo apt-get install tmux  
```	
	
**3)  Compiling the program**  

Clear previous build before you rebuild the project by running
```
make clean
```
Compile your project.
```
make
```	
**4)  Creating database files for deployment**  

Run the following command to create or recreate sensordata and statusdata databases. 
```
sh ./create_database.sh
```  
	
**5)  Modify config.json according to current battery (Do not change the format, only modify the values)**  
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
	
**6) Send files to remote machine for execution**  
	a) scp -P <PortNo> ./modbustcplogger pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger  
	b) scp -P <PortNo> ./db_sensordata.db pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger  
	c) scp -P <PortNo> ./db_statusdata.db pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger  
	d) scp -P <PortNo> ./config.json pi@<Remote-IP-Address>:/home/pi/Desktop/modbustcplogger  
	
&emsp;Ensure MADS platform is setup and mapping is correct  
	&emsp;&emsp;Parameter Mapping  
	&emsp;&emsp;prcr -> primary_charging_relay  
	&emsp;&emsp;prdr -> primary_discharge_relay  
	&emsp;&emsp;prpp -> primary_positive_pump  
	&emsp;&emsp;prnp -> primary_negative_pump  
	&emsp;&emsp;bv -> balancing_valve  
	&emsp;&emsp;pv -> positive_valve  
	&emsp;&emsp;nv -> negative_valve  
	&emsp;&emsp;soc -> state_of_charge  
	&emsp;&emsp;bvolt -> bcu_voltage  
	&emsp;&emsp;bcurr -> bcu_current  
	&emsp;&emsp;bpow -> bcu_power  
	&emsp;&emsp;bsoc -> bcu_state_of_charge  
	&emsp;&emsp;ss -> smoke_sensor  
	&emsp;&emsp;bocv -> bcu_ocv  
	&emsp;&emsp;bptt -> bcu_positive_tank_temp  
	&emsp;&emsp;bntt -> bcu_negative_tank_temp  
	&emsp;&emsp;pthlf -> positive_tank_high_level_float  
	&emsp;&emsp;nthlf -> negative_tank_high_level_float  
	&emsp;&emsp;ptllf -> positive_tank_low_level_float  
	&emsp;&emsp;ntllf -> negative_tank_low_level_float  
	&emsp;&emsp;prvolt -> primary_stack_voltage  
	&emsp;&emsp;prcurr -> primary_stack_current  
	&emsp;&emsp;prspps -> primary_stack_positive_pressure_sensor  
	&emsp;&emsp;prsnps -> primary_stack_negative_pressure_sensor  
	&emsp;&emsp;pspd -> positive_stack_pressure_delta  
	&emsp;&emsp;temp -> sensor_temp  
	&emsp;&emsp;hum -> humiditiy  
	&emsp;&emsp;pcvolt -> pcs1_voltage  
	&emsp;&emsp;pccurr -> pcs1_current  
	&emsp;&emsp;pcrpow -> pcs1_reactive_power  
	&emsp;&emsp;pclpow -> pcs1_load_power  
	&emsp;&emsp;pcacpow -> pcs1_ac_supply_power  

**7)  Update system time**
```
sudo apt-get install ntp
```
```
sudo nano /etc/ntp.conf
```
Copy and paste the following content and replace the default server references in the file.
```
server 0.sg.pool.ntp.org
server 1.sg.pool.ntp.org
server 2.sg.pool.ntp.org
server 3.sg.pool.ntp.org
```
Restart the NTP
```
sudo service ntp restart
```
Check for the status
```
sudo service ntp status
```

**8)  Restart remote raspberry pi machine to apply changes and execute the code (verify the code is executing after boot-up)**  

**9)  Common Errors**  
&emsp;**a) std::invalid argument**
- There is 3 lines of code within battery_main.cpp that reference to a static URL. Those url could be invalid.
```
std::ifstream i("/home/pi/Desktop/modbustcplogger/config.json");
const char * db_data = "/home/pi/Desktop/modbustcplogger/db_sensordata.db";
const char * db_statusdata = "/home/pi/Desktop/modbustcplogger/db_statusdata.db";
```
&emsp;**b) <modbus.h> not found.**
- Open up ```Makefile``` and modify the following line.
```
INC = -I/usr/include/modbus 
```
or
```
INC = -I /usr/include/modbus 
```

# SeowSK notes for modifying the files
Documentation: Written by SeowSK

 1) Adding Multiple Servers (For data migration):  
      1.1) Add a namespace in "Section 1".  
          &emsp;&emsp;- Each namespace represent a server  
      1.2) Declare a new int variable for status checking in "Section 2.1"  
      1.3) Copy "Section 2.2" and "Section 2.3" and paste it on the same indent  
          &emsp;&emsp;- Modify all status variable to what you have declared on (1.2)  

 2) Removing transmission to MADS server  
      2.1) Remove namespace containing MADS URL in "Section 1"  
      2.2) Remove code block "Duplicated code for MADS" up to "End of duplicated code for MADS"  
          &emsp;&emsp;- There are two code block as of 6th April 2023  

 3) Adding a modbus connection  
      3.1) Add a modbus_t, port, bool, and noOfAttempts for your new modbus in "Section 3"  
      3.2) Add Sensor configuration for your new modbus in "Section 3"  
      3.3) Add a reconnection function in "Section 4" (Copy the block of function)  
          &emsp;&emsp;- Modify your variables to what was declared in (3.1) and (3.2)  
      3.4) Declare all of your registers in a new populateConfiguration function in "Section 5"  
          &emsp;&emsp;- (Reason) Reading of sensors is dependent on sensor_config vector size  
      3.5) Add a new function to print your data of the new modbus connection in "Section 6"  
          &emsp;&emsp;- This is optional  
          &emsp;&emsp;- Adding a new function is not required, but a modbus connection may fail.  
          &emsp;&emsp;- Wants to stop the modbus that is not connected from printing.  
      3.6) Add a new probe function in "Section 7"  
          &emsp;&emsp;- Modify all variables declared on (3.1)  
          &emsp;&emsp;- This function will read from modbus and store in memory (not database)  
      3.7) Add your probe function in "Section 8"  
          &emsp;&emsp;- This function will store the read values into database for sending to server  
          &emsp;&emsp;- Remember to add checking for connection and vector size for your new modbus  
          &emsp;&emsp;- Mainly Section 8.1, 8.2, and 8.3  
      3.8) Add your modbus connection, reconnection, and sensor population in "Section 9"  
      3.9) Add a new double array of your new modbus variables in "Section 10", battery_json.cpp  
          &emsp;&emsp;- This section is only used to convert data to JSON before sending to gateway.  
      3.10) Declare the variables of your new modbus connection in "Section 11", battery_json.h  
          &emsp;&emsp;- This section is used to setup your <sensor_config> vector  
      3.11) Add variable name to insert and a ? to values in "Section 12", sql_queries.cpp  
          &emsp;&emsp;- This will allow the insertion of data after reading from probeSensors.  
      3.12) Add a code chunk from "Section 13", sql_queries.cpp for your newly added variable in your new modbus to "Section 13"  
          &emsp;&emsp;- Note that the variable type could be double or int.  
      3.13) Add a fetch for your variable in your new modbus in "Section 14", sql_queries.cpp  
          &emsp;&emsp;- Note that the order of your fetch is important.  
          &emsp;&emsp;- This is used to retrieve data from the database.  
      3.14) Use a tool to insert columns into the database. You can use DB Browser (SQLite)  
      3.15) Update read config function is "Section 15"  
          &emsp;&emsp;- Add modbus IP  
          &emsp;&emsp;- Add modbus slave  
      3.16) Update config.json on Rasberry PI