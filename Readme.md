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

### show messages sent on given topic
```shell
mosquitto_sub -h localhost -t "piAgent"
```
