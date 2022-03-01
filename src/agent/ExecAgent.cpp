#include "../agent.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;

// config value keys
constexpr char const *execCmd = ".command";
constexpr char const *execRate = ".sampleRate";
constexpr char const *execTimeout = ".timeout";
constexpr char const *execSamples = ".samples";

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
int exec(std::stringstream &result, const string &cmd) {
	FILE *pipe = nullptr;
	try {
		pipe = popen(cmd.c_str(), "r");
		if (pipe == nullptr) {
			throw std::runtime_error("popen() failed!");
		}
		std::array<char, 1024> buffer{};
		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
			result << buffer.data();
		}
		return pclose(pipe);
	} catch (...) {
		if (pipe != nullptr) {
			pclose(pipe);
		}
		throw;
	}
}

struct ExecAgent: public Plugin::Agent {
	string cmd;
	string result;
	int samples = 0;
	int sleepMillis = 100;

	pthread_t tid;
	time_point<system_clock> quitAt = time_point<system_clock>::max();

	ExecAgent(Plugin &args, const string &config) : Agent(args) {
		this->cmd = args.getConfig(config + execCmd, "");
		int timeout = args.getConfig(config + execTimeout, 0);
		this->samples = args.getConfig(config + execSamples, samples);
		this->sleepMillis = args.getConfig(config + execRate, sleepMillis);

		if (pthread_create(&tid, nullptr, reinterpret_cast<void *(*)(void *)>(executor), this)) {
			throw runtime_error("failed to create thread: pthread_create");
		}
		if (timeout > 0) {
			quitAt = system_clock::now() + milliseconds(timeout);
		}
	}

	~ExecAgent() {
		pthread_cancel(tid);
		pthread_join(tid, NULL);
	}

	static void *executor(ExecAgent *agent) {
		time_point<system_clock> next = system_clock::now();

		int sleep = agent->sleepMillis;
		int samples = agent->samples;
		string cmd = agent->cmd;
		Plugin *plugin = &agent->arg;
		for ( ; ; ) {
			time_point<system_clock> now = system_clock::now();
			this_thread::sleep_for(next - now);
			next += milliseconds(sleep);

			try {
				stringstream cmdWithArgs;
				stringstream outStream;
				cmdWithArgs << cmd;
				if (samples > 0) {
					for (const auto &v: plugin->readData("", samples)) {
						cmdWithArgs << " '" << v.timestamp << ":" << v.value << "'";
					}
				}
				if (exec(outStream, cmdWithArgs.str()) != 0) {
					agent->result = outStream.str();
					throw std::runtime_error("execute() failed: " + outStream.str());
				}
				agent->result = outStream.str();
			} catch (const exception &e) {
				cout << "error: " << e.what() << endl;
			}
		}
		return nullptr;
	}

	bool check() {
		if (system_clock::now() > quitAt) {
			return false;
		}
		if (result.empty()) {
			return true;
		}
		string x = std::move(result);
		result = "";
		throw runtime_error(x);
	}
};

int initPlugin(Plugin &args, const string &config) {
	args.addAgent(new ExecAgent(args, config));
	return 0;
}
