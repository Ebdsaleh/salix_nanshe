/* src/client/main.cpp */
#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

constexpr uint64_t _SALIX_MAGIC_HEADER = 0x53414C49585F4441;

void send_prompt_and_get_response(const std::string& socket_path,  const std::string& prompt) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[CLI] Socket creation failed." << std::endl;
        return;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);


    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[CLI] Could not connect to Nanshe-d at " << socket_path << std::endl;
        close(sock);
        return;
    }

    // 1. Handshake
    write(sock, &_SALIX_MAGIC_HEADER, sizeof(_SALIX_MAGIC_HEADER));

    // 2. Send the Prompt
    write(sock, prompt.c_str(), prompt.length());

    // 3. Receive the Response
    char buffer[4096];  // Larger buffer for AI responses
    std::cout << "\033[1;32mNanshe>\033[0m ";  // Green prompt
    
    ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        std::cout << buffer << std::endl;
    }

    close(sock);
}


int main() {
    const std::string socket_path = "/tmp/nanshe.sock";
    std::string user_input;

    std::cout << "--- Salix Nanshe Interactive Client ---" << std::endl;
    std::cout << "Type 'exit' or 'quit' to close." << std::endl;

    while (true) {
        std::cout << "\n\033[1;34mYou>\033[0m "; // Blue prompt
        if (!std::getline(std::cin, user_input) || user_input == "exit" || user_input == "quit") {
            break;
        }

        if (user_input.empty()) continue;

        send_prompt_and_get_response(socket_path, user_input);
    }

    std::cout << "Closing session. Stay efficient." << std::endl;
    return 0;
}
