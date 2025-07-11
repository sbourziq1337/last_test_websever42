// //#include "server.hpp"
// #include <iostream>
// #include <fstream>
// #include <map>
// #include <cstring>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <cstdlib>
// #include <sstream>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <sys/epoll.h>

// #define PORT 8080
// #define MAX_EVENTS 1024

// void ft_error(const char *msg)
// {
//     perror(msg);
//     exit(1);
// }

// void make_nonblocking(int fd)
// {
//     int flags = fcntl(fd, F_GETFL, 0);
//     if (flags == -1)
//     {
//         perror("fcntl - get");
//         exit(EXIT_FAILURE);
//     }

//     if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
//     {
//         perror("fcntl - set");
//         exit(EXIT_FAILURE);
//     }
// }

// int setup_server_socket()
// {
//     sockaddr_in serv_add;
//     serv_add.sin_family = AF_INET;
//     serv_add.sin_addr.s_addr = INADDR_ANY;
//     serv_add.sin_port = htons(PORT);

//     int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (socket_fd < 0)
//         ft_error("Socket creation failed");

//     int opt = 1;
//     if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
//         ft_error("setsockopt failed");

//     // Make server socket non-blocking
//     make_nonblocking(socket_fd);

//     if (bind(socket_fd, (struct sockaddr *)&serv_add, sizeof(serv_add)) < 0)
//         ft_error("Binding failed");

//     if (listen(socket_fd, 5) < 0)
//         ft_error("Listening failed");

//     std::cout << "Server is running on port " << PORT << "...\n";
//     return socket_fd;
// }

// int accept_client(int socket_fd)
// {
//     sockaddr_in cli_add;
//     socklen_t cli_len = sizeof(cli_add);
//     int new_socket = accept(socket_fd, (struct sockaddr *)&cli_add, &cli_len);
//     if (new_socket < 0)
//     {
//         if (errno != EAGAIN && errno != EWOULDBLOCK)
//             perror("Accept failed");
//     }
//     return new_socket;
// }

// std::string read_headers(int new_socket, std::string &partial, ssize_t &content_length)
// {
//     char buffer[BUFFER_SIZE];
//     ssize_t bytes_read;
//     std::string headers;

//     // For non-blocking sockets, we need to handle EAGAIN/EWOULDBLOCK
//     while (true)
//     {
//         bytes_read = read(new_socket, buffer, sizeof(buffer));
//         if (bytes_read > 0)
//         {
//             partial.append(buffer, bytes_read);
//             size_t header_end = partial.find("\r\n\r\n");
//             if (header_end != std::string::npos)
//             {
//                 headers = partial.substr(0, header_end);
//                 std::istringstream stream(headers);
//                 std::string header_line;
//                 while (std::getline(stream, header_line))
//                 {
//                     // Remove carriage return if present
//                     if (!header_line.empty() && header_line[header_line.size() - 1] == '\r')
//                         header_line.erase(header_line.size() - 1);
                    
//                     size_t pos = header_line.find(":");
//                     if (pos != std::string::npos)
//                     {
//                         std::string key = header_line.substr(0, pos);
//                         std::string value = header_line.substr(pos + 1);
                        
//                         // Trim whitespace from value
//                         size_t start = value.find_first_not_of(" \t");
//                         if (start != std::string::npos)
//                             value = value.substr(start);
                        
//                         if (key == "Content-Length")
//                             content_length = std::atoll(value.c_str());
//                     }
//                 }
//                 partial = partial.substr(header_end + 4); // remove headers
//                 break;
//             }
//         }
//         else if (bytes_read == 0)
//         {
//             // Connection closed by client
//             break;
//         }
//         else if (bytes_read < 0)
//         {
//             if (errno == EAGAIN || errno == EWOULDBLOCK)
//             {
//                 // No more data available right now, but connection is still open
//                 // In edge-triggered mode, we should return what we have
//                 break;
//             }
//             else
//             {
//                 perror("Read failed");
//                 break;
//             }
//         }
//     }
    
//     return headers;
// }

// void parse_headers(std::istringstream &stream, std::map<std::string, std::string> &headers)
// {
//     std::string line;
//     while (std::getline(stream, line))
//     {
//         // Remove carriage return if present
//         if (!line.empty() && line[line.size() - 1] == '\r')
//             line.erase(line.size() - 1);
            
//         if (line.empty())
//             break;
            
//         size_t pos = line.find(":");
//         if (pos != std::string::npos && pos > 0)
//         {
//             std::string key = line.substr(0, pos);
//             std::string value = line.substr(pos + 1);
            
//             // Trim whitespace from value
//             size_t start = value.find_first_not_of(" \t");
//             if (start != std::string::npos)
//                 value = value.substr(start);
            
//             headers[key] = value;
//         }
//     }
// }

// void handle_post_request(Request &obj, std::map<std::string, std::string> &headers, std::string &partial, int &new_socket, ssize_t &content_length)
// {
//     ssize_t total_written = partial.size();
//     char buffer[BUFFER_SIZE];
//     ssize_t bytes_read;

//     if ((obj.size1 = partial.find("\r\n\r\n")) != std::string::npos)
//     {
//         obj.info_body = partial.substr(0, obj.size1);
//         partial = partial.substr(obj.size1 + 4);
//     }
//     else
//     {
//         while (total_written < content_length && (bytes_read = read(new_socket, buffer, sizeof(buffer))) > 0)
//         {
//             partial.append(buffer, bytes_read);
//             if ((obj.size1 = partial.find("\r\n\r\n")) != std::string::npos)
//             {
//                 obj.info_body = partial.substr(0, obj.size1);
//                 partial = partial.substr(obj.size1 + 4);
//                 total_written += bytes_read;
//                 break;
//             }
//             total_written += bytes_read;
//         }
//     }
    
//     if (headers.count("Content-Type"))
//     {
//         static std::map<std::string, std::string> post_res;
//         std::map<std::string, std::string> form_data;
//         if (headers["Content-Type"].find("application/x-www-form-urlencoded") != std::string::npos)
//         {
//             std::istringstream ss(partial);
//             std::string pair;
//             while (std::getline(ss, pair, '&'))
//             {
//                 size_t eq = pair.find('=');
//                 if (eq != std::string::npos)
//                 {
//                     std::string key = pair.substr(0, eq);
//                     std::string value = pair.substr(eq + 1);
//                     form_data[key] = value;
//                 }
//             }

//             Format_urlencoded(obj.path, new_socket, post_res, form_data, obj);
//             return;
//         }
//     }
    
//     if (obj.info_body.empty())
//     {
//         std::string header = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\n\r\n";
//         response_post("/home/akera/Desktop/test_wib/webserver/error_page/405.html", new_socket, header);
//         return;
//     }
    
//     // Handle multipart form data
//     size_t fn_pos = obj.info_body.find("filename=\"");
//     if (fn_pos != std::string::npos)
//     {
//         fn_pos += 10;
//         size_t fn_end = obj.info_body.find("\"", fn_pos);
//         if (fn_end == std::string::npos)
//             return;

//         obj.filename = obj.info_body.substr(fn_pos, fn_end - fn_pos);
//         obj.all_body.open(obj.filename.c_str(), std::ios::binary);
//         if (!obj.all_body.is_open())
//         {
//             std::cerr << "Failed to open file: " << obj.filename << std::endl;
//             return;
//         }
//     }

//     // Extract boundary
//     std::string boundary;
//     if (headers["Content-Type"].find("boundary=") != std::string::npos)
//     {
//         size_t boundary_pos = headers["Content-Type"].find("boundary=") + 9;
//         size_t boundary_end = headers["Content-Type"].find(";", boundary_pos);
//         if (boundary_end == std::string::npos)
//             boundary_end = headers["Content-Type"].length();
//         boundary = "--" + headers["Content-Type"].substr(boundary_pos, boundary_end - boundary_pos);
//     }

//     ssize_t last_pos;
//     if (!boundary.empty() && (last_pos = partial.find(boundary)) != std::string::npos)
//     {
//         partial = partial.substr(0, last_pos);
//         obj.all_body.write(partial.data(), partial.size());
//     }
//     else
//     {
//         obj.all_body.write(partial.data(), partial.size());
//         while (total_written < content_length && (bytes_read = read(new_socket, buffer, sizeof(buffer))) > 0)
//         {
//             std::string data(buffer, bytes_read);
//             if (!boundary.empty())
//             {
//                 ssize_t pos = data.find(boundary);
//                 if (pos == std::string::npos)
//                 {
//                     obj.all_body.write(buffer, bytes_read);
//                     total_written += bytes_read;
//                 }
//                 else
//                 {
//                     obj.all_body.write(data.c_str(), pos);
//                     total_written += pos;
//                     break;
//                 }
//             }
//             else
//             {
//                 obj.all_body.write(buffer, bytes_read);
//                 total_written += bytes_read;
//             }
//         }
//     }

//     obj.all_body.close();
//     std::string header = "HTTP/1.1 200 OK\r\nContent-Type: " + getContentType(obj.path) + "\r\n\r\n";
//     response_post(obj.path, new_socket, header);
// }

// std::string getmine_type(std::string line_path, std::map<std::string, std::string> mime)
// {
//     std::string::size_type pos = line_path.rfind(".");
//     std::string content_type;
//     if (pos != std::string::npos)
//     {
//         std::string type = line_path.substr(pos);
//         if (mime.count(type))
//         {
//             content_type = mime.find(type)->second;
//             return content_type;
//         }
//     }
//     return "application/octet-stream";
// }

// void handle_request(int new_socket, Request &obj)
// {
//     if (new_socket < 0)
//         return;

//     std::string headers, partial;
//     ssize_t content_length = -1;

//     headers = read_headers(new_socket, partial, content_length);
    
//     if (headers.empty())
//     {
//         std::cerr << "Failed to read headers" << std::endl;
//         close(new_socket);
//         return;
//     }
    
//     std::istringstream method_stream(headers);
//     std::string method_line;
//     if (!std::getline(method_stream, method_line))
//     {
//         perror("Failed to read method line");
//         close(new_socket);
//         return;
//     }
    
//     parsing_method(obj, method_line);
//     std::map<std::string, std::string> head;
//     parse_headers(method_stream, head);
    
//     std::cout << "Method: " << obj.mthod << ", Path: " << obj.path << ", Version: " << obj.version << std::endl;
    
//     if (obj.mthod == "GET" && obj.version == "HTTP/1.1")
//     {
//         std::string type = getmine_type(obj.path, obj.mimitype);
//         parsing_Get(head, obj.path, new_socket, type, obj.uri, obj);
//     }
//     else if (obj.mthod == "POST" && obj.version == "HTTP/1.1")
//     {
//         std::cout << "POST request for path: " << obj.path << std::endl;
//         handle_post_request(obj, head, partial, new_socket, content_length);
//     }
//     else
//     {
//         // Send 405 Method Not Allowed for unsupported methods
//         std::string response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\n\r\n<h1>405 Method Not Allowed</h1>";
//         send(new_socket, response.c_str(), response.length(), 0);
//     }

//     close(new_socket);
// }

// int main()
// {
//     int socket_fd = setup_server_socket();
//     Request obj;
//     all_type(obj);
//     obj.stuct_data = check_configfile();
    
//     if (obj.stuct_data.empty())
//     {
//         std::cerr << "Error: No server configuration found" << std::endl;
//         close(socket_fd);
//         return 1;
//     }
    
//     obj.local_data = obj.stuct_data[0].locations;
//     obj.root = obj.stuct_data[0].root;
//     std::cout << "Server name: " << obj.stuct_data[0].locations[1].path << std::endl;

//     int epfd = epoll_create1(EPOLL_CLOEXEC);
//     if (epfd == -1)
//     {
//         perror("epoll_create1 failed");
//         close(socket_fd);
//         return 1;
//     }
    
//     struct epoll_event ev;
//     ev.events = EPOLLIN;
//     ev.data.fd = socket_fd;

//     if (epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &ev) == -1)
//     {
//         perror("epoll_ctl: add server socket");
//         close(socket_fd);
//         close(epfd);
//         return 1;
//     }
    
//     std::cout << "Server started, waiting for connections..." << std::endl;
    
//     while (true)
//     {
//         struct epoll_event events[MAX_EVENTS];
//         int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
//         if (nfds < 0)
//         {
//             perror("epoll_wait failed");
//             continue;
//         }

//         for (int i = 0; i < nfds; i++)
//         {
//             // New connection on server socket
//             if (events[i].data.fd == socket_fd && (events[i].events & EPOLLIN))
//             {
//                 // Accept all pending connections
//                 while (true)
//                 {
//                     int new_socket = accept_client(socket_fd);
//                     if (new_socket < 0)
//                         break; // No more connections to accept
                    
//                     // make_nonblocking(new_socket);

//                     struct epoll_event client_event;
//                     client_event.events = EPOLLIN; // Level-triggered (remove EPOLLET)
//                     client_event.data.fd = new_socket;

//                     if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_socket, &client_event) == -1)
//                     {
//                         perror("epoll_ctl: add client socket");
//                         close(new_socket);
//                     }
//                 }
//             }
//             // Handle client data
//             else if (events[i].events & EPOLLIN)
//             {
//                 handle_request(events[i].data.fd, obj);
//                 // Note: socket is closed in handle_request, so we should remove it from epoll
//                 epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
//             }
//         }
//     }

//     close(socket_fd);
//     close(epfd);
//     return 0;
// }