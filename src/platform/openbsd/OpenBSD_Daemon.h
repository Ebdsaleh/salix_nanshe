/* src/platform/openbsd/OpenBSD_Daemon.h */
#ifndef SALIX_NANSHE_OPENBSD_DAEMON_H
#define SALIX_NANSHE_OPENBSD_DAEMON_H

#include <core/IDaemon.h>
#include <sys/types.h>
#include <sys/event.h> // For kqueue
#include <memory>

namespace nanshe::platform::openbsd {

    class OpenBSD_Daemon : public nanshe::core::IDaemon {
        public:
            OpenBSD_Daemon();
            ~OpenBSD_Daemon() override;

            // Implementation of IDaemon contract
            bool initialize_service(const std::string& socket_path, nanshe::core::IEngine* engine) override;
            bool apply_platform_hardening() override;
            bool start_event_loop() override;
            void stop_service() override;

        private:
            struct Pimpl;
            std::unique_ptr<Pimpl> pimpl; 
    };

} // namespace nanshe::platform::openbsd

#endif
