//
// Created by PAWEL on 15/10/2026.
//

#ifndef LORIKEET_SERVER_H
#define LORIKEET_SERVER_H
#include <map>
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <netinet/in.h>

#include "Json.h"
#include "WebSocket.h"

namespace HTTPServer {
#define HTTP_INTERNAL_SERVER_ERROR 500 // HTTP status code 500: Server encountered an unexpected condition preventing it from fulfilling the request
#define HTTP_OK 200 // HTTP status code 200: Standard response for successful HTTP requests
#define HTTP_CREATED 201 // HTTP status code 201: Request was successful and resulted in the creation of a new resource
#define HTTP_BAD_REQUEST 400 // HTTP status code 400: Server cannot process the request due to malformed request syntax
#define HTTP_NOT_FOUND 404 // HTTP status code 404: Requested resource could not be found on the server

    struct ServerRequest; // Forward declaration for ServerRequest
    struct ServerResponse; // Forward declaration for ServerResponse

    using RequestHandler = std::function<void(const ServerRequest&, ServerResponse&)>; // Function type definition for route handlers processing request and generating response
    using WebSocketMessageHandler = std::function<void(int fd, const std::string& message)>; // Handler processing incoming text WebSocket messages
    using WebSocketOpenHandler = std::function<void(int fd, const ServerRequest& req)>; // Handler invoked when WebSocket handshake completes successfully
    using WebSocketCloseHandler = std::function<void(int fd, uint16_t code, const std::string& reason)>; // Handler invoked when WebSocket connection terminates
    using WebSocketErrorHandler = std::function<void(int fd, const std::string& error)>; // Handler invoked on WebSocket protocol or transport errors

    struct WebSocketHandlers { // Grouping of event callbacks managing a persistent WebSocket endpoint lifecycle
        WebSocketOpenHandler onOpen; // Callback executed upon connection establishment
        WebSocketMessageHandler onMessage; // Callback executed upon receiving a complete text message
        WebSocketCloseHandler onClose; // Callback executed upon connection closure
        WebSocketErrorHandler onError; // Callback executed upon connection or protocol error
    };

    struct ServerRequest { // Represents an incoming HTTP request containing parsed method, path, query parameters, headers, and body
        std::string method; // HTTP request method (e.g., "GET", "POST")
        std::string path; // Target URL path of the request without query string
        std::map<std::string, std::string> query; // Map of parsed URL query parameters (key-value pairs)
        std::map<std::string, std::string> headers; // Map of HTTP request headers with lowercase keys
        std::string body; // Raw payload body received with the HTTP request
        std::string remoteAddr; // Client IPv4 address or peer address
        int remotePort = 0; // Client remote TCP port

        [[nodiscard]] bool hasQuery(const std::string& key) const; // Check if a query parameter exists in the request
        [[nodiscard]] std::string getQuery(const std::string& key, const std::string& fallback = "") const; // Retrieve a query parameter value, returning fallback if not found
        [[nodiscard]] std::string getHeader(const std::string& key, const std::string& fallback = "") const; // Retrieve a request header value (case-insensitive), returning fallback if not found
        [[nodiscard]] std::string getCookie(const std::string& name, const std::string& fallback = "") const; // Retrieve a cookie value by name from the Cookie header, returning fallback if not found
        [[nodiscard]] bool isWebSocketUpgrade() const; // Check if the request is an RFC 6455 WebSocket upgrade handshake
        [[nodiscard]] Json json() const { // Parse incoming request body as JSON
            if (body.empty()) return Json::Object{};
            return Json::parse(body);
        }
    };

    struct ServerResponse { // Represents an outgoing HTTP response containing status code, response body, and content type
        int status = HTTP_OK; // HTTP status code to send back to the client, defaults to 200 OK
        std::string body; // Response payload content to transmit to the client
        std::string contentType = "text/plain"; // Content-Type header value specifying the MIME type, defaults to text/plain
        std::string charset;
        std::map<std::string, std::string> headers; // Custom outgoing HTTP response headers
        std::vector<std::function<void(const ServerRequest&, const ServerResponse&)>> onComplete; // Callbacks invoked after request processing/dispatch finishes

        void setHeader(std::string name, std::string value); // Set or overwrite an outgoing HTTP response header
        [[nodiscard]] std::string getHeader(const std::string& name, const std::string& fallback = "") const; // Retrieve an outgoing response header value
        [[nodiscard]] bool hasHeader(const std::string& name) const; // Check case-insensitively if an outgoing response header exists
    };

    std::string generateSessionId(); // Generate a pseudo-random unique session identifier string
    std::string extractOrCreateSessionId(const ServerRequest& req, const std::string& cookieName = "lorikeet_session"); // Extract session ID from request headers/cookies with fallback generation

    using RequestHandler = std::function<void(const ServerRequest&, ServerResponse&)>; // Function type definition for route handlers processing request and generating response
    using Middleware = std::function<bool(ServerRequest&, ServerResponse&)>; // Function type definition for middleware processing request/response before route dispatch

    class Server { // Main HTTP server class managing socket setup, routing table, connection acceptance, and request dispatching
        public:
            explicit Server(int port, std::string name = "", size_t threadPoolSize = 0); // Constructor initializing server port, optional log label, and thread pool size
            ~Server(); // Destructor ensuring worker threads and socket are cleanly terminated
            void start(); // Start the server by creating, binding, and listening on socket, then entering connection loop
            std::thread startAsync(); // Start the server asynchronously on a background thread and return the thread handle
            void stop(); // Stop the server accept loop and shutdown worker thread pool
            Server& use(Middleware middleware); // Register a middleware callback executed sequentially on all incoming requests
            void get(const std::string& path, RequestHandler handler); // Register a route handler callback for HTTP GET requests at the given path
            void post(const std::string& path, RequestHandler handler); // Register a route handler callback for HTTP POST requests at the given path
            void put(const std::string& path, RequestHandler handler); // Register a route handler callback for HTTP PUT requests at the given path
            void del(const std::string& path, RequestHandler handler); // Register a route handler callback for HTTP DELETE requests at the given path
            void route(const std::string& method, const std::string& path, RequestHandler handler); // Register a route handler callback for arbitrary HTTP method at the given path
            void setFallbackGetHandler(RequestHandler handler); // Register a fallback handler for unmatched HTTP GET requests (e.g. SPA client-side routes)
            void websocket(const std::string& path, WebSocketHandlers handlers); // Register a WebSocket endpoint with event callbacks
            void websocket(const std::string& path, WebSocketMessageHandler onMessage); // Register a simple text-message WebSocket endpoint
            bool sendWebSocketText(int fd, std::string_view text); // Send a thread-safe text WebSocket frame to a client
            bool sendWebSocketBinary(int fd, const uint8_t* data, size_t length); // Send a thread-safe binary WebSocket frame to a client
            void closeWebSocket(int fd, uint16_t statusCode = 1000, std::string_view reason = ""); // Initiate graceful closure of a WebSocket connection
            [[nodiscard]] size_t activeWebSocketCount(); // Return the number of currently active WebSocket connections
            int getPort() const; // Return the configured port number the server listens on
            const std::string& getName() const; // Return the configured name of the server
            static std::string getStatusMessage(int status); // Return human-readable message corresponding to an HTTP status code
            size_t getThreadPoolSize() const; // Return configured or effective thread pool worker count
            void setThreadPoolSize(size_t size); // Set the number of worker threads to use (must be called before start())

            static void sendJson(ServerResponse& res, int status, const Json& json, int indent = -1); // Helper to serialize a Json object, set Content-Type to application/json, and set status code
            static void sendHtml(ServerResponse& res, int status, const std::string &html); // Helper to set Content-Type to text/html and set status code
            static bool prepareWebSocketHandshake(const ServerRequest& req, ServerResponse& res); // Validate RFC 6455 handshake headers and build 101 Switching Protocols response

        private:
            struct ActiveWsConnection { // Internal representation of an upgraded persistent WebSocket connection
                int fd = -1; // Socket file descriptor
                ServerRequest request; // Initial HTTP upgrade request
                WebSocketHandlers handlers; // Registered event handlers for this endpoint
                std::vector<uint8_t> rxBuffer; // Incoming byte accumulation buffer for stream parsing
                keet::WebSocket::MessageDefragmenter defragmenter; // Message defragmenter assembling multi-frame messages
                std::mutex sendMutex; // Mutex protecting concurrent writes to this socket
            };

            int _port; // Port number on which the server listens for incoming connections
            std::string _name; // Optional identifier name used to prefix server log messages
            int _server_fd; // Socket file descriptor for the server listener
            sockaddr_in _address; // Socket address structure holding IPv4 address and port configuration
            std::vector<Middleware> _middlewares; // Ordered list of registered middleware callbacks executed sequentially prior to route matching
            std::map<std::string, std::map<std::string, RequestHandler>> _routes; // Routing table mapping HTTP method to path to corresponding handler function
            RequestHandler _fallbackGetHandler; // Optional fallback handler invoked when an HTTP GET request path is not explicitly registered

            std::map<std::string, WebSocketHandlers> _wsRoutes; // Registered WebSocket endpoint routes by path
            std::map<int, std::shared_ptr<ActiveWsConnection>> _wsConnections; // Active WebSocket client connections keyed by fd
            std::mutex _wsConnectionsMutex; // Mutex protecting WebSocket connections map and routes
            std::thread _wsPollThread; // Dedicated background thread running the WebSocket poll loop
            int _wsWakePipe[2] = {-1, -1}; // Unidirectional pipe used to awaken poll() on connection updates or shutdown

            size_t _threadPoolSize; // Configured worker thread pool size (0 means auto-detect based on hardware concurrency)
            std::vector<std::thread> _workers; // Worker threads handling client HTTP requests concurrently
            std::queue<std::function<void()>> _taskQueue; // Queue of pending worker tasks (HTTP requests and WS messages)
            std::mutex _queueMutex; // Mutex guarding access to the client socket queue
            std::condition_variable _cv; // Condition variable notifying workers when new client connections are enqueued
            std::atomic<bool> _running{false}; // Atomic flag tracking whether the server and its worker pool are active

            static constexpr int MAX_HEADER_SIZE = 16 * 1024; // Maximum allowed size for raw HTTP request headers (16 KB)
            static constexpr int MAX_BODY_SIZE = 1024 * 1024; // Maximum allowed size for HTTP request body payload (1 MB)

            std::string _logPrefix() const; // Build the log message prefix using the server's name if provided
            void _log(const std::string& message) const; // Write a prefixed informational message to standard output
            void _logError(const std::string& message) const; // Write a prefixed error message to standard error

            static std::string _toLower(std::string s); // Convert all characters in a string to lowercase
            static std::string _trim(const std::string& s); // Strip leading and trailing whitespace characters from a string
            void _addRoute(const std::string& method, const std::string& path, RequestHandler handler); // Internal helper to associate a method and path with a handler in routing table
            bool _createSocket(); // Create TCP stream socket file descriptor with address reuse option enabled
            bool _bindSocket(); // Bind socket descriptor to configured port and all network interfaces (INADDR_ANY)
            bool _listenPort() const; // Put socket in passive listening mode with a connection backlog queue
            static std::string _getStatusMessage(int status); // Look up human-readable reason phrase for a given HTTP status code
            static std::string _buildResponse(const ServerResponse& response); // Format a ServerResponse object into a valid HTTP/1.1 response string with headers
            static bool _parseRequest(const std::string& raw, ServerRequest& request); // Parse raw request line and headers into a ServerRequest structure
            static int _hexValue(char c); // Convert a single hex character to its 0-15 integer value, or -1 if invalid
            static std::string _urlDecode(const std::string& s); // Decode percent-encoded characters and convert '+' to spaces in a string
            static void _parseQuery(const std::string& queryString, std::map<std::string, std::string>& out); // Parse and URL-decode URL query string key-value pairs into a map
            void _handleClient(int client_fd); // Process an individual connected client socket through request read, dispatch, and response write
            void _acceptLoop(); // Infinite loop accepting new client connections and handing them off to _handleClient
            void _workerLoop(); // Worker loop dequeuing client sockets and executing _handleClient
            void _startThreadPool(); // Spawn worker threads in the pool
            void _stopThreadPool(); // Signal worker threads to stop and join them
            void _enqueueWork(std::function<void()> task); // Enqueue a unit of work onto the worker thread pool
            void _registerWebSocketConnection(int fd, ServerRequest req, WebSocketHandlers handlers); // Track new persistent WebSocket connection
            void _unregisterWebSocketConnection(int fd, uint16_t code, const std::string& reason, bool sendCloseFrame); // Close and untrack WebSocket connection
            void _startWsPollLoop(); // Launch background WebSocket event poll loop
            void _stopWsPollLoop(); // Terminate background WebSocket event poll loop
            void _wsPollLoop(); // Dedicated event loop monitoring active WebSocket file descriptors with poll()

            static int _readRequest(int client_fd, ServerRequest& request); // Read full HTTP request from socket, parsing headers and streaming body per Content-Length

            static void _setError(ServerResponse& res, int status); // Populate a ServerResponse with a standard JSON error message for given status code
            void _dispatch(ServerRequest& request, ServerResponse& response); // Look up matching route handler for request method and path, handling 404, 405, and exceptions
    };
}

#endif //LORIKEET_SERVER_H
