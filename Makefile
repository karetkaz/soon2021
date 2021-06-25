OUTPUT=.
CFLAGS=-Wall -Wextra -g0 -O3 -std=c++17
CFLAGS+=-I src/lib

test: build
	./mainAgent mqttAgent.so testAgent.so tempAgent.so

tempAgent.so: src/agent_plugin.h src/agent_temp.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/tempAgent.so $(filter %.cpp, $^) -ldl

testAgent.so: src/agent_plugin.h src/agent_test.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/testAgent.so $(filter %.cpp, $^) -ldl

mqttAgent.so: src/agent_plugin.h src/agent_mqtt.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/mqttAgent.so $(filter %.cpp, $^) -ldl

mainAgent: src/lib/influxdb.hpp src/agent_config.h src/agent_plugin.h src/agent_config.cpp src/main.cpp
	g++ $(CFLAGS) -o $(OUTPUT)/mainAgent $(filter %.cpp, $^) -ldl -lstdc++fs

clean:
	rm -f tempAgent.so testAgent.so mqttAgent.so mainAgent

build: tempAgent.so testAgent.so mqttAgent.so mainAgent
