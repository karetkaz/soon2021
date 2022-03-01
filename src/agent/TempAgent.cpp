#include "../agent.h"
#include <iostream>
#include <Python.h>

using namespace std;

struct TempAgent: public Plugin::Agent {
	TempAgent(Plugin &arg) : Agent(arg) {}

	bool check() {
		static int maxEvents = 10;      // TODO: remove, should be in an infinite loop
		if ((maxEvents -= 1) < 0) {
			return false;
		}

		if (maxEvents == 4) {
			for (const auto &v : arg.readData("", 90000)) {
				cout << "[temp]/2: " << v.name << ": " << v.value << "@" << v.timestamp << endl;
			}
		}

		cout << "[temp]: watchdog" << endl;
		return true;
	}
};

int initPlugin(Plugin &args, const string& config) {
	Py_Initialize();
	PyRun_SimpleString("print('Hello World from Embedded Python string !!!')");
	string file = args.homePath(args.getConfig(config + ".script", "undefined.py"));
	FILE* fp = _Py_fopen(file.c_str(), "r");
	if (fp == NULL) {
		Py_Finalize();
		throw runtime_error("Failed to open file: " + file);
	}
	PyRun_AnyFile(fp, file.c_str());
	Py_Finalize();

	args.addAgent(new TempAgent(args));
	(void) config;
	return 0;
}
