#include "../agent.h"
#include <iostream>
#include <Python.h>

using namespace std;

bool watchdog() {
    static int maxEvents = 10;      // TODO: remove, should be in an infinite loop
    if ((maxEvents -= 1) < 0) {
        return false;
    }

    if (maxEvents == 2) {
        throw range_error("generated error1");
    }
    if (maxEvents == 5) {
        throw range_error("generated error2");
    }

    cout << "[test]: watchdog" << endl;
    return true;
}

int initPlugin(Plugin &args) {
    for (const auto &v : args.readData("", 2)) {
        cout << "[test]: " << v.name << ": " << v.value << "@" << v.timestamp << endl;
    }

    Py_Initialize();
    PyRun_SimpleString("print('Hello World from Embedded Python string !!!')");
    string file = args.homePath("test.py");
    FILE* fp = _Py_fopen(file.c_str(), "r");
    PyRun_AnyFile(fp, file.c_str());
    Py_Finalize();

    args.addListener(watchdog);
    return 0;
}
