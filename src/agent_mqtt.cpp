#include <iostream>
#include "agent_plugin.h"

static Plugin *arg;
static string lastState;
static int maxEvents = 500;
bool onEvent() {
    if ((maxEvents -= 1) < 0) {
        return false;
    }
    string newState = arg->reportState();
    if (newState == lastState) {
        return true;
    }

    lastState = newState;
    // TODO: send the change via mqtt
    cout << "[mqtt]: " << newState << endl;
    return true;
}

int initPlugin(Plugin &args) {
    arg = &args;
    args.addListener(onEvent);
    return 0;
}