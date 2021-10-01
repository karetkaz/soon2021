#include "../agent.h"
#include <mosquitto.h>
#include <iostream>
#include <cstring>

// config value keys
constexpr char const *mqttHost = ".host";
constexpr char const *mqttPort = ".port";
constexpr char const *mqttTopic = ".topic";

static Plugin *arg;
static string state;
static string topic = "topic";
struct mosquitto *mqtt;

void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
void on_publish(struct mosquitto *mosq, void *obj, int mid);
void publish_data(struct mosquitto *mosq, const char *message);

bool onEvent() {
    static int maxEvents = 500000;     // TODO: remove, should be in an infinite loop
    if ((maxEvents -= 1) < 0) {
        mosquitto_destroy(mqtt);
        mosquitto_lib_cleanup();
        return false;
    }

    string newState = arg->reportState();
    if (newState == state) {
        return true;
    }

    state = newState;
    cout << "[mqtt]: " << newState << endl;
    publish_data(mqtt, newState.c_str());
    return true;
}

int initPlugin(Plugin &args, const string& config) {
    mosquitto_lib_init();
    mqtt = mosquitto_new(nullptr, true, nullptr);
    if (mqtt == nullptr) {
        fprintf(stderr, "Error: Out of memory.\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    /* Configure callbacks. This should be done before connecting ideally. */
    mosquitto_connect_callback_set(mqtt, on_connect);
    mosquitto_publish_callback_set(mqtt, on_publish);

    topic = args.getConfig(config + mqttTopic, topic);
    string host = args.getConfig(config + mqttHost, "localhost");
    int port = (int) args.getConfig(config + mqttPort, -1);
    int rc = mosquitto_connect(mqtt, host.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mqtt);
        mosquitto_lib_cleanup();
        return 1;
    }

    rc = mosquitto_loop_start(mqtt);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mqtt);
        mosquitto_lib_cleanup();
        return 1;
    }

    args.addListener(onEvent);
    arg = &args;
    return 0;
}

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    /* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if (reason_code != 0) {
        /* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
        mosquitto_disconnect(mosq);
    }

    /* You may wish to set a flag here to indicate to your application that the
	 * client is now connected. */
    (void) obj;
}


/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_publish(struct mosquitto *mosq, void *obj, int mid) {
    printf("Message with mid %d has been published.\n", mid);
    (void) mosq;
    (void) obj;
}

/* This function pretends to read some data from a sensor and publish it.*/
void publish_data(struct mosquitto *mosq, const char *message) {
    int rc = mosquitto_publish(mosq, nullptr, topic.c_str(), strlen(message), message, 2, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
    }
}
