PROG=modbustcplogger
CC=g++
CPPFLAGS=-g -O0 -Wall -fext-numeric-literals --std=c++17
LDFLAGS= -lsqlite3 -pthread -lssl -lcrypto
ADDLFLAGS= `pkg-config --libs --cflags libmodbus`
INC = -I/usr/include/modbus 
OBJS=battery_main.o battery_json.o sql_queries.o

#-I/home/naina/Desktop/VFLOW_PROJECT
#valgrind --leak-check=yes ./modbustcplogger

$(PROG) : $(OBJS)        
	$(CC)  -o $(PROG) $(OBJS)   $(ADDLFLAGS) $(LDFLAGS)

battery_main.o :
	$(CC) $(CPPFLAGS) $(ADDLFLAGS) -c battery_main.cpp

battery_json.o : battery_json.h
	$(CC) $(CPPFLAGS) $(ADDLFLAGS) -c battery_json.cpp

sql_queries.o :sql_queries.h        
	$(CC) $(CPPFLAGS) $(ADDLFLAGS) -c sql_queries.cpp

clean:
	rm -f $(PROG) $(OBJS)
