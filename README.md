Original Author: Naina Gupta
July'2021 - ModbusTCPLogger v1.0

Contributing Author: Sin Kiat, Saravanan
Steps for code deployment
Steps to deploy to linux machine.

The following steps will use a Raspberry PI machine as examples.
Auto Start Up of device
Method a):

mkdir /home/pi/.config/autostart  
nano /home/pi/.config/autostart/modbustcplogger.desktop
Adding the following content into the file.

[Desktop Entry]  
Type = Application  
Name = ModbusTCPLogger  
Exec = xterm -hold -e '/home/pi/Desktop/modbustcplogger/modbustcplogger'  
ctrl+s  
ctrl+x  
Method b):
Check for rc-local. All linux system have rc-local service, thus check for the file location or rc.local

systemctl status rc-local
Get the file location of rc.local and modify it. The file may or may not exist but the executed directory can be found int the above status command. Modify the below directory to that of your system.

sudo nano /etc/rc.local
Copy and paste the following content into the file.

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
Modify the file to be executatble

sudo chmod +x /etc/rc.local
The startup script in the directory copied from the above file content.

sudo nano /home/pi/Desktop/modbusctplogger/start.sh
Copy and paste the following content into the file.

sudo tmux new-session -d -s start 'sudo /home/pi/Desktop/modbustcplogger/modbustcplogger'
sudo tmux set-option -t start:0 remain-on-exit
Modify the file to be executable

sudo chmod +x /home/pi/Desktop/modbusctplogger/start.sh
Create directory and prepare your linux machine to compile and execute the program.
cd Desktop
mkdir modbustcplogger
sudo apt-get install libmodbus-dev
sudo apt-get install libsqlite3-dev
sudo apt-get install sqlite3
sudo apt-get install nlohmann-json-dev  
sudo apt-get install libssl-dev  
sudo apt-get install tmux  
Compiling the program
Clear previous build before you rebuild the project by running

make clean
Compile your project.

make
Creating database files for deployment
Run the following command to create or recreate sensordata and statusdata databases.

sh ./create_database.sh
Modify config.json according to current battery (Do not change the format, only modify the values)
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

Send files to remote machine for execution
a) scp -P ./modbustcplogger pi@:/home/pi/Desktop/modbustcplogger
b) scp -P ./db_sensordata.db pi@:/home/pi/Desktop/modbustcplogger
c) scp -P ./db_statusdata.db pi@:/home/pi/Desktop/modbustcplogger
d) scp -P ./config.json pi@:/home/pi/Desktop/modbustcplogger

Ensure MADS platform is setup and mapping is correct
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

Update system time
sudo apt-get install ntp
sudo nano /etc/ntp.conf
Copy and paste the following content and replace the default server references in the file.

server 0.sg.pool.ntp.org
server 1.sg.pool.ntp.org
server 2.sg.pool.ntp.org
server 3.sg.pool.ntp.org
Restart the NTP

sudo service ntp restart
Check for the status

sudo service ntp status
Restart remote raspberry pi machine to apply changes and execute the code (verify the code is executing after boot-up)

Common Errors
a) std::invalid argument

There is 3 lines of code within battery_main.cpp that reference to a static URL. Those url could be invalid.
std::ifstream i("/home/pi/Desktop/modbustcplogger/config.json");
const char * db_data = "/home/pi/Desktop/modbustcplogger/db_sensordata.db";
const char * db_statusdata = "/home/pi/Desktop/modbustcplogger/db_statusdata.db";
 b) <modbus.h> not found.

Open up Makefile and modify the following line.
INC = -I/usr/include/modbus 
or

INC = -I /usr/include/modbus 
SeowSK notes for modifying the files
Documentation: Written by SeowSK

Adding Multiple Servers (For data migration):
1.1) Add a namespace in "Section 1".
  - Each namespace represent a server
1.2) Declare a new int variable for status checking in "Section 2.1"
1.3) Copy "Section 2.2" and "Section 2.3" and paste it on the same indent
  - Modify all status variable to what you have declared on (1.2)

Removing transmission to MADS server
2.1) Remove namespace containing MADS URL in "Section 1"
2.2) Remove code block "Duplicated code for MADS" up to "End of duplicated code for MADS"
  - There are two code block as of 6th April 2023

Adding a modbus connection
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
3.15) Update read config function is "Section 15"
  - Add modbus IP
  - Add modbus slave
3.16) Update config.json on Rasberry PI