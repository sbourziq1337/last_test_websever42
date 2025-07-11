#include "server.hpp"
void ft_error(const char *msg)
{
    perror(msg);
    exit(1);
}

void make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl - get");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl - set");
        exit(EXIT_FAILURE);
    }
}

int create_socket()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
        ft_error("Socket creation failed");

    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        ft_error("setsockopt failed");

    make_nonblocking(socket_fd);
    return socket_fd;
}

// void setup_server_address(sockaddr_in &serv_add, int port)
// {
//     serv_add.sin_family = AF_INET;
//     serv_add.sin_addr.s_addr = INADDR_ANY;
//     serv_add.sin_port = htons(port);
// }

#include <netdb.h>      // getaddrinfo
#include <sys/socket.h> // AF_INET, SOCK_STREAM
#include <netinet/in.h> // sockaddr_in
#include <cstring>      // memset, strerror
#include <iostream>
#include <cstdlib> // freeaddrinfo

bool setup_server_address(struct sockaddr_in &serv_addr, const std::string &ip, int port)
{
    struct addrinfo hints, *res = NULL;

    std::memset(&hints, 0, sizeof(hints));
     hints.ai_family = AF_UNSPEC;       // IPv4 only
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // For wildcard IPs like "0.0.0.0"

    // Use getaddrinfo to resolve the IP
    int ret = getaddrinfo(ip.c_str(), NULL, &hints, &res);
    if (ret != 0 || !res)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(ret) << std::endl;
        return false;
    }

    // Copy resolved address into sockaddr_in
    struct sockaddr_in *addr_in = reinterpret_cast<sockaddr_in *>(res->ai_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = addr_in->sin_addr; // Only this part matters

    freeaddrinfo(res);
    return true;
}

void bind_and_listen(int socket_fd, sockaddr_in &serv_add)
{
    if (bind(socket_fd, (struct sockaddr *)&serv_add, sizeof(serv_add)) < 0)
        ft_error("Binding failed");

    if (listen(socket_fd, 5) < 0)
        ft_error("Listening failed");
}

int setup_server_socket(Request &global_obj)
{
    sockaddr_in serv_add;
    // std::cout
    // setup_server_address(serv_add, global_obj.server.port);
    setup_server_address(serv_add, "0.0.0.0", global_obj.server.port);

    int socket_fd = create_socket();
    // ssss 1
    bind_and_listen(socket_fd, serv_add);

    return socket_fd;
}

int accept_client(int socket_fd)
{
    sockaddr_in cli_add;
    socklen_t cli_len = sizeof(cli_add);
    int new_socket = accept(socket_fd, (struct sockaddr *)&cli_add, &cli_len);
    if (new_socket < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("Accept failed");
    }
    return new_socket;
}
// Initialize server configuration
bool initialize_server_config(std::vector<Request> &global_obj, 
                              std::map<std::string, std::vector<size_t> > &hostport_to_indexes)
{
    std::vector<ServerConfig> all_servers = check_configfile();
    if (all_servers.empty())
    {
        std::cerr << "Error: No server configurations found" << std::endl;
        return false;
    }

    global_obj.resize(all_servers.size());

    for (size_t i = 0; i < all_servers.size(); ++i)
    {
        all_type(global_obj[i].mimitype);
        global_obj[i].server = all_servers[i];
        global_obj[i].local_data = all_servers[i].locations;
        global_obj[i].root = all_servers[i].root;

        std::ostringstream oss;
        oss << all_servers[i].host << ":" << all_servers[i].port;
        hostport_to_indexes[oss.str()].push_back(i);
        std::cout << "=======> " << oss.str() << std::endl;
    }

    std::cout << "Loaded " << all_servers.size() << " server configuration(s)." << std::endl;
    return true;
}