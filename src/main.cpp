#include "agent_config.h"
#include <filesystem>
#include <dlfcn.h>

using namespace std;

int main(int argc, char *argv[]) {

    PluginImpl args("config.json");

    for (int i = 1; i < argc; i++) {
        string libFile = argv[i];
        string libPath = filesystem::absolute(libFile);

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
