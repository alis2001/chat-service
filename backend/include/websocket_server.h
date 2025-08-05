#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace caffis {

class WebSocketServer {
private:
    boost::asio::io_context io_context_;
    std::vector<std::thread> thread_pool_;
    int port_;

public:
    explicit WebSocketServer(int port);
    ~WebSocketServer();
    
    void start();
    void stop();
    
private:
    void accept_connections();
    void handle_session(boost::beast::tcp_stream stream);
};

} // namespace caffis
