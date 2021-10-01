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

    const rapidjson::Value* getConfig(const string &property) const;

    string getConfig(const string &property, string defValue) const override {
        const rapidjson::Value *value = getConfig(property);
        if (value == nullptr || !value->IsString()) {
            return defValue;
        }
        return value->GetString();

    }

    double getConfig(const string &property, double defValue) const override {
        const rapidjson::Value *value = getConfig(property);
        if (value == nullptr || !value->IsNumber()) {
            return defValue;
        }
        return value->GetDouble();
    }

    void addListener(function<bool()> listener) override {
        listeners.push_back(listener);
    }

    string homePath(string file) const override {
        return basePath + file;
    }

    string reportState() const override;
};

#endif //AGENT_AGENT_CONFIG_H
