#include <windows.h>
#include "agent_config.h"
#include <cstdio>

using namespace std;

int main(int argc, char *argv[]) {

    PluginImpl args("../config.json");

    for (int i = 1; i < argc; i++) {
        string libFile = argv[i];
        string libPath = string("lib") + libFile;

        HMODULE plugin = LoadLibraryA(libPath.c_str());
        if (plugin == nullptr) {
            cout << "Error executing dlopen(`" << libFile << "`): " << GetLastError() << endl;
            return 1;
        }

        FARPROC initFunction = GetProcAddress(plugin, Plugin::initName);
        if (initFunction == nullptr) {
            cout << "Error executing dlsym(`" << libFile << "`, `" << Plugin::initName << "`): " << GetLastError() << endl;
            FreeLibrary(plugin);
            return -1;
        }

        int result = ((int (*)(Plugin&))initFunction)(args);
        cout << "agent `" << libFile << "` initialized:  "<< result << endl;
    }

    return args.mainLoop();
}
