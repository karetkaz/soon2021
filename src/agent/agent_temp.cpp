#include "../agent.h"
#include <iostream>

using namespace std;

static Plugin *arg;

bool onEvent() {
    static int maxEvents = 10;      // TODO: remove, should be in an infinite loop
    if ((maxEvents -= 1) < 0) {
        return false;
    }

    if (maxEvents == 4) {
        for (const auto &v : arg->readData("", 90000)) {
            cout << "[temp]/2: " << v.name << ": " << v.value << "@" << v.timestamp << endl;
        }
    }

    cout << "[temp]: watchdog" << endl;
    return true;
}

int initPlugin(Plugin &args, const string& config) {
    arg = &args;
    for (const auto &v : args.readData()) {
        cout << "[temp]: " << v.name << ": " << v.value << "@" << v.timestamp << endl;
    }
    args.addListener(onEvent);
    (void) config;
    return 0;
}