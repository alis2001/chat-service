#pragma once

#include <string>

namespace caffis {
namespace config {

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 5002;
    int max_connections = 10000;
    int thread_pool_size = 8;
};

struct DatabaseConfig {
    std::string connection_string;
};

struct RedisConfig {
    std::string host = "redis";
    int port = 6379;
};

} // namespace config
} // namespace caffis
