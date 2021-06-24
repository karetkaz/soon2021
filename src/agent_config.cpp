#include "agent_config.h"
#include "lib/influxdb.hpp"
#include <fstream>

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

PluginImpl::PluginImpl(const string& config) : settings(1024) {
    ifstream input(config);
    if (!input) {
        throw runtime_error("failed to load config");
    }
    deserializeJson(settings, input);
}

int PluginImpl::mainLoop() {
    for (size_t i = 0; !listeners.empty(); i++) {
        if (i >= listeners.size()) {
            i = 0;
        }
        try {
            if (!listeners[i]()) {
                std::cout << "watchdog did not respond." << std::endl;
                listeners.erase(listeners.begin() + i);
            }
        } catch (exception &e) {
            lastError = e.what();
        }
    }
    return 0;
}

vector<Plugin::Data> PluginImpl::readData(string entity, int limit) {
    JsonObject db = settings[dataBase];
    influxdb_cpp::server_info si(db[dbHost], db[dbPort], db[dbName], db[dbUser], db[dbPass]);

    stringstream query;
    string response;
    vector<Plugin::Data> result;
    query << "select * from " << db[dbTable];
    if (!entity.empty()) {
        query << " where " << db[dbEntity] << "='" << entity << "'";
    }
    if (limit > 0) {
        query << " limit " << limit;
    }
    int status = influxdb_cpp::query(response, query.str(), si);
    if (status != 0) {
        return result;
    }

    DynamicJsonDocument json(1 << 20);
    DeserializationError err = deserializeJson(json, response);
    if (err != DeserializationError::Ok) {
        string message = "failed to deserialize json: ";
        throw runtime_error(message + err.c_str());
    }
    for (auto results : json[dbResResults].as<JsonArray>()) {
        for (auto series : results[dbResSeries].as<JsonArray>()) {
            // columns: [timestamp, entity_id, value]
            JsonArray columns = series[dbResColumns];
            JsonArray values = series[dbResValues];
            for (auto value : values) {
                result.emplace_back(Data(columns[2], value[2], value[0]));
            }
        }
    }

    return result;
}

string PluginImpl::getConfig(string property) {
    return settings[property];
}
