#include "core/agent_config.h"
#include <filesystem>
#include <dlfcn.h>

using namespace std;
namespace fs = std::filesystem;
char const *configJson = "/config.json";

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
    for (int i = 1; i < argc; i++) {
        string libPath;
        string libFile = argv[i];
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

        int result = ((int (*)(Plugin&))initFunction)(args);
        cout << "agent `" << libFile << "` initialized:  "<< result << endl;
    }

    return args.mainLoop();
}
