// #include "server.hpp"
// // Process request headers based on method
// bool process_request_headers(ChunkedClientInfo &client)
// {
//     std::string index_path = client.request_obj.uri;
//     bool found_method = false;
//     bool found_local = false;
//     // check for redirection
//     if (client.request_obj.found_redirection == false)
//     {
//         std::string red_path = normalize_path(client.request_obj.uri);
//         while (true)
//         {
//             for (size_t i = 0; i < client.request_obj.local_data.size(); ++i)
//             {
//                 std::string location_path = normalize_path(client.request_obj.local_data[i].path);

//                 if (red_path == location_path && !client.request_obj.local_data[i].redirection.empty())
//                 {
//                     client.request_obj.response_red = "HTTP/1.1 302 Found\r\n";
//                     client.request_obj.response_red += "Location: " + client.request_obj.local_data[i].redirection + "\r\n";
//                     client.request_obj.response_red += "Content-Length: 0\r\n";
//                     client.request_obj.response_red += "Connection: close\r\n\r\n";

//                     client.upload_state = 2;
//                     client.request_obj.found_redirection = true;
//                     client.request_obj.mthod = "Redirection";
//                     return true;
//                 }
//             }

//             red_path = remove_last_path_component(red_path);
//             if (red_path.empty() || red_path == normalize_path(client.request_obj.root))
//                 break;
//         }
//     }

//     // handle methods GET, POST, etc.
//     while (true)
//     {
//         for (size_t i = 0; i < client.request_obj.local_data.size(); i++)
//         {
//             std::string location_path = normalize_path(client.request_obj.local_data[i].path);
//             index_path = normalize_path(index_path);
//             if (index_path == location_path)
//             {
//                 found_local = true;
//                 if (client.request_obj.local_data[i].methods.empty())
//                 {
//                     found_method = true;
//                     break;
//                 }
//                 for (size_t j = 0; j < client.request_obj.local_data[i].methods.size(); j++)
//                 {
//                     if (client.request_obj.mthod == client.request_obj.local_data[i].methods[j])
//                         found_method = true;
//                 }
//                 if (found_method)
//                     break;
//             }
//         }
//         if (found_method == true || found_local == true)
//             break;
//         index_path = remove_last_path_component(index_path);
//     }
//     if (found_method == false)
//     {
//         client.upload_state = 2;
//         client.request_obj.mthod = "method not found";
//         return true;
//     }
//     if((client.request_obj.mthod == "POST"))
//     {
//         if(client.request_obj.server.client_max_body_size <= 0 || client.request_obj.server.client_max_body_size <= client.content_length)
//         {
//             std::cout  << "Client max body size is not set or invalid." << std::endl;
//             client.request_obj.mthod = "content_length";
//             client.upload_state = 2;
//             return true;
//         }
//     }
//     if (client.request_obj.mthod == "GET")
//     {
//         client.upload_state = 2;
//         return true;
//     }
//     else if (client.request_obj.mthod == "POST")
//     {
//         if (process_post_request(client))
//         {
//             if (client.upload_state != 2)
//             {
//                 client.upload_state = 1;
//                 client.bytes_read = 0;
//             }
//             return true;
//         }
//         return false;
//     }
//     else
//     {
//         client.upload_state = 2;
//         return true;
//     }
// }

// // Structure to hold server-specific information
// struct ServerInfo {
//     int socket_fd;
//     Request server_config;
//     std::map<int, ChunkedClientInfo> clients;
// };

// // Modified setup function for multiple servers (C++98 compatible)
// std::vector<ServerInfo> setup_multiple_servers(const Request &global_obj)
// {
//     const std::vector<ServerConfig> &server_configs = global_obj.server_configs;
    
//     if (server_configs.empty())
//     {
//         std::cerr << "No server configurations available" << std::endl;
//         return std::vector<ServerInfo>();
//     }
    
//     std::vector<ServerInfo> servers;
//     for (size_t i = 0; i < server_configs.size(); ++i)
//     {
//         const ServerConfig &config = server_configs[i];
//         ServerInfo server_info;
//         server_info.server_config = Request(); // Initialize with default Request
//         server_info.server_config.server = config; // Set server config
//         server_info.socket_fd = setup_server_socket(server_info.server_config);
        
//         if (server_info.socket_fd < 0)
//         {
//             std::cerr << "Failed to setup server on port " << config.port << std::endl;
//             continue; // Skip this server if setup failed
//         }
        
//         servers.push_back(server_info);
//         std::cout << "Successfully setup server on port " << config.port << std::endl;
//     }
//     return servers; // Return all successfully setup servers
// }

// // Add all server sockets to epoll
// int setup_epoll_for_multiple_servers(const std::vector<ServerInfo>& servers)
// {
//     int epfd = epoll_create1(0);
//     if (epfd == -1)
//     {
//         perror("epoll_create1 failed");
//         return -1;
//     }
    
//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         struct epoll_event ev;
//         ev.events = EPOLLIN;
//         ev.data.fd = servers[i].socket_fd;
        
//         if (epoll_ctl(epfd, EPOLL_CTL_ADD, servers[i].socket_fd, &ev) == -1)
//         {
//             perror("epoll_ctl: server socket");
//             close(epfd);
//             return -1;
//         }
//     }
    
//     return epfd;
// }

// // Find which server a socket belongs to
// ServerInfo* find_server_by_socket(std::vector<ServerInfo>& servers, int socket_fd)
// {
//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         if (servers[i].socket_fd == socket_fd)
//         {
//             return &servers[i];
//         }
//     }
//     return NULL;
// }

// // Find which server a client belongs to
// ServerInfo* find_server_by_client(std::vector<ServerInfo>& servers, int client_fd)
// {
//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         std::map<int, ChunkedClientInfo>::iterator it = servers[i].clients.find(client_fd);
//         if (it != servers[i].clients.end())
//         {
//             return &servers[i];
//         }
//     }
//     return NULL;
// }

// // Modified handle_new_connections for specific server
// void handle_new_connections_for_server(ServerInfo& server_info, int epfd)
// {
//     while (true)
//     {
//         int new_socket = accept_client(server_info.socket_fd);
//         if (new_socket <= 0)
//             break;
            
//         make_nonblocking(new_socket);
        
//         struct epoll_event ev;
//         ev.events = EPOLLIN | EPOLLET;
//         ev.data.fd = new_socket;
        
//         if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_socket, &ev) == -1)
//         {
//             perror("epoll_ctl: client socket");
//             close(new_socket);
//             continue;
//         }
        
//         // Properly initialize client info for this server
//         ChunkedClientInfo client_info;
//         client_info.is_active = true;
//         client_info.upload_state = 0;
//         client_info.bytes_read = 0;
//         client_info.content_length = 0;
        
//         // Properly initialize the request object with server config
//         client_info.request_obj.server = server_info.server_config.server;
//         client_info.request_obj.local_data = server_info.server_config.server.locations;
//         client_info.request_obj.root = server_info.server_config.server.root;
//         client_info.request_obj.found_redirection = false;
        
//         server_info.clients[new_socket] = client_info;
        
//         std::cout << "New client connected to server on port " 
//                   << server_info.server_config.server.port 
//                   << ", fd: " << new_socket << std::endl;
//     }
// }

// // Initialize multiple server configurations
// bool initialize_multiple_server_configs(Request &global_obj)
// {
//     // Parse your config file to get all server blocks
//     std::vector<ServerConfig> all_servers = check_configfile(); // Your existing function
    
//     if (all_servers.empty())
//     {
//         std::cerr << "Error: No server configurations found" << std::endl;
//         return false;
//     }
    
//     global_obj.server_configs = all_servers;
//     return true;
// }

// // Cleanup inactive clients for all servers
// void cleanup_all_inactive_clients(int epfd, std::vector<ServerInfo>& servers)
// {
//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         cleanup_inactive_clients(epfd, servers[i].clients);
//     }
// }

// // Main function - updated for multiple servers
// int main()
// {
//     Request global_obj;
    
//     // Initialize configurations for all servers
//     if (!initialize_multiple_server_configs(global_obj))
//     {
//         std::cerr << "Failed to initialize server configurations" << std::endl;
//         return 1;
//     }
    
//     if (global_obj.server_configs.empty())
//     {
//         std::cerr << "No server configurations available" << std::endl;
//         return 1;
//     }
    
//     // Safe way to print server configurations
//     std::cout << "Loaded " << global_obj.server_configs.size() << " server configuration(s):" << std::endl;
//     for (size_t i = 0; i < global_obj.server_configs.size(); ++i)
//     {
//         std::cout << "Server " << i << " port: " << global_obj.server_configs[i].port << std::endl;
//     }

//     // Setup all servers
//     std::vector<ServerInfo> servers = setup_multiple_servers(global_obj);
//     if (servers.empty())
//     {
//         std::cerr << "Failed to setup any servers" << std::endl;
//         return 1;
//     }
    
//     // Setup epoll for all servers
//     int epfd = setup_epoll_for_multiple_servers(servers);
//     if (epfd == -1)
//     {
//         // Close all server sockets
//         for (size_t i = 0; i < servers.size(); ++i)
//         {
//             close(servers[i].socket_fd);
//         }
//         return 1;
//     }
    
//     std::cout << "All servers started successfully, waiting for connections..." << std::endl;
    
//     // Main event loop
//     while (true)
//     {
//         struct epoll_event events[MAX_EVENTS];
//         int nfds = epoll_wait(epfd, events, MAX_EVENTS, 2000);
        
//         if (nfds < 0)
//         {
//             perror("epoll_wait failed");
//             continue;
//         }
        
//         for (int i = 0; i < nfds; i++)
//         {
//             int fd = events[i].data.fd;
            
//             // Check if it's a server socket (new connection)
//             ServerInfo* server = find_server_by_socket(servers, fd);
//             if (server != NULL && (events[i].events & EPOLLIN))
//             {
//                 handle_new_connections_for_server(*server, epfd);
//             }
//             // Handle client data
//             else if (events[i].events & EPOLLIN)
//             {
//                 ServerInfo* client_server = find_server_by_client(servers, fd);
//                 if (client_server != NULL)
//                 {
//                     client_server->server_config.fd_client = fd;
//                     std::map<int, ChunkedClientInfo>::iterator client_it = 
//                         client_server->clients.find(fd);
                    
//                     if (client_it != client_server->clients.end() && 
//                         client_it->second.is_active)
//                     {
//                         handle_request_chunked(fd, client_it->second, 
//                                              client_server->server_config);
//                     }
//                 }
//             }
//             // Handle error events
//             else if (events[i].events & (EPOLLERR | EPOLLHUP))
//             {
//                 std::cout << "Error or hangup on fd: " << fd << std::endl;
//                 ServerInfo* client_server = find_server_by_client(servers, fd);
//                 if (client_server != NULL)
//                 {
//                     client_server->clients.erase(fd);
//                 }
//                 epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
//                 close(fd);
//             }
//         }
        
//         // Cleanup inactive clients from all servers
//         cleanup_all_inactive_clients(epfd, servers);
//     }
    
//     // Cleanup
//     close(epfd);
//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         close(servers[i].socket_fd);
//     }
    
//     return 0;
// }