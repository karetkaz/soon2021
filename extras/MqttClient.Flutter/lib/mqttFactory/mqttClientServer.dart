import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

MqttClient getMqttClient(String host, String client, {int tcpPort = 1883, int wsPort = 8083}) {
  return MqttServerClient.withPort(host, client, tcpPort);
}