import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_browser_client.dart';

MqttClient getMqttClient(String host, String clientId, {int tcpPort = 1883, int wsPort = 8083}) {
  MqttClient client = MqttBrowserClient.withPort('ws://' + host, clientId+"Web", wsPort);
  client.websocketProtocolString = ['mqtt'];
  return client;
}