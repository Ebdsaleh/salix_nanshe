/* src/daemon/main.cpp */
#include <iostream>
#include <memory>
#include <string>

/* Core Interfaces */
#include <core/IDaemon.h>
#include <core/IEngine.h>

/* Platform-Specific Implementations */
#include <platform/openbsd/OpenBSD_Daemon.h>

/* Engine-Specific Implementations (The Llama Adapter) */
#include <engine/llama/Llama_Engine.h> 

/**
 * Note: For this build, we are assuming Llama_Engine implements nanshe::core::IEngine
 * and handles all the llama_backend_init and generation loops internally.
 */

int main(int argc, char** argv) {
    // 1. Argument Validation
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_model.gguf>" << std::endl;
        return 1;
    }

    const std::string model_path = argv[1];
    const std::string socket_path = "/tmp/nanshe.sock";

    // 2. Instantiate the "Brain" (The AI Engine)
    // Later, you can swap this for Sumerian_Engine without changing this file.
    std::unique_ptr<nanshe::core::IEngine> nanshe_engine =  std::make_unique<nanshe::engine::llama::Llama_Engine>();

    // 3. Instantiate the "Skeleton" (The OS-Domain Daemon)
    std::unique_ptr<nanshe::core::IDaemon> platform_daemon = std::make_unique<nanshe::platform::openbsd::OpenBSD_Daemon>();

    // 4. Initialize Service (Socket setup, model loading)
    std::cout << "[Nanshe-d] Initializing Salix Nanshe Service..." << std::endl;
    
    /**
     * Logic: We'll eventually pass the engine to the daemon so the daemon 
     * can poke the engine when the socket receives data.
     */
    if (!platform_daemon->initialize_service(socket_path, nanshe_engine.get())) {
        std::cerr << "[Nanshe-d] Fatal: Failed to initialize service." << std::endl;
        return 1;
    }

    // Load the model BEFORE applying the security pledge
    std::cout << "[Nanshe-d] Spinning up inference engine..." << std::endl;

    if (!nanshe_engine->load_model_asset(model_path)) {
        std::cerr << "[Nanshe-d] Fatal: Failed to load model asset: " << model_path << std::endl;
        return 1;
    }
    

    // 5. Apply Platform Hardening (The "Security Interlock")
    std::cout << "[Nanshe-d] Applying OpenBSD security policies..." << std::endl;
    if (!platform_daemon->apply_platform_hardening()) {
        std::cerr << "[Nanshe-d] Fatal: Kernel rejected security pledge/unveil." << std::endl;
        return 1;
    }

    // 6. Enter Execution State (The event loop/suspension)
    std::cout << "[Nanshe-d] Daemon active. Entering event loop." << std::endl;
    if (!platform_daemon->start_event_loop()) {
        std::cerr << "[Nanshe-d] Fatal: Event loop failure." << std::endl;
        return 1;
    }

    // 7. Graceful Teardown
    platform_daemon->stop_service();
    std::cout << "[Nanshe-d] Service terminated cleanly." << std::endl;

    return 0;
}
