#include "../agent.h"
#include <mosquitto.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <array>
#include <unistd.h>
#include <memory>

using namespace std;

// config value keys
constexpr char const *mqttHost = ".host";
constexpr char const *mqttPort = ".port";
constexpr char const *mqttUser = ".user";
constexpr char const *mqttTopic = ".topic";

static string user;
static string topic;
static string configFile;
constexpr char const *notifyTopic = "topic";
constexpr char const *configTopic = "configure";
constexpr char const *configFilename = "config_.json";
static struct mosquitto *mosq;

void exec(const string &cmd) {
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
}

static void on_connect(struct mosquitto *mosq, void *userdata, int rc);
static void on_disconnect(struct mosquitto *mosq, void *userdata, int rc) {
	printf("on_disconnect(reasonCode: %d)\n", rc);
}
static void on_subscribe(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos);
static void on_unsubscribe(struct mosquitto *mosq, void *userdata, int mid) {
	printf("on_unsubscribe(mid: %d)\n", mid);
}
static void on_publish(struct mosquitto *mosq, void *userdata, int mid);
static void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
static void publish_data(struct mosquitto *mosq, const char *message);

struct MqttAgent: public Plugin::Agent {
	string state;

	explicit MqttAgent(Plugin &arg) : Agent(arg) {}

	bool check() override {
		string newState = arg.reportState();
		if (newState == state) {
			return true;
		}

		state = newState;
		string message = user + ": " + newState;
		cout << "[mqtt]: " << message << endl;
		publish_data(mosq, message.c_str());
		return true;
	}
};

int initPlugin(Plugin &args, const string &config) {
	mosquitto_lib_init();
	mosq = mosquitto_new(nullptr, true, nullptr);
	if (mosq == nullptr) {
		fprintf(stderr, "Error: Out of memory.\n");
		mosquitto_lib_cleanup();
		return 1;
	}

	/* Configure callbacks. This should be done before connecting ideally. */
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_subscribe_callback_set(mosq, on_subscribe);
	mosquitto_unsubscribe_callback_set(mosq, on_unsubscribe);
	mosquitto_publish_callback_set(mosq, on_publish);
	mosquitto_message_callback_set(mosq, on_message);

	configFile = args.homePath(configFilename);
	user = args.getConfig(config + mqttUser, user);
	topic = args.getConfig(config + mqttTopic, notifyTopic);
	string host = args.getConfig(config + mqttHost, "localhost");
	int port = (int) args.getConfig(config + mqttPort, -1);
	int rc = mosquitto_connect(mosq, host.c_str(), port, 60);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
		return 1;
	}

	rc = mosquitto_loop_start(mosq);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
		return 1;
	}

	args.addAgent(new MqttAgent(args));
	return 0;
}

/* Callback called when the client receives a CONNACK message from the broker. */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
	/* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
	printf("on_connect: %s\n", mosquitto_connack_string(rc));
	if (rc != 0) {
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
		mosquitto_disconnect(mosq);
	}

	/* Making subscriptions in the on_connect() callback means that if the
	 * connection drops and is automatically resumed by the client, then the
	 * subscriptions will be recreated when the client reconnects. */
	int subscribeRC = mosquitto_subscribe(mosq, nullptr, configTopic, 1);
	if (subscribeRC != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(subscribeRC));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
static void on_subscribe(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos) {
	bool have_subscription = false;

	/* In this example we only subscribe to a single topic at once, but a
	 * SUBSCRIBE can contain many topics at once, so this is one way to check
	 * them all. */
	for (int i = 0; i < qos_count; i++) {
		printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
		if (granted_qos[i] <= 2) {
			have_subscription = true;
		}
	}
	if (!have_subscription) {
		/* The broker rejected all of our subscriptions, we know we only sent
		 * the one SUBSCRIBE, so there is no point remaining connected. */
		fprintf(stderr, "Error: All subscriptions rejected.\n");
		mosquitto_disconnect(mosq);
	}
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
static void on_publish(struct mosquitto *mosq, void *userdata, int mid) {
	// printf("Message with mid %d has been published.\n", mid);
}

/* This function pretends to read some data from a sensor and publish it.*/
static void publish_data(struct mosquitto *mosq, const char *message) {
	int rc = mosquitto_publish(mosq, nullptr, topic.c_str(), (int) strlen(message), message, 2, false);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
	}
}

static void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	printf("on_message(%s, %d, %s)\n", message->topic, message->qos, (char *) message->payload);
	if (message->payload == nullptr) {
		// no payload
		return;
	}
	if (strcmp(message->topic, configTopic) != 0) {
		// not configuring
		return;
	}

	// list all users
	if (strcmp((char *) message->payload, "ls") == 0) {
		const char *usr = user.c_str();
		mosquitto_publish(mosq, nullptr, configTopic, (int) strlen(usr), usr, 2, false);
		return;
	}

	if (strcmp((char *) message->payload, "ps") == 0) {
		string mgs = user + "@" + to_string(getpid());
		const char *usr = mgs.c_str();
		mosquitto_publish(mosq, nullptr, configTopic, (int) strlen(usr), usr, 2, false);
		return;
	}

	// get config from all users
	if (strcmp((char *) message->payload, "get") == 0) {
		ifstream config(configFile);
		if (!config) {
			cout << "cannot open config file: " << configFile << endl;
			return;
		}
		char json[2 << 20];
		size_t n = config.readsome(json, size(json));
		mosquitto_publish(mosq, nullptr, configTopic, (int) n, json, 2, false);
		return;
	}
	// get for current user
	string getUser = "get@" + user;
	if (strcmp((char *) message->payload, getUser.c_str()) == 0) {
		ifstream config(configFile);
		if (!config) {
			cout << "cannot open config file: " << configFile << endl;
			return;
		}
		char json[2 << 20];
		size_t n = config.readsome(json, size(json));
		mosquitto_publish(mosq, nullptr, configTopic, (int) n, json, 2, false);
		return;
	}

	string restartUser = "restart@" + user;
	if (strcmp((char *) message->payload, restartUser.c_str()) == 0) {
		ifstream config(configFile);
		if (!config) {
			cout << "cannot open config file: " << configFile << endl;
			return;
		}
		exec("./Agent &> /dev/null &");
		exit(-1);
		return;
	}

	// put for current user
	if (strcmp(message->topic, configTopic) == 0) {
		string putUser = user + ": ";
		if (strncmp((char *) message->payload, putUser.c_str(), strlen(putUser.c_str())) == 0) {
			ofstream config(configFile);
			if (!config) {
				cout << "cannot open config file: " << configFile << endl;
				return;
			}
			config << (char *) message->payload + strlen(putUser.c_str()) << endl;
			exec("./Agent &> /dev/null &");
			exit(-1);
		}
	}
}
