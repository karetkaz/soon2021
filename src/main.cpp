#include "core/agent_config.h"
#include <filesystem>
#include <dlfcn.h>

using namespace std;
namespace fs = std::filesystem;
char const *configJson = "/config.json";
char const *const moduleKey = "module";

int initPlugin(PluginImpl& args, const string& libFile, const string& config) {
    string libPath;
    if (!fs::exists(libFile)) {
        libPath = filesystem::absolute("lib" + libFile);
    } else {
        libPath = filesystem::absolute(libFile);
    }

    void *plugin = dlopen(libPath.c_str(), RTLD_NOW);
    if (plugin == nullptr) {
        cout << "Error executing dlopen(`" << libFile << "`): " << dlerror() << endl;
        return 1;
    }

    void *initFunction = dlsym(plugin, Plugin::initName);
    if (initFunction == nullptr) {
        cout << "Error executing dlsym(`" << libFile << "`, `" << Plugin::initName << "`): " << dlerror() << endl;
        dlclose(plugin);
        return -1;
    }

    try {
        return ((int (*)(Plugin &, const string &)) initFunction)(args, config);
    } catch (exception& e) {
        cout << "Error executing " << Plugin::initName << "(`" << libFile << "`, `" << config << "`): " << e.what() << endl;
        return -2;
    }
}

int main(int argc, char *argv[]) {
    // search for configuration file from the current folder in each parent until the root of the filesystem
    string homePath = fs::absolute(fs::path(argv[0]).parent_path());
    while (fs::exists(homePath) && !fs::is_empty(homePath)) {
        if (fs::exists(homePath + configJson)) {
            break;
        }
        string parent = fs::path(homePath).parent_path();
        if (parent == homePath) {
            homePath = "";
            break;
        }
        homePath = parent;
    }

    // open each library and invoke the initializer method
    PluginImpl args(homePath + configJson);

    for (auto &option : args.settings.GetObject()) {
        rapidjson::Value &value = option.value;
        if (!value.IsObject() || !value.HasMember(moduleKey)) {
            continue;
        }

        string agent = value[moduleKey].GetString();
        int init = initPlugin(args, agent, option.name.GetString());
        cout << "agent `" << agent << "` initialized:  " << init << endl;
        if (init != 0) {
            return init;
        }
    }

    for (int i = 1; i < argc; i++) {
        string agent = argv[i];
        int init = initPlugin(args, agent, "");
        cout << "agent `" << agent << "` initialized:  " << init << endl;
        if (init != 0) {
            return init;
        }
    }

    return args.mainLoop();
}
