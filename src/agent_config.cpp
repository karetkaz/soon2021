#include "agent_config.h"
#include <fstream>
#include <influxdb.hpp>
#include <rapidjson/prettywriter.h>
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

PluginImpl::PluginImpl(const string& config) {
    ifstream input(config);
    if (!input) {
        throw runtime_error("failed to open config");
    }
    rapidjson::IStreamWrapper stream(input);
    if (settings.ParseStream(stream).HasParseError()) {
        throw runtime_error("failed to parse config");
    }
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
    for (auto &results : json[dbResResults].GetArray()) {
        for (auto &series : results[dbResSeries].GetArray()) {
            // columns: [timestamp, entity_id, value]
            auto columns = series[dbResColumns].GetArray();
            auto values = series[dbResValues].GetArray();
            for (auto &value : values) {
                result.emplace_back(Data(columns[2].GetString(), value[2].GetDouble(), value[0].GetInt64()));
            }
        }
    }

    return result;
}

string PluginImpl::getConfig(string property) {
    return settings[property.c_str()].GetString();
}
