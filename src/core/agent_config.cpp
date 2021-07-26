#include "agent_config.h"
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

PluginImpl::PluginImpl(const string& config) {
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
    for (size_t i = 0; !listeners.empty(); i++) {
        if (i >= listeners.size()) {
            n += 1;
            i = 0;
        }
        try {
            if (!listeners[i]()) {
                std::cout << "watchdog did not respond @" << n << std::endl;
                listeners.erase(listeners.begin() + (ssize_t) i);
            }
        } catch (exception &e) {
            lastErrorTimestamp = time(nullptr) * 1000;
            lastErrorMessage = e.what();
        }
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

const rapidjson::Value* getConfig(const PluginImpl *impl, const string &property) {
    const rapidjson::Value *value = &impl->settings;
    stringstream ss(property);
    string item;

    while (getline(ss, item, '.')) {
        if (!value->IsObject()) {
            return nullptr;
        }
        value = &(*value)[item.c_str()];
    }
    return value;
}

double PluginImpl::getNumber(string property) {
    return getConfig(this, property)->GetDouble();
}

string PluginImpl::getString(string property) {
    return getConfig(this, property)->GetString();
}

string PluginImpl::reportState() const {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    if (writer.StartObject()) {
        writer.Key("lastErrorMessage");
        writer.String(lastErrorMessage.c_str());
        writer.Key("lastErrorTimestamp");
        writer.Uint64(lastErrorTimestamp);
        writer.Key("activeModuleCount");
        writer.Uint(listeners.size());
        writer.EndObject();
    }
    return s.GetString();
}
