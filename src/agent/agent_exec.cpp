#include "../agent.h"
#include <iostream>
#include <memory>
#include <thread>

using namespace std::chrono;

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(const char* cmd) {
    std::array<char, 1024> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// config value keys
constexpr char const *execCmd = ".command";
constexpr char const *execRate = ".rate";
constexpr char const *execSamples = ".samples";

static string cmd;
static int samples = 100;
static int sleepMillis = 100;
static pthread_t tid;

void* executor(void* args) {
    time_point<system_clock> next = system_clock::now();
    for ( ; ; ) {
        time_point<system_clock> now = system_clock::now();
        this_thread::sleep_for(next - now);
        next += milliseconds(sleepMillis);

        try {
            cout << exec(cmd.c_str());
        } catch (const runtime_error &e) {
            cout << e.what() << endl;
        }
    }
    return nullptr;
}

bool onEvent() {
    static time_point<system_clock> quitAt = system_clock::now() + seconds(10);
    static time_point<system_clock> stopAt = system_clock::now() + seconds(3);
    if (system_clock::now() > quitAt) {
        return false;
    }
    if (system_clock::now() > stopAt) {
        pthread_cancel(tid);
    }
    return true;
}

int initPlugin(Plugin &args, const string& config) {
    cmd = args.getConfig(config + execCmd, "");
    samples = args.getConfig(config + execSamples, samples);
    sleepMillis = args.getConfig(config + execRate, sleepMillis);

    if (pthread_create(&tid, NULL, executor, NULL)) {
        throw runtime_error("failed to create thread: pthread_create");
    }

    args.addListener(onEvent);
    return 0;
}
