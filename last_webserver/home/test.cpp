// #include "server.hpp"

// void ft_error(const char *msg) {
//     perror(msg);
//     exit(1);
// }

// void make_nonblocking(int fd) {
//     int flags = fcntl(fd, F_GETFL, 0);
//     if (flags == -1) {
//         perror("fcntl - get");
//         exit(EXIT_FAILURE);
//     }
    
//     if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
//         perror("fcntl - set");
//         exit(EXIT_FAILURE);
//     }
// }

// int create_socket() {
//     int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (socket_fd < 0)
//         ft_error("Socket creation failed");
    
//     int opt = 1;
//     if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
//         ft_error("setsockopt failed");
    
//     make_nonblocking(socket_fd);
//     return socket_fd;
// }

// void setup_server_address(sockaddr_in &serv_add, int port) {
//     serv_add.sin_family = AF_INET;
//     serv_add.sin_addr.s_addr = INADDR_ANY;
//     serv_add.sin_port = htons(port);
// }

// void bind_and_listen(int socket_fd, sockaddr_in &serv_add, int port) {
//     if (bind(socket_fd, (struct sockaddr *)&serv_add, sizeof(serv_add)) < 0)
//         ft_error("Binding failed");
    
//     if (listen(socket_fd, 5) < 0)
//         ft_error("Listening failed");
    
//     std::cout << "Server is running on port " << port << "...\n";
// }

// int setup_server_socket(Request &server_config) {
//     sockaddr_in serv_add;
//     setup_server_address(serv_add, server_config.server.port);
    
//     int socket_fd = create_socket();
//     bind_and_listen(socket_fd, serv_add, server_config.server.port);
    
//     return socket_fd;
// }

// int accept_client(int socket_fd) {
//     sockaddr_in cli_add;
//     socklen_t cli_len = sizeof(cli_add);
//     int new_socket = accept(socket_fd, (struct sockaddr *)&cli_add, &cli_len);
//     if (new_socket < 0) {
//         if (errno != EAGAIN && errno != EWOULDBLOCK)
//             perror("Accept failed");
//     }
//     return new_socket;
// }

// // Initialize server configuration
// bool initialize_server_config(std::vector<Request> &global_obj) {
//     std::vector<ServerConfig> all_servers = check_configfile();
//     if (all_servers.empty()) {
//         std::cerr << "Error: No server configurations found" << std::endl;
//         return false;
//     }
    
//     global_obj.resize(all_servers.size());
    
//     for (size_t i = 0; i < all_servers.size(); ++i) {
//         all_type(global_obj[i].mimitype);
//         global_obj[i].server = all_servers[i];
//         global_obj[i].local_data = all_servers[i].locations;
//         global_obj[i].root = all_servers[i].root;
//     }
    
//     return true;
// }

// // Structure to hold server socket and its associated config
// struct ServerInfo {
//     int socket_fd;
//     size_t config_index;
    
//     ServerInfo() : socket_fd(-1), config_index(0) {}
//     ServerInfo(int fd, size_t idx) : socket_fd(fd), config_index(idx) {}
// };

// // Find which server a socket belongs to
// size_t find_server_by_socket(const std::vector<ServerInfo> &servers, int socket_fd) {
//     for (size_t i = 0; i < servers.size(); ++i) {
//         if (servers[i].socket_fd == socket_fd) {
//             return servers[i].config_index;
//         }
//     }
//     return SIZE_MAX; // Not found
// }

// // Main server loop
// int main() {
//     std::vector<Request> global_obj;
    
//     if (!initialize_server_config(global_obj)) {
//         return 1;
//     }
    
//     std::cout << "Loaded " << global_obj.size() << " server configuration(s):" << std::endl;
    
//     // Create all server sockets
//     std::vector<ServerInfo> servers;
//     servers.reserve(global_obj.size());
    
//     for (size_t i = 0; i < global_obj.size(); ++i) {
//         std::cout << "Setting up server " << i << " on port " << global_obj[i].server.port << std::endl;
//         int socket_fd = setup_server_socket(global_obj[i]);
//         servers.push_back(ServerInfo(socket_fd, i));
//     }
    
//     // Create a single epoll instance for all servers
//     int epfd = epoll_create1(0);
//     if (epfd == -1) {
//         perror("epoll_create1 failed");
//         // Cleanup sockets
//         for (size_t i = 0; i < servers.size(); ++i) {
//             close(servers[i].socket_fd);
//         }
//         return 1;
//     }
    
//     // Add all server sockets to epoll
//     for (size_t i = 0; i < servers.size(); ++i) {
//         struct epoll_event ev;
//         ev.events = EPOLLIN;
//         ev.data.fd = servers[i].socket_fd;
        
//         if (epoll_ctl(epfd, EPOLL_CTL_ADD, servers[i].socket_fd, &ev) == -1) {
//             perror("epoll_ctl: server socket");
//             close(epfd);
//             for (size_t j = 0; j < servers.size(); ++j) {
//                 close(servers[j].socket_fd);
//             }
//             return 1;
//         }
//     }
    
//     std::map<int, ClientInfo> clients;
    
//     std::cout << "All servers started. Waiting for connections..." << std::endl;
    
//     while (true) {
//         struct epoll_event events[MAX_EVENTS];
//         int nfds = epoll_wait(epfd, events, MAX_EVENTS, 2000);
        
//         if (nfds < 0) {
//             perror("epoll_wait failed");
//             continue;
//         }
        
//         for (int i = 0; i < nfds; i++) {
//             int fd = events[i].data.fd;
            
//             // Check if this is a server socket (new connection)
//             size_t server_idx = find_server_by_socket(servers, fd);
//             if (server_idx != SIZE_MAX && (events[i].events & EPOLLIN)) {
//                 // Handle new connection for this specific server
//                 global_obj[server_idx].fd_client = fd;
//                 handle_new_connections(fd, epfd, clients, global_obj[server_idx]);
//             }
//             else if (events[i].events & EPOLLIN) {
//                 // Handle existing client connection
//                 std::map<int, ClientInfo>::iterator client_it = clients.find(fd);
//                 if (client_it != clients.end() && client_it->second.is_active) {
//                     // Find which server this client belongs to
//                     // You might need to store server association with each client
//                     // For now, we'll use the first server as default
//                     // You should modify ClientInfo to store server_index
//                     size_t client_server_idx = 0; // Default to first server
                    
//                     // If you have server association stored in ClientInfo:
//                     // size_t client_server_idx = client_it->second.server_index;
                    
//                     global_obj[client_server_idx].fd_client = fd;
//                     handle_request_chunked(fd, client_it->second, global_obj[client_server_idx]);
//                 }
//             }
//         }
        
//         cleanup_inactive_clients(epfd, clients);
//     }
    
//     // Cleanup
//     close(epfd);
//     for (size_t i = 0; i < servers.size(); ++i) {
//         close(servers[i].socket_fd);
//     }
    
//     return 0;
// }