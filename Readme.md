# install development tools on raspberry
```shell
sudo apt install build-essential
sudo apt install python3.7-dev
```

## install and configure mosquito

```shell
sudo apt install mosquitto mosquitto-clients
sudo systemctl status mosquitto
```

### configure and allow [websocket connection](https://stackoverflow.com/questions/41076961/unable-to-connect-to-mosquitto-over-websocket)

edit the config than restart the service
```shell
sudo nano /etc/mosquitto/mosquitto.conf
sudo systemctl restart mosquitto.service
```

add these lines to the end of configuration
```
port 1883
listener 8083
protocol websockets
```

### set the mqtt host
```shell
export mqtthost=127.0.0.1
```

### show messages sent on given topic
```shell
mosquitto_sub -h localhost -t "piAgent"
```

### list the running agents
```shell
mosquitto_pub -h "$mqtthost" -t "configure" -m "ls"
```

### list the running agents with their process id
```shell
mosquitto_pub -h "$mqtthost" -t "configure" -m "ps"
```

### list the running agent(s) configuration
```shell
mosquitto_pub -h "$mqtthost" -t "configure" -m "get@AgentUser"
#mosquitto_pub -h "$mqtthost" -t "configure" -m "get"
```

### restart a running agent
```shell
mosquitto_pub -h "$mqtthost" -t "configure" -m "restart@AgentUser@<pid>"
```

### stop a running agent
```shell
mosquitto_pub -h "$mqtthost" -t "configure" -m "stop@AgentUser@<pid>"
```

### change the configuration of an agent and restart it
```shell
mosquitto_pub -h "$mqtthost" -t "configure" -m 'AgentUser: {"a": "xyx"}'
```
