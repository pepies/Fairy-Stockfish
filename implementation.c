#include <stdio.h>
#include <dlfcn.h>  // Header for dynamic loading of libraries

// Function pointers for dynamically loaded library functions
typedef void (*InitializeEngineFunc)(int argc, char* argv[]);
typedef void (*RunEngineLoopFunc)(int argc, char* argv[]);
typedef void (*ShutdownEngineFunc)();
typedef const char* (*GetOutputFunc)();
typedef void (*SendCommandFunc)(const char* command);

int main() {
    void* handle;
    InitializeEngineFunc initialize_engine;
    RunEngineLoopFunc run_engine_loop;
    ShutdownEngineFunc shutdown_engine;
    GetOutputFunc get_output;
    SendCommandFunc send_command;

    // Load the shared library
    handle = dlopen("./libstockfish.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }

    // Load the functions from the library
    initialize_engine = (InitializeEngineFunc)dlsym(handle, "initialize_engine");
    run_engine_loop = (RunEngineLoopFunc)dlsym(handle, "run_engine_loop");
    shutdown_engine = (ShutdownEngineFunc)dlsym(handle, "shutdown_engine");
    get_output = (GetOutputFunc)dlsym(handle, "get_output");
    send_command = (SendCommandFunc)dlsym(handle, "send_command");

    if (!initialize_engine || !run_engine_loop || !shutdown_engine || !get_output || !send_command) {
        fprintf(stderr, "Error: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    // Initialize engine with command line arguments (if any)
    char* argv[] = {"stockfish", ""};
    int argc = sizeof(argv) / sizeof(argv[0]) - 1;
    initialize_engine(argc, argv);

    // Example: Send a command to the engine
    send_command("uci");

    // Run engine loop (this should run indefinitely until shutdown)
    run_engine_loop(argc, argv);

    // Example: Retrieve and print output from the engine
    const char* output = get_output();
    printf("Output from engine:\n%s\n", output);

    // Shutdown the engine
    shutdown_engine();

    // Close the library handle
    dlclose(handle);

    return 0;
}