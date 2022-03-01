#include "../agent.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;

// config value key representing the delay between two consecutive data processing
constexpr char const *execRate = ".sampleRate";
// config value key representing the number of values to querry from database
constexpr char const *execSamples = ".samples";

struct TestAgent: public Plugin::Agent {
	string state;
	bool alive = true;
	int samples = 20;
	int sleep = 100;
	pthread_t tid;

	TestAgent(Plugin &args, const string &config) : Agent(args) {
		this->samples = args.getConfig(config + execSamples, samples);
		this->sleep = args.getConfig(config + execRate, sleep);

		// can not use std::thread from dynamically loaded shared library
		if (pthread_create(&tid, nullptr, reinterpret_cast<void *(*)(void *)>(execute), this)) {
			throw runtime_error("failed to create thread: pthread_create");
		}
	}

	~TestAgent() {
		pthread_join(tid, NULL);
	}

	bool check() {
		if (!alive) {
			return false;
		}
		if (state.empty()) {
			return true;
		}
		string x = state;
		state = "";
		throw runtime_error(x);
	}

	static void *execute(TestAgent *agent) {
		time_point<system_clock> next = system_clock::now();
		Plugin *plugin = &agent->arg;
		int samples = agent->samples;
		int sleep = agent->sleep;

		// endless loop quiting after 50 iterations
		for (int i = 0; i < 5; ++i) {
			try {
				// sleep for a while
				time_point<system_clock> now = system_clock::now();
				this_thread::sleep_for(next - now);
				next += milliseconds(sleep);

				// read the last 20 entries from influx database, and diagnose
				double min = std::numeric_limits<double>::infinity();
				double max = -std::numeric_limits<double>::infinity();
				for (const auto &v: plugin->readData("", samples)) {
					min = std::min(min, v.value);
					max = std::max(max, v.value);
				}
				cout << i << ", min: " << min << ", max: " << max << endl;
				if (min == max)  {
					agent->state = "minimum is equal to maximum: " + to_string(min);
				}
			} catch (const exception &e) {
				agent->state = string(e.what());
				cout << "error: " << e.what() << endl;
			}
		}
		// stop the agent
		agent->alive = false;
		return nullptr;
	}
};


int initPlugin(Plugin &args, const string& config) {
	args.addAgent(new TestAgent(args, config));
	return 0;
}
