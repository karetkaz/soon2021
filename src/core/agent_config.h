#ifndef AGENT_AGENT_CONFIG_H
#define AGENT_AGENT_CONFIG_H

#include "../agent.h"
#include "rapidjson/document.h"
#include <functional>
#include <vector>
#include <iostream>

struct PluginImpl : public Plugin {
    rapidjson::Document settings;
    vector<function<bool()>> listeners;
    string basePath = "./";
    string lastErrorMessage = "";
    uint64_t lastErrorTimestamp = 0;

    PluginImpl(const string& config);

    int mainLoop();

    vector<Data> readData(string entity, int limit) override;

    double getNumber(string property) override;

    string getString(string property) override;

    void addListener(function<bool()> listener) override {
        listeners.push_back(listener);
    }

    string homePath(string file) const override {
        return basePath + file;
    }

    string reportState() const override;
};

#endif //AGENT_AGENT_CONFIG_H
