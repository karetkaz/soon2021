#include "agent_config.h"
#include <thread>
#include <fstream>
#include <filesystem>
#include <influxdb.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>

constexpr char const *dataBase = "database";
constexpr char const *dbHost = "host";
constexpr char const *dbPort = "port";
constexpr char const *dbUser = "user";
constexpr char const *dbPass = "pass";
constexpr char const *dbName = "name";
constexpr char const *dbTable = "table";
constexpr char const *dbEntity = "entity";

constexpr char const *dbResResults = "results";
constexpr char const *dbResSeries = "series";
constexpr char const *dbResColumns = "columns";
constexpr char const *dbResValues = "values";

using namespace std;

PluginImpl::PluginImpl(const string &config) {
	ifstream input(config);
	if (!input) {
		throw runtime_error("failed to open config");
	}
	rapidjson::IStreamWrapper stream(input);
	if (settings.ParseStream(stream).HasParseError()) {
		throw runtime_error("failed to parse config");
	}
	basePath = filesystem::absolute(config).parent_path().string() + "/";
}

int PluginImpl::mainLoop() {
	size_t n = 0;
	Agent *prev = nullptr;
	for (Agent *it = listeners; it != nullptr; n++) {
		try {
			if (!it->check()) {
				std::cout << "watchdog did not respond " << "@" << n << std::endl;
				if (prev != nullptr) {
					prev->next = it->next;
				} else {
					listeners = it->next;
				}
				delete it;
				prev = nullptr;
				it = listeners;
				continue;
			}
		} catch (exception &e) {
			lastErrorTimestamp = time(nullptr) * 1000;
			lastErrorMessage = e.what();
		}
		prev = it;
		it = it->next;
		if (it == nullptr) {
			prev = nullptr;
			it = listeners;
		}
		this_thread::sleep_for(1ms);
	}
	(void) initPlugin;
	return 0;
}

vector<Plugin::Data> PluginImpl::readData(string entity, int limit) {
	rapidjson::Value &db = settings[dataBase];
	string host = db[dbHost].GetString();
	int port = db[dbPort].GetInt();
	string name = db[dbName].GetString();
	string user = db[dbUser].GetString();
	string pass = db[dbPass].GetString();
	influxdb_cpp::server_info si(host, port, name, user, pass);

	stringstream query;
	query << "select * from " << db[dbTable].GetString();
	if (!entity.empty()) {
		query << " where " << db[dbEntity].GetString() << "='" << entity << "'";
	}
	query << " order by time desc";
	if (limit > 0) {
		query << " limit " << limit;
	}

	string response;
	int status = influxdb_cpp::query(response, query.str(), si);
	if (status != 0) {
		return {};
	}

	rapidjson::Document json;
	if (json.Parse(response.c_str()).HasParseError()) {
		throw runtime_error("failed to parse response");
	}

	vector<Plugin::Data> result;
	for (auto &results: json[dbResResults].GetArray()) {
		for (auto &series: results[dbResSeries].GetArray()) {
			// columns: [timestamp, entity_id, value]
			auto columns = series[dbResColumns].GetArray();
			auto values = series[dbResValues].GetArray();
			for (auto &value: values) {
				result.emplace_back(columns[2].GetString(), value[2].GetDouble(), value[0].GetInt64());
			}
		}
	}

	return result;
}

const rapidjson::Value *PluginImpl::getConfig(const string &property) const {
	const rapidjson::Value *value = &settings;
	stringstream ss(property);
	string item;

	while (getline(ss, item, '.')) {
		if (!value->IsObject()) {
			return nullptr;
		}
		if (!value->HasMember(item.c_str())) {
			return nullptr;
		}
		value = &(*value)[item.c_str()];
	}
	return value;
}

string PluginImpl::reportState() const {
	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);
	int activeModuleCount = 0;
	for (Agent *it = listeners; it != nullptr; it = it->next) {
		activeModuleCount += 1;
	}
	if (writer.StartObject()) {
		writer.Key("lastErrorMessage");
		writer.String(lastErrorMessage.c_str());
		writer.Key("lastErrorTimestamp");
		writer.Uint64(lastErrorTimestamp);
		writer.Key("activeModuleCount");
		writer.Uint(activeModuleCount);
		writer.EndObject();
	}
	return s.GetString();
}
