OUTPUT=.
CFLAGS=-Wall -Wextra -g0 -O3 -std=c++17
CFLAGS+=-I src/lib/mosquitto/include
CFLAGS+=-I src/lib

PYTHON_LIBRARY=python3.7m
PYTHON_INCLUDE=/usr/include/python3.7

test: build
	./Agent

tempAgent.so: src/agent.h src/agent/TempAgent.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/tempAgent.so -I $(PYTHON_INCLUDE) $(filter %.cpp, $^) -ldl -l$(PYTHON_LIBRARY)

testAgent.so: src/agent.h src/agent/TestAgent.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/testAgent.so $(filter %.cpp, $^) -ldl -lpthread

mqttAgent.so: src/agent.h src/agent/MqttAgent.cpp libmosquitto.a
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/mqttAgent.so $(filter %.cpp, $^) $(filter %.a, $^) -lpthread

execAgent.so: src/agent.h src/agent/ExecAgent.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/execAgent.so $(filter %.cpp, $^) $(filter %.a, $^) -lpthread

Agent: src/lib/influxdb.hpp src/core/agent_config.h src/agent.h src/core/agent_config.cpp src/main.cpp
	g++ $(CFLAGS) -o $(OUTPUT)/Agent $(filter %.cpp, $^) -ldl -lstdc++fs

libmosquitto.a:
	make -C src/lib/mosquitto/lib WITH_CJSON=no WITH_TLS=no WITH_STATIC_LIBRARIES=yes
	mv src/lib/mosquitto/lib/libmosquitto.a libmosquitto.a
	make -C src/lib/mosquitto/lib clean

clean:
	rm -f tempAgent.so testAgent.so mqttAgent.so execAgent.so Agent

build: tempAgent.so testAgent.so mqttAgent.so execAgent.so Agent
