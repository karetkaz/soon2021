#include <string>
#include <vector>

/**
 * base class of the Plugin object, given to the agents
 * allows the agent to query data from influx database
 * allows the agent to access to the configuration
 */
struct Plugin {

	/**
	 * Base class of a specialized Agent
	 */
	class Agent {
		friend class PluginImpl;
		Agent *next = nullptr;

	protected:
		Plugin &arg;

		explicit Agent(Plugin &arg) : arg(arg) {};

		/** returns if the plugin is still alve
		 * in case there is an error it should be thrown
		 */
		virtual bool check() = 0;

		/// if check returns false, the listener will be destroyed
		virtual ~Agent() = default;
	};

	/**
	 * Entity data returned from influx database
	 */
	struct Data {
		std::string name;
		long timestamp;
		double value;

		Data(std::string name, long timestamp, double value)
			: name(std::move(name))
			, timestamp(timestamp)
			, value(value) {
		}
	};

	/// name of the initializer method in the plugin files
	static constexpr const char *const initName = "initPlugin";

	/// virtual method for the agents to query data from influx database
	virtual std::vector<Data> readData(std::string entity = "", int limit = 100) = 0;

	/// virtual method for the agents to access configuration values
	virtual double getConfig(const std::string &property, double defValue) const = 0;

	/// virtual method for the agents to access configuration values
	virtual std::string getConfig(const std::string &property, std::string defValue) const = 0;

	/// return the absolute path located at the project root
	virtual std::string homePath(std::string file = "") const = 0;

	/// add a listener to be invoked to check if the module still runs, and if there are errors
	virtual void addAgent(Agent *listener) = 0;

	/** report the internal state in json format:
	  * active module count
	  * last error: database connection failure, etc...
	  * number of executed tasks in the last hour
	  * number of failed tasks in the last hour
	*/
	virtual std::string reportState() const = 0;
};

/// this method declaration needs to be implemented by the plugin, so it can be loaded and executed using dlopen
extern "C" int initPlugin(Plugin &args, const std::string &config);
