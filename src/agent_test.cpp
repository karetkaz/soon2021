#include "agent_plugin.h"
#include <iostream>

using namespace std;
static int maxEvents = 10;

bool watchdog() {
    if ((maxEvents -= 1) < 0) {
        return false;
    }

    if (maxEvents == 2) {
        throw range_error("generated error1");
    }
    if (maxEvents == 5) {
        throw range_error("generated error2");
    }

    cout << "[test]: watchdog" << endl;
    return true;
}


int initPlugin(Plugin &args) {
    for (const auto &v : args.readData("", 2)) {
        cout << "[test]: " << v.name << ": " << v.value << "@" << v.timestamp << endl;
    }
    args.addListener(watchdog);
    return 0;
}
