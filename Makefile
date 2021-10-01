OUTPUT=.
CFLAGS=-Wall -Wextra -g0 -O3 -std=c++17
CFLAGS+=-I src/lib/mosquitto/include
CFLAGS+=-I src/lib

PYTHON_LIBRARY=python3.8
PYTHON_INCLUDE=/usr/include/python3.8
#raspberry
#PYTHON_LIBRARY=python3.7m
#PYTHON_INCLUDE=/usr/include/python3.7

test: build
	./mainAgent

tempAgent.so: src/agent.h src/agent/agent_temp.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/tempAgent.so $(filter %.cpp, $^) -ldl

testAgent.so: src/agent.h src/agent/agent_test.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/testAgent.so -I $(PYTHON_INCLUDE) $(filter %.cpp, $^) -ldl -l$(PYTHON_LIBRARY)

mqttAgent.so: src/agent.h src/agent/agent_mqtt.cpp src/lib/mosquitto/lib/libmosquitto.a
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/mqttAgent.so $(filter %.cpp, $^) $(filter %.a, $^) -lpthread

execAgent.so: src/agent.h src/agent/agent_exec.cpp
	g++ -fPIC -shared $(CFLAGS) -o $(OUTPUT)/execAgent.so $(filter %.cpp, $^) $(filter %.a, $^)

mainAgent: src/lib/influxdb.hpp src/core/agent_config.h src/agent.h src/core/agent_config.cpp src/main.cpp
	g++ $(CFLAGS) -o $(OUTPUT)/mainAgent $(filter %.cpp, $^) -ldl -lstdc++fs

src/lib/mosquitto/lib/libmosquitto.a:
	make -C src/lib/mosquitto/lib WITH_CJSON=no WITH_TLS=no WITH_STATIC_LIBRARIES=yes

clean:
	rm -f tempAgent.so testAgent.so mqttAgent.so execAgent.so mainAgent

build: tempAgent.so testAgent.so mqttAgent.so execAgent.so mainAgent

#sudo apt install build-essential
#sudo apt install python3.7-dev

#sudo apt install mosquitto mosquitto-clients
#sudo systemctl status mosquitto
#mosquitto_sub -h localhost -t "piAgent"

#ssh pi:raspberry@192.168.50.110
#cd /home/pi/Desktop/Soon/dlaicourse-master/TensorFlow\ Deployment/Course\ 2\ -\ TensorFlow\ Lite/Week\ 4/Examples/image_classification/
#python3 classify.py --filename dog.jpg --model_path mobilenet_v1_1.0_224_quant.tflite --label_path labels_mobilenet_quant_v1_224.txt
