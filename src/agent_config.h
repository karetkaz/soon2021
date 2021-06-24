#ifndef AGENT_AGENT_CONFIG_H
#define AGENT_AGENT_CONFIG_H

#include "agent_plugin.h"
#include "lib/ArduinoJson.h"
#include <functional>
#include <vector>
#include <iostream>

struct PluginImpl : public Plugin {
    DynamicJsonDocument settings;
    vector<function<bool()>> listeners;
    string lastError = "";

    PluginImpl(const string& config);

    int mainLoop();

    vector<Data> readData(string entity, int limit) override;

    string getConfig(string property) override;

    void addListener(function<bool()> listener) override {
        listeners.push_back(listener);
    }

    string reportState() const override {
        return lastError;
    }
};

#endif //AGENT_AGENT_CONFIG_H
