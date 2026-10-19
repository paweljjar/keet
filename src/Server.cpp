//
// Created by PAWEL on 15/10/2026.
//

#include "keet/Server.h"
#include "keet/Json.h"
#include "keet/Crypto.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <ranges>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

namespace HTTPServer {
    bool ServerRequest::hasQuery(const std::string &key) const { // Check if a query parameter exists
        return query.contains(key); // Use the contains method of std::map to check for the key
    }

    std::string ServerRequest::getQuery(const std::string &key, const std::string &fallback) const { // Get the value of a query parameter, or return a fallback if it doesn't exist
        const auto it = query.find(key); // Use the find method of std::map to look for the key
        return it != query.end() ? it->second : fallback; // If the key is found, return its value; otherwise, return the fallback
    }

    std::string ServerRequest::getHeader(const std::string& key, const std::string& fallback) const { // Get the value of a header (case-insensitively), or return a fallback if it doesn't exist
        std::string lowerKey = key;
        for (char& c : lowerKey) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        const auto it = headers.find(lowerKey); // Use the find method of std::map to look for the key
        return it != headers.end() ? it->second : fallback; // If the key is found, return its value; otherwise, return the fallback
    }

    std::string ServerRequest::getCookie(const std::string& name, const std::string& fallback) const { // Retrieve a cookie value by name from the Cookie header, returning fallback if not found
        const std::string cookieHeader = getHeader("cookie");
        if (cookieHeader.empty()) {
            return fallback;
        }

        size_t start = 0;
        while (start < cookieHeader.size()) {
            size_t end = cookieHeader.find(';', start);
            if (end == std::string::npos) {
                end = cookieHeader.size();
            }
            std::string pair = cookieHeader.substr(start, end - start);
            const size_t first = pair.find_first_not_of(" \t");
            const size_t last = pair.find_last_not_of(" \t");
            if (first != std::string::npos && last != std::string::npos) {
                pair = pair.substr(first, last - first + 1);
            } else {
                pair.clear();
            }

            const size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                const std::string k = pair.substr(0, eq);
                const std::string v = pair.substr(eq + 1);
                if (k == name) {
                    return v;
                }
            }
            start = end + 1;
        }
        return fallback;
    }

    bool ServerRequest::isWebSocketUpgrade() const { // Check if the request is an RFC 6455 WebSocket upgrade handshake
        if (method != "GET") {
            return false;
        }
        std::string upgrade = getHeader("upgrade");
        for (char& c : upgrade) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (upgrade != "websocket") {
            return false;
        }
        std::string conn = getHeader("connection");
        for (char& c : conn) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return conn.find("upgrade") != std::string::npos;
    }

    void ServerResponse::setHeader(std::string name, std::string value) { // Set or overwrite an outgoing HTTP response header
        headers[std::move(name)] = std::move(value);
    }

    std::string ServerResponse::getHeader(const std::string& name, const std::string& fallback) const { // Retrieve an outgoing response header value
        for (const auto& [k, v] : headers) {
            if (k.size() == name.size()) {
                bool match = true;
                for (size_t i = 0; i < k.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(k[i])) != std::tolower(static_cast<unsigned char>(name[i]))) {
                        match = false;
                        break;
                    }
                }
                if (match) return v;
            }
        }
        return fallback;
    }

    bool ServerResponse::hasHeader(const std::string& name) const { // Check case-insensitively if an outgoing response header exists
        for (const auto& [k, _] : headers) {
            if (k.size() == name.size()) {
                bool match = true;
                for (size_t i = 0; i < k.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(k[i])) != std::tolower(static_cast<unsigned char>(name[i]))) {
                        match = false;
                        break;
                    }
                }
                if (match) return true;
            }
        }
        return false;
    }

    std::string generateSessionId() { // Generate a pseudo-random unique session identifier string
        thread_local std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist;
        const uint64_t p1 = dist(rng);
        const uint64_t p2 = dist(rng);
        std::ostringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(16) << p1
           << std::setw(16) << p2;
        return ss.str();
    }

    std::string extractOrCreateSessionId(const ServerRequest& req, const std::string& cookieName) { // Extract session ID from request headers/cookies with fallback generation
        std::string sid = req.getHeader("x-session-id");
        if (!sid.empty()) {
            return sid;
        }
        sid = req.getCookie(cookieName);
        if (!sid.empty()) {
            return sid;
        }
        return generateSessionId();
    }

    //region public methods
    Server::Server(const int port, std::string name, const size_t threadPoolSize) : _port(port), _name(std::move(name)), _server_fd(-1), _address{}, _wsWakePipe{-1, -1} { // Constructor for the Server class, initializes the port, optional name, and worker pool configuration
        if (port < 0 || port > 65535) { // Validate the port number to ensure it's within the valid range for TCP/UDP ports
            throw std::invalid_argument("Port number must be between 0 and 65535"); // If the port number is invalid, throw an exception to indicate the error
        }
        if (threadPoolSize == 0) {
            const unsigned int hw = std::thread::hardware_concurrency();
            _threadPoolSize = hw > 0 ? std::max(4u, hw) : 8;
        } else {
            _threadPoolSize = threadPoolSize;
        }
    }

    Server::~Server() { // Destructor ensuring listener socket is closed and worker threads are joined
        stop();
    }

    void Server::stop() { // Stop server accept loop and shut down worker threads
        if (!_running.exchange(false)) {
            return;
        }
        if (_server_fd != -1) {
            ::shutdown(_server_fd, SHUT_RDWR);
            ::close(_server_fd);
            _server_fd = -1;
        }
        _stopWsPollLoop();
        _stopThreadPool();
        _log("Server stopped");
    }

    size_t Server::getThreadPoolSize() const { // Return configured worker pool thread count
        return _threadPoolSize;
    }

    void Server::setThreadPoolSize(const size_t size) { // Update configured worker pool thread count before server starts
        if (_running) {
            throw std::runtime_error("Cannot change thread pool size while server is running");
        }
        if (size == 0) {
            const unsigned int hw = std::thread::hardware_concurrency();
            _threadPoolSize = hw > 0 ? std::max(4u, hw) : 8;
        } else {
            _threadPoolSize = size;
        }
    }

    void Server::start() { // Start the server, creating the socket, binding it, and listening for incoming connections
        if (!this->_createSocket()) { // Attempt to create the socket, and if it fails, print an error message and return
            _logError("Error creating socket"); // Print an error message to standard error indicating that the socket creation failed
            return;
        }
        if (!this->_bindSocket()) {
            _logError("Error binding socket"); // Print an error message to standard error indicating that the socket binding failed
            return;
        }
        if (!this->_listenPort()) {
            _logError("Error listening on socket"); // Print an error message to standard error indicating that the socket listening failed
            return;
        }
        _running = true;
        _startThreadPool();
        _startWsPollLoop();
        _log("Server started on port " + std::to_string(this->getPort()) + " with " + std::to_string(_threadPoolSize) + " worker threads");
        _acceptLoop(); // Enter the accept loop to continuously accept incoming client connections and hand them off to workers
        stop();
    }

    std::thread Server::startAsync() { // Start the server in a separate background thread so multiple servers can run concurrently
        return std::thread([this] { // Spawn and return a std::thread executing this server's blocking start() method
            this->start(); // Invoke start() on the current Server instance inside the background thread
        });
    }

    Server& Server::use(Middleware middleware) { // Register a middleware function in the execution pipeline
        _middlewares.push_back(std::move(middleware));
        return *this;
    }

    void Server::get(const std::string& path, RequestHandler handler)  { _addRoute("GET",  path, std::move(handler)); } // Register a GET route with the specified path and handler function, moving the handler to avoid unnecessary copies
    void Server::post(const std::string& path, RequestHandler handler) { _addRoute("POST", path, std::move(handler)); } // Register a POST route with the specified path and handler function, moving the handler to avoid unnecessary copies
    void Server::put(const std::string& path, RequestHandler handler)  { _addRoute("PUT",  path, std::move(handler)); } // Register a PUT route with the specified path and handler function, moving the handler to avoid unnecessary copies
    void Server::del(const std::string& path, RequestHandler handler)  { _addRoute("DELETE", path, std::move(handler)); } // Register a DELETE route with the specified path and handler function, moving the handler to avoid unnecessary copies
    void Server::route(const std::string& method, const std::string& path, RequestHandler handler) { _addRoute(method, path, std::move(handler)); } // Register an arbitrary HTTP method route with the specified path and handler function

    void Server::setFallbackGetHandler(RequestHandler handler) { // Register a fallback handler for unmatched HTTP GET requests
        _fallbackGetHandler = std::move(handler);
        _log("Registered fallback GET handler");
    }

    int Server::getPort() const { // Return the port number that the server is listening on
        return _port;
    }

    const std::string& Server::getName() const { // Return the configured name of the server
        return _name;
    }

    void Server::sendJson(ServerResponse& res, const int status, const Json& json, const int indent) { // Send a JSON response to the client, setting the status code, content type, and body of the response
        res.status = status; // Set the HTTP status code of the response to the provided status value
        res.contentType = "application/json"; // Set the Content-Type header of the response to "application/json" to indicate that the response body contains JSON data
        res.body = json.dump(indent); // Serialize the provided Json object to a string using the dump method, with the specified indentation level, and set it as the body of the response
    }

    void Server::sendHtml(ServerResponse &res, const int status, const std::string &html) { // Send a HTML response to the client, setting the status code, content type, and body of the response
        res.status = status; // Set the HTTP status code of the response to the provided status value
        res.contentType = "text/html"; // Set the Content-Type header of the response to "text/html" to indicate that the response body contains HTML data
        res.charset = "utf-8"; // Set the charset parameter of the Content-Type header to "utf-8" to indicate that the response body is encoded in UTF-8
        res.body = std::move(html); // Move the provided HTML string into the body of the response
    }

    bool Server::prepareWebSocketHandshake(const ServerRequest& req, ServerResponse& res) { // Validate RFC 6455 handshake headers and build 101 Switching Protocols response
        if (!req.isWebSocketUpgrade()) {
            return false;
        }

        const std::string version = req.getHeader("sec-websocket-version");
        if (version != "13") {
            res.status = 426; // Upgrade Required per RFC 6455 section 4.2.2 and 4.4
            res.setHeader("Sec-WebSocket-Version", "13");
            res.contentType = "text/plain";
            res.body = "Sec-WebSocket-Version 13 required";
            return false;
        }

        const std::string key = req.getHeader("sec-websocket-key");
        if (key.empty()) {
            res.status = HTTP_BAD_REQUEST;
            res.contentType = "text/plain";
            res.body = "Missing Sec-WebSocket-Key";
            return false;
        }

        res.status = 101; // Switching Protocols
        res.setHeader("Upgrade", "websocket");
        res.setHeader("Connection", "Upgrade");
        res.setHeader("Sec-WebSocket-Accept", keet::Crypto::computeWebSocketAccept(key));
        return true;
    }

    void Server::websocket(const std::string& path, WebSocketHandlers handlers) { // Register a WebSocket endpoint with event callbacks
        std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
        _wsRoutes[path] = std::move(handlers);
        _log("Registered WebSocket route: " + path);
    }

    void Server::websocket(const std::string& path, WebSocketMessageHandler onMessage) { // Register a simple text-message WebSocket endpoint
        WebSocketHandlers handlers;
        handlers.onMessage = std::move(onMessage);
        websocket(path, std::move(handlers));
    }

    bool Server::sendWebSocketText(const int fd, const std::string_view text) { // Send a thread-safe text WebSocket frame to a client
        std::shared_ptr<ActiveWsConnection> conn;
        {
            std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
            const auto it = _wsConnections.find(fd);
            if (it == _wsConnections.end()) {
                return false;
            }
            conn = it->second;
        }

        const auto frame = keet::WebSocket::makeTextFrame(text);
        std::lock_guard<std::mutex> lock(conn->sendMutex);
        size_t totalSent = 0;
        while (totalSent < frame.size()) {
            const ssize_t n = send(fd, frame.data() + totalSent, frame.size() - totalSent, 0);
            if (n > 0) {
                totalSent += static_cast<size_t>(n);
            } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                pollfd pfd{ .fd = fd, .events = POLLOUT, .revents = 0 };
                const int pr = poll(&pfd, 1, 2000);
                if (pr <= 0) return false;
            } else {
                return false;
            }
        }
        return true;
    }

    bool Server::sendWebSocketBinary(const int fd, const uint8_t* data, const size_t length) { // Send a thread-safe binary WebSocket frame to a client
        std::shared_ptr<ActiveWsConnection> conn;
        {
            std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
            const auto it = _wsConnections.find(fd);
            if (it == _wsConnections.end()) {
                return false;
            }
            conn = it->second;
        }

        const auto frame = keet::WebSocket::makeBinaryFrame(data, length);
        std::lock_guard<std::mutex> lock(conn->sendMutex);
        size_t totalSent = 0;
        while (totalSent < frame.size()) {
            const ssize_t n = send(fd, frame.data() + totalSent, frame.size() - totalSent, 0);
            if (n > 0) {
                totalSent += static_cast<size_t>(n);
            } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                pollfd pfd{ .fd = fd, .events = POLLOUT, .revents = 0 };
                const int pr = poll(&pfd, 1, 2000);
                if (pr <= 0) return false;
            } else {
                return false;
            }
        }
        return true;
    }

    void Server::closeWebSocket(const int fd, const uint16_t statusCode, const std::string_view reason) { // Initiate graceful closure of a WebSocket connection
        _unregisterWebSocketConnection(fd, statusCode, std::string(reason), true);
    }

    size_t Server::activeWebSocketCount() { // Return the number of currently active WebSocket connections
        std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
        return _wsConnections.size();
    }

    //endregion public methods

    //region private methods
    std::string Server::_logPrefix() const { // Build the log message prefix using the server's name if one was provided
        return _name.empty() ? "" : "[" + _name + "] "; // Return "[name] " when a name is set, or an empty string otherwise
    }

    void Server::_log(const std::string& message) const { // Write a prefixed informational message to standard output
        std::cout << (_logPrefix() + message) << std::endl; // Output the combined prefix and message atomically before flushing
    }

    void Server::_logError(const std::string& message) const { // Write a prefixed error message to standard error
        std::cerr << (_logPrefix() + message) << std::endl; // Output the combined prefix and error message atomically before flushing
    }

    std::string Server::_toLower(std::string s) { // Convert a string to lowercase by iterating over each character and applying the tolower function, returning the modified string
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    std::string Server::_trim(const std::string& s) { // Trim whitespace from the beginning and end of a string
        const size_t b = s.find_first_not_of(" \t"); // Find the index of the first character that is not a space or tab
        if (b == std::string::npos) return ""; // If the string is entirely whitespace, return an empty string
        return s.substr(b, s.find_last_not_of(" \t") - b + 1); // Return the substring from the first non-whitespace character to the last non-whitespace character
    }

    void Server::_addRoute(const std::string& method, const std::string& path, RequestHandler handler) { // Add a route to the server's routing table, associating an HTTP method and path with a handler function
        _routes[method][path] = std::move(handler); // Store the handler function in the nested map, moving it to avoid unnecessary copies
        _log("Registered " + method + " route: " + path); // Print a message to standard output indicating that the route has been registered, including the HTTP method and path
    }

    bool Server::_createSocket() { // Create a TCP socket for the server to listen for incoming connections
        _log("Creating socket..."); // Print a message to standard output indicating that the socket creation process has started
        this->_server_fd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket using the IPv4 address family (AF_INET), stream socket type (SOCK_STREAM) for TCP, and default protocol (0)
        if (this->_server_fd == -1) { // Check if the socket creation failed (socket returns -1 on error)
            _logError("Failed to create socket"); // Print an error message to standard error indicating that the socket creation failed
            return false;
        }

        constexpr int opt = 1; // Set the socket option to allow reuse of the address, which helps avoid "address already in use" errors when restarting the server
        setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        _log("Socket created!"); // Print a message to standard output indicating that the socket has been successfully created
        return true;
    }

    bool Server::_bindSocket() { // Bind the created socket to the specified address and port, allowing the server to listen for incoming connections on that port
        _log("Binding socket..."); // Print a message to standard output indicating that the socket binding process has started
        this->_address.sin_family = AF_INET; // Set the address family to IPv4 (AF_INET) for the socket
        this->_address.sin_addr.s_addr = INADDR_ANY; // Set the IP address to INADDR_ANY, allowing the server to accept connections on any available network interface
        this->_address.sin_port = htons(this->_port); // Set the port number for the socket, converting it to network byte order using htons (host-to-network short)

        if (bind(this->_server_fd, reinterpret_cast<sockaddr*>(&this->_address), sizeof(this->_address)) == -1) { // Attempt to bind the socket to the specified address and port, checking for errors (bind returns -1 on failure)
            _logError("Failed to bind socket: " + std::string(strerror(errno))); // Print an error message with errno
            return false;
        }

        _log("Socket bound!"); // Print a message to standard output indicating that the socket has been successfully bound
        return true;
    }

    bool Server::_listenPort() const { // Start listening for incoming connections on the bound socket, allowing the server to accept client requests
        _log("Listening on socket..."); // Print a message to standard output indicating that the server is starting to listen for incoming connections
        if (listen(this->_server_fd, 128) == -1) { // Call the listen function on the socket with connection backlog of 128
            _logError("Failed to listen on socket"); // Print an error message to standard error indicating that the server failed to start listening on the socket
            return false;
        }

        _log("Socket is now listening!"); // Print a message to standard output indicating that the socket is now listening for incoming connections
        return true;
    }

    std::string Server::getStatusMessage(const int status) { // Return a human-readable message corresponding to an HTTP status code, allowing the server to provide meaningful responses to clients
        switch (status) {
            case 101: return "Switching Protocols";
            case HTTP_OK: return "OK";
            case 201: return "Created";
            case 204: return "No Content";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 304: return "Not Modified";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 403: return "Forbidden";
            case HTTP_NOT_FOUND: return "Not Found";
            case 405: return "Method Not Allowed";
            case 413: return "Payload Too Large";
            case 426: return "Upgrade Required";
            case 431: return "Request Header Fields Too Large";
            case HTTP_INTERNAL_SERVER_ERROR: return "Internal Server Error";
            case 501: return "Not Implemented";
            default:
                return "Unknown Status";
        }
    }

    std::string Server::_getStatusMessage(const int status) { // Look up human-readable reason phrase for a given HTTP status code
        return getStatusMessage(status);
    }

    std::string Server::_buildResponse(const ServerResponse& response) { // Build a complete HTTP response string from the provided ServerResponse object, including the status line, headers, and body, allowing the server to send a properly formatted response to the client
        if (response.status == 101) {
            std::ostringstream out;
            out << "HTTP/1.1 101 Switching Protocols\r\n";
            for (const auto& [name, value] : response.headers) {
                out << name << ": " << value << "\r\n";
            }
            out << "\r\n";
            return out.str();
        }

        std::ostringstream out;
        out << "HTTP/1.1 " << response.status << " " << getStatusMessage(response.status) << "\r\n"
            << "Content-Type: " << response.contentType
            << (response.charset.empty() ? "" : "; charset=" + response.charset) << "\r\n"
            << "Content-Length: " << response.body.size() << "\r\n";

        for (const auto& [name, value] : response.headers) {
            out << name << ": " << value << "\r\n";
        }

        if (!response.hasHeader("Access-Control-Allow-Origin")) {
            out << "Access-Control-Allow-Origin: *\r\n";
        }
        if (!response.hasHeader("Access-Control-Allow-Methods")) {
            out << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        }
        if (!response.hasHeader("Access-Control-Allow-Headers")) {
            out << "Access-Control-Allow-Headers: Content-Type, X-Session-Id\r\n";
        }
        if (!response.hasHeader("Access-Control-Expose-Headers")) {
            out << "Access-Control-Expose-Headers: X-Session-Id, X-Route-Path\r\n";
        }
        out << "Connection: close\r\n\r\n"
            << response.body;
        return out.str();
    }

    bool Server::_parseRequest(const std::string& raw, ServerRequest& request) { // Parse a raw HTTP request string into a ServerRequest object, extracting the method, path, query parameters, and headers, allowing the server to process incoming requests
        size_t lineEnd = raw.find("\r\n"); // Find the end of the request line (the first line of the HTTP request) by searching for the CRLF ("\r\n") sequence
        std::istringstream line(raw.substr(0, lineEnd)); // Create an input string stream from the request line to facilitate parsing of the method, target (path and query), and HTTP version
        std::string target, version; // Declare variables to hold the target (path and query) and HTTP version extracted from the request line

        if (!(line >> request.method >> target >> version)) { // Attempt to read the method, target, and version from the request line; if any of these fail to parse, return false to indicate a malformed request
            std::cerr << "Failed to parse request line" << std::endl; // Print an error message to standard error indicating that the request line could not be parsed correctly
            return false;
        }

        size_t q = target.find('?'); // Find the position of the '?' character in the target string, which separates the path from the query string; if not found, q will be std::string::npos
        request.path = target.substr(0, q); // Extract the path from the target string by taking the substring from the beginning of the target to the position of the '?' character (or the end of the string if '?' is not found)
        if (q != std::string::npos) { // If a query string is present (indicated by the presence of '?'), parse the query parameters into the request.query map
            _parseQuery(target.substr(q + 1), request.query); // Call the _parseQuery helper function to parse the query string (the substring after the '?') and populate the request.query map with key-value pairs representing the query parameters
        }

        if (lineEnd == std::string::npos) return true; // If there are no headers (indicated by the absence of a CRLF after the request line), return true to indicate that the request has been successfully parsed
        size_t pos = lineEnd + 2; // Set the position to the start of the headers section by moving past the CRLF that ends the request line
        while (pos < raw.size()) { // Loop through the headers section of the raw request string, parsing each header line until the end of the string is reached
            size_t end = raw.find("\r\n", pos); // Find the end of the current header line by searching for the next CRLF ("\r\n") sequence starting from the current position
            if (end == std::string::npos) end = raw.size(); // If no CRLF is found, set the end position to the end of the string, indicating that this is the last header line
            std::string headerLine = raw.substr(pos, end - pos); // Extract the current header line from the raw request string by taking the substring from the current position to the end position
            pos = end + 2; // Advance the search position past the CRLF sequence to the start of the next header line

            size_t colon = headerLine.find(':'); // Locate the colon separator that divides the header name from its value
            if (colon == std::string::npos) return false; // If no colon is found, the header line is malformed, so return false to indicate a parsing error
            request.headers[_toLower(headerLine.substr(0, colon))] = _trim(headerLine.substr(colon + 1)); // Normalize the header name to lowercase and trim any whitespace from the value before storing it in the headers map
        }
        return true; // Successfully parsed the entire request line and all header fields, return true
    }

    int Server::_hexValue(const char c) { // Convert a single hexadecimal character to its numerical integer value (0-15), or return -1 if invalid
        if (c >= '0' && c <= '9') return c - '0'; // If the character is a decimal digit ('0'-'9'), return its numeric value by subtracting '0'
        if (c >= 'a' && c <= 'f') return c - 'a' + 10; // If the character is a lowercase hex letter ('a'-'f'), return its decimal value (10-15)
        if (c >= 'A' && c <= 'F') return c - 'A' + 10; // If the character is an uppercase hex letter ('A'-'F'), return its decimal value (10-15)
        return -1; // If the character is not a valid hexadecimal digit, return -1 to indicate an error
    }

    std::string Server::_urlDecode(const std::string& s) { // Decode a percent-encoded URL string, converting '+' to space and '%XX' hexadecimal sequences to their character values
        std::string out; // Initialize a string to accumulate the decoded output characters
        out.reserve(s.size()); // Preallocate memory for the output string to avoid multiple reallocations during decoding
        for (size_t i = 0; i < s.size(); ++i) { // Loop through each character of the input string to inspect and decode it
            if (s[i] == '+') { // If the character is '+', treat it as an encoded space according to URL encoding rules
                out += ' '; // Replace '+' with a space character in the decoded output
            } else if (s[i] == '%' && i + 2 < s.size() && _hexValue(s[i + 1]) >= 0 && _hexValue(s[i + 2]) >= 0) { // Check if '%' is followed by two valid hexadecimal digits representing an encoded byte
                out += static_cast<char>(_hexValue(s[i + 1]) * 16 + _hexValue(s[i + 2])); // Convert the two hexadecimal digits into a single byte character and append it to the output
                i += 2; // Advance the index by 2 to skip past the two hex digits that were processed
            } else { // If the character is not part of an escape sequence, treat it as a literal character
                out += s[i]; // Append regular characters directly to the output string
            }
        }
        return out; // Return the fully decoded string
    }

    void Server::_parseQuery(const std::string& queryString, std::map<std::string, std::string>& out) { // Parse a query string containing key-value pairs separated by '&' and populate the provided map with URL-decoded values
        std::istringstream stream(queryString); // Create an input string stream from the query string to split pairs by the '&' delimiter
        std::string pair; // Declare a string variable to hold each individual key-value pair token
        while (std::getline(stream, pair, '&')) { // Read key-value pairs separated by '&' until the end of the query string is reached
            if (pair.empty()) continue; // Skip empty segments caused by leading, trailing, or duplicate '&' characters
            size_t eq = pair.find('='); // Find the position of the '=' character that separates the key from the value in the pair
            std::string key   = eq == std::string::npos ? pair : pair.substr(0, eq); // Extract key up to '=', or use the entire pair if '=' is absent
            std::string value = eq == std::string::npos ? ""   : pair.substr(eq + 1); // Extract value after '=', or use an empty string if '=' is absent
            out[_urlDecode(key)] = _urlDecode(value); // URL-decode both the key and value, and insert them into the output map
        }
    }

    void Server::_handleClient(const int client_fd) { // Process an accepted client connection by reading the request, routing it, sending the response, and closing the connection
        ServerRequest request; // Instantiate a ServerRequest object to hold the parsed incoming request data
        ServerResponse response; // Instantiate a ServerResponse object to hold the response data to be generated

        sockaddr_in peerAddr{};
        socklen_t peerLen = sizeof(peerAddr);
        if (getpeername(client_fd, reinterpret_cast<sockaddr*>(&peerAddr), &peerLen) == 0) {
            char ipBuf[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &peerAddr.sin_addr, ipBuf, sizeof(ipBuf))) {
                request.remoteAddr = ipBuf;
            }
            request.remotePort = ntohs(peerAddr.sin_port);
        }

        const int status = _readRequest(client_fd, request); // Read and parse the incoming HTTP request from the client socket, returning 0 on success or an HTTP status code on error
        if (status < 0) { close(client_fd); return; } // If reading failed or the client disconnected prematurely, close the socket and exit early

        if (status != 0) {
            _setError(response, status); // If request reading or validation returned an error status code, set the error response
            const std::string out = _buildResponse(response);
            write(client_fd, out.c_str(), out.size());
            close(client_fd);
            return;
        }

        if (request.isWebSocketUpgrade()) { // Handle RFC 6455 WebSocket upgrade handshake if requested
            WebSocketHandlers handlers;
            bool hasWsRoute = false;
            {
                std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
                const auto it = _wsRoutes.find(request.path);
                if (it != _wsRoutes.end()) {
                    handlers = it->second;
                    hasWsRoute = true;
                }
            }

            if (hasWsRoute) {
                if (!prepareWebSocketHandshake(request, response)) {
                    const std::string out = _buildResponse(response);
                    write(client_fd, out.c_str(), out.size());
                    close(client_fd);
                    return;
                }

                const std::string out = _buildResponse(response);
                write(client_fd, out.c_str(), out.size());
                _registerWebSocketConnection(client_fd, std::move(request), std::move(handlers));
                return; // Upgraded persistent WebSocket connection handoff complete
            }
        }

        _dispatch(request, response); // Otherwise, dispatch the valid request to the matching registered route handler

        for (const auto& cb : response.onComplete) { // Run completion hooks (e.g. access loggers)
            try {
                if (cb) cb(request, response);
            } catch (...) {}
        }

        const std::string out = _buildResponse(response); // Serialize the ServerResponse object into a formatted HTTP response string
        write(client_fd, out.c_str(), out.size()); // Write the complete HTTP response string to the client socket
        close(client_fd); // Close the client connection socket since HTTP Connection: close is used
    }

    void Server::_startThreadPool() { // Spawn configured worker threads to process client connections concurrently
        _workers.reserve(_threadPoolSize);
        for (size_t i = 0; i < _threadPoolSize; ++i) {
            _workers.emplace_back([this] {
                _workerLoop();
            });
        }
    }

    void Server::_stopThreadPool() { // Drain pending tasks, wake all workers, and join worker threads
        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            while (!_taskQueue.empty()) {
                _taskQueue.pop();
            }
        }
        _cv.notify_all();
        for (auto& worker : _workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        _workers.clear();
    }

    void Server::_workerLoop() { // Worker thread function pulling tasks from queue and processing them
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(_queueMutex);
                _cv.wait(lock, [this] {
                    return !_running || !_taskQueue.empty();
                });
                if (!_running && _taskQueue.empty()) {
                    break;
                }
                task = std::move(_taskQueue.front());
                _taskQueue.pop();
            }
            if (task) {
                task();
            }
        }
    }

    void Server::_acceptLoop() { // Main accept loop running continuously to accept client TCP connections and enqueue them for worker threads
        while (_running) { // Keep accepting connections as long as the server is running
            sockaddr_in client{}; // Allocate a sockaddr_in structure to store incoming client connection address info
            socklen_t len = sizeof(client); // Initialize address structure length for the accept system call
            const int client_fd = accept(this->_server_fd, reinterpret_cast<sockaddr*>(&client), &len); // Accept the next connection on the server socket
            if (client_fd == -1) { // Check if the accept call failed
                if (!_running) {
                    break; // Server stopped, exit accept loop cleanly
                }
                _logError("Failed to accept connection: " + std::string(strerror(errno)));
                continue;
            }
            _enqueueWork([this, client_fd] {
                _handleClient(client_fd);
            });
        }
    }

    void Server::_enqueueWork(std::function<void()> task) { // Enqueue a unit of work onto the worker thread pool
        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _taskQueue.push(std::move(task));
        }
        _cv.notify_one();
    }

    void Server::_registerWebSocketConnection(const int fd, ServerRequest req, WebSocketHandlers handlers) { // Track new persistent WebSocket connection
        const int flags = fcntl(fd, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }
#ifdef SO_NOSIGPIPE
        constexpr int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif

        auto conn = std::make_shared<ActiveWsConnection>();
        conn->fd = fd;
        conn->request = std::move(req);
        conn->handlers = std::move(handlers);

        {
            std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
            _wsConnections[fd] = conn;
        }

        if (_wsWakePipe[1] != -1) {
            const char wake = 'w';
            (void)write(_wsWakePipe[1], &wake, 1);
        }

        if (conn->handlers.onOpen) {
            _enqueueWork([handlers = conn->handlers, fd, req = conn->request] {
                handlers.onOpen(fd, req);
            });
        }
    }

    void Server::_unregisterWebSocketConnection(const int fd, const uint16_t code, const std::string& reason, const bool sendCloseFrame) { // Close and untrack WebSocket connection
        std::shared_ptr<ActiveWsConnection> conn;
        {
            std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
            const auto it = _wsConnections.find(fd);
            if (it == _wsConnections.end()) {
                return;
            }
            conn = it->second;
            _wsConnections.erase(it);
        }
        if (sendCloseFrame) {
            const auto closeFrame = keet::WebSocket::makeCloseFrame(code, reason);
            std::lock_guard<std::mutex> lock(conn->sendMutex);
            send(fd, closeFrame.data(), closeFrame.size(), 0);
        }
        close(fd);
        if (_wsWakePipe[1] != -1) {
            const char wake = 'w';
            (void)write(_wsWakePipe[1], &wake, 1);
        }
        if (conn->handlers.onClose) {
            _enqueueWork([handlers = conn->handlers, fd, code, reason] {
                handlers.onClose(fd, code, reason);
            });
        }
    }

    void Server::_startWsPollLoop() { // Launch background WebSocket event poll loop
        if (_wsWakePipe[0] != -1 || _wsWakePipe[1] != -1) {
            return;
        }
        if (pipe(_wsWakePipe) == -1) {
            _logError("Failed to create WebSocket wake pipe");
            return;
        }
        const int flags0 = fcntl(_wsWakePipe[0], F_GETFL, 0);
        if (flags0 >= 0) fcntl(_wsWakePipe[0], F_SETFL, flags0 | O_NONBLOCK);
        const int flags1 = fcntl(_wsWakePipe[1], F_GETFL, 0);
        if (flags1 >= 0) fcntl(_wsWakePipe[1], F_SETFL, flags1 | O_NONBLOCK);

        _wsPollThread = std::thread([this] {
            _wsPollLoop();
        });
    }

    void Server::_stopWsPollLoop() { // Terminate background WebSocket event poll loop
        if (_wsWakePipe[1] != -1) {
            const char wake = 'q';
            (void)write(_wsWakePipe[1], &wake, 1);
        }
        if (_wsPollThread.joinable()) {
            _wsPollThread.join();
        }
        std::vector<std::shared_ptr<ActiveWsConnection>> toClose;
        {
            std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
            for (const auto& [fd, conn] : _wsConnections) {
                toClose.push_back(conn);
            }
            _wsConnections.clear();
        }
        for (const auto& conn : toClose) {
            const auto closeFrame = keet::WebSocket::makeCloseFrame(1001, "Server shutting down");
            {
                std::lock_guard<std::mutex> lock(conn->sendMutex);
                send(conn->fd, closeFrame.data(), closeFrame.size(), 0);
            }
            close(conn->fd);
            if (conn->handlers.onClose) {
                conn->handlers.onClose(conn->fd, 1001, "Server shutting down");
            }
        }
        if (_wsWakePipe[0] != -1) {
            close(_wsWakePipe[0]);
            _wsWakePipe[0] = -1;
        }
        if (_wsWakePipe[1] != -1) {
            close(_wsWakePipe[1]);
            _wsWakePipe[1] = -1;
        }
    }

    void Server::_wsPollLoop() { // Dedicated event loop monitoring active WebSocket file descriptors with poll()
        while (_running) {
            std::vector<pollfd> fds;
            fds.reserve(64);
            fds.push_back(pollfd{ .fd = _wsWakePipe[0], .events = POLLIN, .revents = 0 });

            {
                std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
                for (const auto& [fd, _] : _wsConnections) {
                    fds.push_back(pollfd{ .fd = fd, .events = POLLIN, .revents = 0 });
                }
            }

            const int ret = poll(fds.data(), static_cast<nfds_t>(fds.size()), 1000);
            if (ret < 0) {
                if (errno == EINTR) continue;
                if (!_running) break;
                _logError("WebSocket poll error: " + std::string(strerror(errno)));
                continue;
            }

            if (!_running) break;

            if (fds[0].revents & POLLIN) {
                char drainBuf[64];
                while (read(_wsWakePipe[0], drainBuf, sizeof(drainBuf)) > 0) {}
            }

            for (size_t i = 1; i < fds.size(); ++i) {
                const int fd = fds[i].fd;
                const short revents = fds[i].revents;
                if (revents == 0) continue;

                if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    _unregisterWebSocketConnection(fd, 1006, "Connection dropped", false);
                    continue;
                }

                if (revents & POLLIN) {
                    std::shared_ptr<ActiveWsConnection> conn;
                    {
                        std::lock_guard<std::mutex> lock(_wsConnectionsMutex);
                        const auto it = _wsConnections.find(fd);
                        if (it == _wsConnections.end()) continue;
                        conn = it->second;
                    }

                    uint8_t buffer[4096];
                    bool dropConnection = false;
                    while (true) {
                        const ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
                        if (n > 0) {
                            conn->rxBuffer.insert(conn->rxBuffer.end(), buffer, buffer + n);
                        } else if (n == 0) {
                            dropConnection = true;
                            break;
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;
                            }
                            dropConnection = true;
                            break;
                        }
                    }

                    if (dropConnection) {
                        _unregisterWebSocketConnection(fd, 1006, "Connection closed by client", false);
                        continue;
                    }

                    size_t offset = 0;
                    while (offset < conn->rxBuffer.size()) {
                        auto parseResult = keet::WebSocket::parseFrame(conn->rxBuffer.data() + offset, conn->rxBuffer.size() - offset, true);
                        if (parseResult.status == keet::WebSocket::ParseStatus::NeedMoreData) {
                            break;
                        }
                        if (parseResult.status == keet::WebSocket::ParseStatus::ProtocolError) {
                            if (conn->handlers.onError) {
                                _enqueueWork([handlers = conn->handlers, fd, desc = parseResult.errorMessage] {
                                    handlers.onError(fd, desc);
                                });
                            }
                            _unregisterWebSocketConnection(fd, static_cast<uint16_t>(keet::WebSocket::CloseCode::ProtocolError), parseResult.errorMessage, true);
                            break;
                        }

                        offset += parseResult.bytesConsumed;
                        keet::WebSocket::Frame frame = std::move(*parseResult.frame);

                        if (frame.opcode == keet::WebSocket::Opcode::Close) {
                            const uint16_t code = frame.closeStatusCode();
                            const std::string reason = frame.closeReason();
                            _unregisterWebSocketConnection(fd, code, reason, true);
                            break;
                        }
                        if (frame.opcode == keet::WebSocket::Opcode::Ping) {
                            const auto pong = keet::WebSocket::handlePing(frame);
                            std::lock_guard<std::mutex> sendLock(conn->sendMutex);
                            send(fd, pong.data(), pong.size(), 0);
                        } else if (frame.opcode == keet::WebSocket::Opcode::Pong) {
                            // Pong heartbeat response received
                        } else {
                            auto defragRes = conn->defragmenter.processFrame(std::move(frame));
                            if (defragRes == keet::WebSocket::MessageDefragmenter::Result::Error) {
                                std::string err = conn->defragmenter.errorMessage();
                                if (conn->handlers.onError) {
                                    _enqueueWork([handlers = conn->handlers, fd, desc = err] {
                                        handlers.onError(fd, desc);
                                    });
                                }
                                _unregisterWebSocketConnection(fd, static_cast<uint16_t>(keet::WebSocket::CloseCode::ProtocolError), err, true);
                                break;
                            }
                            if (defragRes == keet::WebSocket::MessageDefragmenter::Result::MessageComplete) {
                                if (conn->defragmenter.messageOpcode() == keet::WebSocket::Opcode::Text) {
                                    std::string msg = conn->defragmenter.messageText();
                                    conn->defragmenter.reset();
                                    if (conn->handlers.onMessage) {
                                        _enqueueWork([handlers = conn->handlers, fd, msg = std::move(msg)] {
                                            handlers.onMessage(fd, msg);
                                        });
                                    }
                                } else {
                                    conn->defragmenter.reset();
                                }
                            }
                        }
                    }

                    if (offset > 0) {
                        conn->rxBuffer.erase(conn->rxBuffer.begin(), conn->rxBuffer.begin() + offset);
                    }
                }
            }
        }
    }

    int Server::_readRequest(const int client_fd, ServerRequest &request) { // Read raw bytes from the client socket, extract headers and body according to Content-Length, and populate the ServerRequest object
        std::string raw; // Buffer to accumulate raw data read from the socket until all headers are received
        char buffer[4096]; // Chunk buffer of 4 KB used to read data from the socket in increments

        size_t headEnd; // Variable to store position of double CRLF ("\r\n\r\n") separating headers from the request body
        while ((headEnd = raw.find("\r\n\r\n")) == std::string::npos) { // Loop until the double CRLF indicating the end of the HTTP headers is found
            if (raw.size() > MAX_HEADER_SIZE) return 431; // If accumulated raw data exceeds the maximum allowed header size, return HTTP 431 (Request Header Fields Too Large)
            const ssize_t n = read(client_fd, buffer, sizeof(buffer)); // Read incoming bytes from the client socket into the temporary chunk buffer
            if (n <= 0) return -1; // If read returns 0 (client closed connection) or negative (read error), return -1 to signal failure
            raw.append(buffer, static_cast<size_t>(n)); // Append newly read bytes to the raw request buffer string
        }

        if (!_parseRequest(raw.substr(0, headEnd), request)) return HTTP_BAD_REQUEST; // Parse request line and headers; if parsing fails, return HTTP 400 (Bad Request)
        std::string body = raw.substr(headEnd + 4); // Extract initial body bytes that may have been read past the double CRLF delimiter

        if (request.headers.contains("transfer-encoding")) return 501; // Return HTTP 501 (Not Implemented) if Transfer-Encoding header is present (chunked encoding not supported)

        size_t contentLength = 0; // Initialize expected body length to 0
        const auto it = request.headers.find("content-length"); // Look up Content-Length header in the parsed headers map
        if (it != request.headers.end()) { // If Content-Length header is present, validate and extract its numeric value
            const std::string& value = it->second; // Retrieve string value of the Content-Length header
            if (value.empty() || !std::isdigit(static_cast<unsigned char>(value[0]))) return HTTP_BAD_REQUEST; // If value is empty or does not start with a digit, return HTTP 400 (Bad Request)
            try { // Attempt to parse the numeric string into an integer value safely
                size_t used = 0; // Variable to track number of characters consumed by stoull
                contentLength = std::stoull(value, &used); // Convert Content-Length string to unsigned long long
                if (used != value.size()) return HTTP_BAD_REQUEST; // If there are trailing non-digit characters in the value, return HTTP 400 (Bad Request)
            } catch (const std::exception&) { // Catch any exception thrown by stoull (e.g. std::out_of_range or std::invalid_argument)
                return HTTP_BAD_REQUEST; // Return HTTP 400 (Bad Request) if conversion fails
            }
            if (contentLength > MAX_BODY_SIZE) return 413; // If content length exceeds maximum allowed body size (1 MB), return HTTP 413 (Payload Too Large)
        }

        const auto expect = request.headers.find("expect"); // Look up Expect header in the parsed headers map
        if (body.size() < contentLength && expect != request.headers.end() && // Check if body is incomplete and Expect header was supplied
            _toLower(expect->second) == "100-continue") { // Check if Expect header value is "100-continue"
            const std::string cont = "HTTP/1.1 100 Continue\r\n\r\n"; // Prepare the HTTP 100 Continue interim response string
            write(client_fd, cont.c_str(), cont.size()); // Send 100 Continue response to client to prompt sending of remaining body
        }

        while (body.size() < contentLength) { // Loop until the entire body of length contentLength has been read from the socket
            const ssize_t n = read(client_fd, buffer, sizeof(buffer)); // Read subsequent bytes of the body from the client socket into the buffer
            if (n <= 0) return -1; // If read fails or client disconnects prematurely, return -1 to signal failure
            body.append(buffer, static_cast<size_t>(n)); // Append incoming bytes to the body string
        }
        body.resize(contentLength); // Trim any potential excess bytes beyond contentLength so the body length matches exactly

        request.body = std::move(body); // Move the assembled body string into the request object to avoid unnecessary copying
        return 0; // Return 0 to indicate request was successfully read and prepared
    }

    void Server::_setError(ServerResponse& res, const int status) { // Helper function to set an error response with the given HTTP status code and a JSON body containing the error message
        sendJson(res, status, Json::Object{{"error", _getStatusMessage(status)}}); // Serialize a JSON object with an "error" key set to the status message and populate the response
    }

    void Server::_dispatch(ServerRequest& request, ServerResponse& response) { // Route the incoming request through middleware pipeline and to the appropriate registered handler
        for (const auto& middleware : _middlewares) {
            if (!middleware) continue;
            try {
                if (!middleware(request, response)) {
                    return; // Short-circuited by middleware
                }
            } catch (const std::exception& e) {
                _logError(std::string("Middleware threw: ") + e.what());
                _setError(response, HTTP_INTERNAL_SERVER_ERROR);
                return;
            } catch (...) {
                _logError("Unknown exception in middleware pipeline");
                _setError(response, HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
        }

        if (request.method == "OPTIONS") { // Handle CORS preflight OPTIONS requests automatically
            response.status = HTTP_OK; // Respond with 200 OK for preflight checks
            response.body = ""; // Empty body for OPTIONS response
            return; // Return early with CORS headers attached by _buildResponse
        }

        const auto methodIt = _routes.find(request.method); // Look up routing map corresponding to the request's HTTP method (e.g. "GET" or "POST")
        if (methodIt != _routes.end()) { // If routes have been registered for this HTTP method
            const auto it = methodIt->second.find(request.path); // Look up handler registered for the specific request path within this method's routes
            if (it != methodIt->second.end()) { // If a matching route handler is found for the path
                try { // Execute the route handler within a try-catch block to gracefully handle any exceptions thrown by handler code
                    it->second(request, response); // Invoke the registered route handler function, passing the request and response references
                } catch (const std::exception& e) { // Catch any standard exception thrown by the route handler
                    _logError(std::string("Handler threw: ") + e.what()); // Print an error message to standard error indicating that the handler threw an exception
                    _setError(response, HTTP_INTERNAL_SERVER_ERROR); // Set the response to HTTP 500 (Internal Server Error) to notify the client of the failure
                }
                return; // Return after the route handler has finished executing
            }
        }

        // Path exists, but not for this method -> 405 instead of 404
        for (const auto &val: _routes | std::views::values) { // Iterate over the route maps of all registered HTTP methods using a range view
            if (val.contains(request.path)) { // Check if the requested path is registered under a different HTTP method
                _setError(response, 405); // Set the response to HTTP 405 (Method Not Allowed) if path exists for other methods
                return; // Return immediately after setting the 405 response
            }
        }

        if (request.method == "GET" && _fallbackGetHandler) { // Invoke fallback GET handler when an unmatched GET path is requested
            try {
                _fallbackGetHandler(request, response);
            } catch (const std::exception& e) {
                _logError(std::string("Fallback GET handler threw: ") + e.what());
                _setError(response, HTTP_INTERNAL_SERVER_ERROR);
            }
            return;
        }

        _setError(response, HTTP_NOT_FOUND); // If the path does not exist under any registered HTTP method, set the response to HTTP 404 (Not Found)
    }
    //endregion private methods
} // HTTPServer
