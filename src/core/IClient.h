/* src/core/IClient.h */
#ifndef SALIX_NANSHE_ICLIENT_H
#define SALIX_NANSHE_ICLIENT_H

#include <string>


namespace nanshe::core {


    class IClient {
    public:
        virtual ~IClient() = default;

        virtual bool connect_to_daemon(const std::string& socket_path) = 0;
        virtual void transmit_request(const std::string& raw_input) = 0;
        virtual std::string receive_stream_response() = 0;
        virtual void disconnect() = 0;
    };

}  // namespace nanshe::core 

#endif
