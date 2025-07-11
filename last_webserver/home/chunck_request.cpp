#include "server.hpp"


bool sendDataReliably(int fd, const char *data, size_t size)
{
    ssize_t total_sent = 0;
    int retry_count = 0;
    const int max_retries = 100;

    while (total_sent < static_cast<ssize_t>(size) && retry_count < max_retries)
    {
        ssize_t sent = send(fd, data + total_sent, size - total_sent, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (sent == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(100); // Brief wait for socket buffer
                retry_count++;
                continue;
            }
            perror("send failed");
            return false;
        }
        if (sent == 0)
        {
            return false; // Connection closed
        }
        total_sent += sent;
        retry_count = 0;
    }

    return (total_sent == static_cast<ssize_t>(size));
}
// Thread-safe chunk sending with non-blocking I/O support
void sendChunk(int fd, const char *data, size_t size)
{
    if (size == 0)
    {
        // Final chunk (indicates end of transmission)
        const char *finalChunk = "0\r\n\r\n";
        send(fd, finalChunk, strlen(finalChunk), MSG_NOSIGNAL);
        return;
    }

    // 1. Send the size header in hex + CRLF
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << size << "\r\n";
    std::string header = chunkHeader.str();
    send(fd, header.c_str(), header.length(), MSG_NOSIGNAL);

    // 2. Send the actual data with retry logic
    ssize_t total_sent = 0;
    int retry_count = 0;
    const int max_retries = 100;

    while (total_sent < static_cast<ssize_t>(size) && retry_count < max_retries)
    {
        ssize_t sent = send(fd, data + total_sent, size - total_sent, MSG_NOSIGNAL | MSG_DONTWAIT);

        if (sent <= 0)
        {
            usleep(100); // sleep for 0.1ms
            retry_count++;
            continue;
        }

        total_sent += sent;
        retry_count = 0; // reset on success
    }

    // 3. Send trailer (CRLF)
    send(fd, "\r\n", 2, MSG_NOSIGNAL);
}
void response_plus(std::string name_file, int fd, std::string header, std::map<std::string, std::string> &headers)
{
    std::ifstream file(name_file.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        const char *error404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
                               "<html><body><h1>404 Not Found</h1></body></html>";
        send(fd, error404, strlen(error404), MSG_NOSIGNAL);
        return;
    }

    long fileSize = getFileSize(name_file);
    if (fileSize == -1)
    {
        file.close();
        const char *error500 = "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n";
        send(fd, error500, strlen(error500), MSG_NOSIGNAL);
        return;
    }

    // Check for Range header (for video seeking support)
    std::map<std::string, std::string>::const_iterator rangeIt = headers.find("Range");
    bool isPartialContent = false;
    long start = 0, end = fileSize - 1;

    if (rangeIt != headers.end() && parseRangeHeader(rangeIt->second, fileSize, start, end))
    {
        isPartialContent = true;
    }

    // For video files, support partial content requests
    std::string contentType = getContentType(name_file);
    bool isVideo = (contentType.find("video/") == 0);

    std::ostringstream oss;

    if (isPartialContent)
    {
        //oss << "HTTP/1.1 206 Partial Content\r\n";
        oss << "Content-Range: bytes " << start << "-" << end << "/" << fileSize << "\r\n";
        oss << "Content-Length: " << (end - start + 1) << "\r\n";
        oss << "Accept-Ranges: bytes\r\n";
    }
    else if (isVideo)
    {
        // For video files, always include Accept-Ranges even for full content
        //oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Length: " << fileSize << "\r\n";
        oss << "Accept-Ranges: bytes\r\n";
    }
    else
    {
        // Use chunked encoding for non-video files
       // oss << "HTTP/1.1 200 OK\r\n";
        oss << "Transfer-Encoding: chunked\r\n";
    }

    oss << "Connection: close\r\n\r\n";
    header += oss.str();

    const char *new_head = header.c_str();
    send(fd, new_head, strlen(new_head), MSG_NOSIGNAL);

    // Seek to start position for partial content
    if (isPartialContent || start > 0)
    {
        file.seekg(start);
    }

    long remainingBytes = end - start + 1;
    char buffer[BUFFER_SIZE];

    // Enhanced sending logic with better error handling for concurrent clients
    if (isVideo && !isPartialContent)
    {
        // For full video content, send with Content-Length (no chunking)
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
        {
            size_t bytesRead = file.gcount();

            if (!sendDataReliably(fd, buffer, bytesRead))
            {
                file.close();
                return;
            }
        }
    }
    else if (isPartialContent)
    {
        // Send partial content with Content-Length
        while (remainingBytes > 0 && (file.read(buffer, std::min(static_cast<long>(sizeof(buffer)), remainingBytes)) || file.gcount() > 0))
        {
            size_t bytesRead = file.gcount();
            remainingBytes -= bytesRead;

            if (!sendDataReliably(fd, buffer, bytesRead))
            {
                file.close();
                return;
            }
        }
    }
    else
    {
        // Use chunked encoding for other files
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
        {
            size_t bytesRead = file.gcount();
            sendChunk(fd, buffer, bytesRead);
        }

        // Send final chunk to indicate end of transfer
        sendChunk(fd, NULL, 0);
    }

    file.close();
}

// Overloaded version with empty headers map as default
void response(std::string name_file, int fd, std::string header)
{
    std::map<std::string, std::string> empty_headers;
    response_plus(name_file, fd, header, empty_headers);
}

// POST response function with enhanced concurrent handling
void response_post(std::string name_file, int fd, std::string header)
{
    std::ifstream file(name_file.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        // Send upload success page if file doesn't exist
        std::string success_html =
            "<html><body>"
            "<h1>Upload Successful!</h1>"
            "<p>File has been uploaded successfully.</p>"
            "<a href='/'>Go back</a>"
            "</body></html>";

        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: text/html\r\n";
        oss << "Transfer-Encoding: chunked\r\n";
        oss << "Connection: close\r\n\r\n";

        std::string full_header = oss.str();
        send(fd, full_header.c_str(), full_header.length(), MSG_NOSIGNAL);

        sendChunk(fd, success_html.c_str(), success_html.length());
        sendChunk(fd, NULL, 0); // End chunked transfer
        return;
    }

    // Send existing file with chunked encoding
    std::ostringstream oss;
    oss << "Transfer-Encoding: chunked\r\n";
    oss << "Connection: close\r\n\r\n";
    header += oss.str();

    const char *new_head = header.c_str();
    send(fd, new_head, strlen(new_head), MSG_NOSIGNAL);

    char buffer[BUFFER_SIZE];

    // Read and send file in chunks with enhanced error handling
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        size_t bytesRead = file.gcount();
        sendChunk(fd, buffer, bytesRead);
    }

    // Send final chunk to indicate end of transfer
    sendChunk(fd, NULL, 0);

    file.close();
}