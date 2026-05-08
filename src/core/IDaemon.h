/* src/core/IDaemon.h */
#ifndef SALIX_NANSHE_IDAEMON_H
#define SALIX_NANSHE_IDAEMON_H

#include <string>
#include <core/IEngine.h>

namespace nanshe::core {
class IDaemon {
    public: 
        virtual ~IDaemon() = default;

        // Service Lifecycle
        // Returns true if socket and resources are pinned successfully
        virtual bool initialize_service(const std::string& socket_path, IEngine* engine) = 0;

        // Blocking: Orchestrates the kqueue sleep/wake cycle
        virtual bool start_event_loop() = 0;

        // Teardown: Best effort cleanup
        virtual void stop_service() = 0;

        // Security

        // If the kernel rejects our security policy, we fail-fast
        virtual bool apply_platform_hardening() = 0;


};

} // namespace nanshe::core

#endif
