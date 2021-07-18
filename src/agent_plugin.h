#include <unordered_map>
#include <functional>
#include <string>
#include <vector>
#include <utility>

using namespace std;

struct Plugin {

    struct Data {
        string name;
        double value;
        long timestamp;

        Data(string name, double value, long timestamp) : name(name), value(value), timestamp(timestamp) {}
    };

    /// name of the initializer method in the plugin files
    static constexpr const char *initName = "initPlugin";

    /// virtual callback method for the initializer method to query data from database
    virtual vector<Data> readData(string entity = "", int limit = 40) = 0;

    /// virtual callback method for the initializer method to access configuration values
    virtual double getNumber(string property) = 0;

    /// virtual callback method for the initializer method to access configuration values
    virtual string getString(string property) = 0;

    ///
    virtual void addListener(function<bool()> listener) = 0;

    /** report the internal state in json format:
      * active module count
      * last error: database connection failure, etc...
      * number of executed tasks in the last hour
      * number of failed tasks in the last hour
    */
    virtual string reportState() const = 0;
};

// this forward method declaration needs to be implemented by the plugin, so it can be loaded and executed using dlopen
extern "C" int initPlugin(Plugin &args);
