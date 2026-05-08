/* src/platform/openbsd/OpenBSD_Daemon.cpp */
#include <platform/openbsd/OpenBSD_Daemon.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>    // For sockaddr_un
#include <sys/event.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <system_error>


namespace nanshe::platform::openbsd {
    
    // The "Authorized Message" Code (Salix Magic Signature)
    constexpr uint64_t _SALIX_MAGIC_HEADER = 0x53414C49585F4441; // "SALIX_DA"

    struct OpenBSD_Daemon::Pimpl {
        // member variables
        int32_t _server_socket_fd = -1;
        int32_t _kernel_queue_fd = -1;
        std::string _active_socket_path;
        nanshe::core::IEngine* _engine = nullptr;

        // private  methods
        // Internal helper for the handshake
        bool _verify_client_handshake(int32_t client_fd);                

        void handle_client_connection(int32_t client_fd);
        bool _setup_unix_socket();
        };

    /* Pimpl private methods  */



    bool OpenBSD_Daemon::Pimpl::_verify_client_handshake(int32_t client_fd) {
        // 1. Kernel-level Identity Check (getpeereid)
        uid_t euid;
        gid_t egid;
        if (getpeereid(client_fd, &euid, &egid) != 0) return false;

        // Only allow the user who started the Salix Nanshe daemon to interact with it
        if (euid != geteuid()) {
            std::cerr << "[Security] Blocked unauthorized UID: " << euid << std::endl;
            return false;
        } 

        // 2. Magic Header Check
        uint64_t received_header = 0;
        ssize_t bytes_read = read(client_fd, &received_header, sizeof(received_header));

        return (bytes_read == sizeof(uint64_t) && received_header == _SALIX_MAGIC_HEADER);
    
    }



    void OpenBSD_Daemon::Pimpl::handle_client_connection(int32_t client_fd) {
        if (!_verify_client_handshake(client_fd)) {
            close(client_fd);
            return;
        }

        // 1. Read the prompt from the client
        char buffer[1024] = {0};
        read(client_fd, buffer, sizeof(buffer) - 1);
        std::string prompt(buffer);

        // 2. THE WIRING: Pass the prompt to the Gemma-3 Brain
        std::cout << "[Nanshe-d] Thinking..." << std::endl;
        std::string response = _engine->predict_next_sequence(prompt);

        // 3. Send the REAL AI response back to the client
        write(client_fd, response.c_str(), response.length());

        close(client_fd);
    }



    bool OpenBSD_Daemon::Pimpl::_setup_unix_socket() {
        /* Native OpenBSD socket code */ 
        // 1. Create the socket (Assembler-grade efficiency)
        _server_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (_server_socket_fd < 0) {
            return false;
        }

        // 2. Prepare the address structure
        struct sockaddr_un address;
        std::memset(&address, 0, sizeof(address));
        address.sun_family = AF_UNIX;

        // Ensure the path fits in the sun_path buffer
        std::strncpy(address.sun_path, _active_socket_path.c_str(), sizeof(address.sun_path) -1);

        // 3. The "Clean-Room" Ritual: Remove any stale socket files from previous crashes
        unlink(_active_socket_path.c_str());

        // 4. Bind the socket to the filesystem path
        if (bind(_server_socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(_server_socket_fd);
            return false;
        }

        // 5. Listen for incoming "pokes" from the client
        if (listen(_server_socket_fd, 5) < 0) {
            close(_server_socket_fd);
            return false;
        }
        return true;
    }


    
    
    /* Constructor, Destructor */
    OpenBSD_Daemon::OpenBSD_Daemon() : pimpl(std::make_unique<Pimpl>()) {}

    OpenBSD_Daemon::~OpenBSD_Daemon() { stop_service(); }


    /* public methods */


    bool OpenBSD_Daemon::initialize_service(const std::string& socket_path, nanshe::core::IEngine* engine) {
        if (socket_path.empty() || !engine) return false;
        pimpl->_active_socket_path = socket_path;
        pimpl->_engine = engine; // Store the engine reference
        return pimpl->_setup_unix_socket();
    }



    bool OpenBSD_Daemon::start_event_loop() {
        pimpl->_kernel_queue_fd = kqueue();
        if (pimpl->_kernel_queue_fd == -1) return false;

        struct kevent change_event;
        EV_SET(&change_event, pimpl->_server_socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

        if (kevent(pimpl->_kernel_queue_fd, &change_event, 1, NULL, 0, NULL) == -1) return false;

        struct kevent event_list[1]; 
        
        while (true) {
            // SUSPENDED: No CPU cycles used while waiting for the "knock"
            int n_events = kevent(pimpl->_kernel_queue_fd, NULL, 0, event_list, 1, NULL);

            if (n_events > 0) {
                int32_t client_fd = accept(pimpl->_server_socket_fd, nullptr, nullptr);
                if (client_fd >= 0) {
                    pimpl->handle_client_connection(client_fd);
                }
            }
        }
        return true;
    }    



    void OpenBSD_Daemon::stop_service() {
        if (pimpl->_server_socket_fd != -1) {
            close(pimpl->_server_socket_fd);
            pimpl->_server_socket_fd = -1;

            // Clean up the socket file so the next run starts fresh
            unlink(pimpl->_active_socket_path.c_str());
        }    
    }
    


    bool OpenBSD_Daemon::apply_platform_hardening() { 
        // 1. UNVEIL FIRST: Tell the kernel exactly what files we can touch
        // Unveil the model directory for reading only
        if (unveil("../models", "r") == -1) {
            std::cerr << "[Security] Unveil models failed: " << std::strerror(errno) << std::endl;
            return false;
        }

        // Unveil /tmp for the Unix socket (Read, Write, and Create/Delete)
        if (unveil("/tmp", "rwc") == -1) {
            std::cerr << "[Security] Unveil /tmp failed: " << std::strerror(errno) << std::endl;
            return false;
        }


        // Lock the unveil list (No more files can be added to our reality)
        if (unveil(NULL, NULL) == -1) return false;


        // 2. PLEDGE SECOND: Restrict our syscalls
        // stdio: logs, kqueue
        // unix: sockets, getpeereid
        // rpath: reading the model file
        // cpath: unlinking the socket file in stop_service()

        // OpenBSD Security Foundation: Restrict syscalls
        // stdio: logging, unix: socket comms, rpath: reading GGUF models 
        if(pledge("stdio unix rpath cpath", NULL) == -1) {
            std::cerr << "[Security] Pledge failed: " << std::strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
    

    
    
}  // namespace nanshe::platform::openbsd
