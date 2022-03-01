#ifndef AGENT_AGENT_CONFIG_H
#define AGENT_AGENT_CONFIG_H

#include "../agent.h"

//#define RAPIDJSON_PARSE_ERROR_NORETURN(parseErrorCode,offset) throw throw std::runtime_error(#parseErrorCode)
#define RAPIDJSON_ASSERT(expr) if (!static_cast <bool> (expr)) throw std::runtime_error(#expr)
//#define RAPIDJSON_ASSERT(expr) static_cast <bool> (expr) ? void(0) : throw std::runtime_error(#expr)
#include "rapidjson/document.h"
#include <functional>
#include <vector>
#include <iostream>

struct PluginImpl : public Plugin {
	rapidjson::Document settings;
	Agent *listeners = nullptr;

	std::string basePath = "./";
	std::string lastErrorMessage = "";
	uint64_t lastErrorTimestamp = 0;

	PluginImpl(const std::string& config);

	int mainLoop();

	std::vector<Data> readData(std::string entity, int limit) override;

	const rapidjson::Value* getConfig(const std::string &property) const;

	std::string getConfig(const std::string &property, std::string defValue) const override {
		const rapidjson::Value *value = getConfig(property);
		if (value == nullptr || !value->IsString()) {
			return defValue;
		}
		return value->GetString();
	}

	double getConfig(const std::string &property, double defValue) const override {
		const rapidjson::Value *value = getConfig(property);
		if (value == nullptr || !value->IsNumber()) {
			return defValue;
		}
		return value->GetDouble();
	}

	void addAgent(Agent *listener) override {
		listener->next = this->listeners;
		this->listeners = listener;
	}

	std::string homePath(std::string file) const override {
		return basePath + file;
	}

	std::string reportState() const override;
};

#endif //AGENT_AGENT_CONFIG_H
