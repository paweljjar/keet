//
// Created by PAWEL on 19/10/2026.
//

#ifndef LORIKEET_MIDDLEWARE_H
#define LORIKEET_MIDDLEWARE_H

#include "Server.h"
#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace keet::Middleware {

    // Options controlling the HTTP request logging middleware
    struct LoggerOptions {
        std::function<void(const std::string&)> logFunction; // Custom log sink (defaults to std::cout)
        bool includeTimestamp = true;                        // Prepend formatted timestamp [YYYY-MM-DD HH:MM:SS]
        bool includeDuration = true;                         // Record execution time in milliseconds
        bool includeStatus = true;                           // Include HTTP status code and message
        bool includeRemoteAddr = true;                       // Include client IP address if available
    };

    // Factory creating request logging middleware
    [[nodiscard]] HTTPServer::Middleware logger(LoggerOptions options = {});

    // Options controlling Cross-Origin Resource Sharing (CORS) middleware
    struct CorsOptions {
        std::string origin = "*";                                  // Allowed origin ("*" or specific domain)
        std::vector<std::string> allowedOrigins;                   // Optional whitelist of origins to match against request Origin
        std::vector<std::string> allowedMethods = {"GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH"}; // Allowed HTTP methods
        std::vector<std::string> allowedHeaders = {"Content-Type", "Authorization", "X-Requested-With", "X-Session-Id"}; // Allowed request headers
        std::vector<std::string> exposedHeaders = {"Content-Length", "X-Session-Id", "X-Route-Path"}; // Response headers exposed to browser
        bool allowCredentials = false;                             // Whether to send Access-Control-Allow-Credentials: true
        int maxAge = 86400;                                        // Preflight cache TTL in seconds (Access-Control-Max-Age)
    };

    // Factory creating CORS middleware
    [[nodiscard]] HTTPServer::Middleware cors(CorsOptions options = {});

    // Options controlling defensive HTTP security headers middleware
    struct SecurityOptions {
        std::string xContentTypeOptions = "nosniff";                     // X-Content-Type-Options: nosniff
        std::string xFrameOptions = "SAMEORIGIN";                        // X-Frame-Options: SAMEORIGIN or DENY
        std::string xXssProtection = "1; mode=block";                    // X-XSS-Protection: 1; mode=block
        std::string referrerPolicy = "strict-origin-when-cross-origin";  // Referrer-Policy
        std::string contentSecurityPolicy = "";                          // Content-Security-Policy (e.g., default-src 'self')
        std::string strictTransportSecurity = "";                        // Strict-Transport-Security (e.g., max-age=31536000; includeSubDomains)
        std::string permissionsPolicy = "";                              // Permissions-Policy
    };

    // Factory creating security headers middleware
    [[nodiscard]] HTTPServer::Middleware securityHeaders(SecurityOptions options = {});

} // namespace keet::Middleware

namespace HTTPServer::Middlewares {
    using namespace keet::Middleware;
}

#endif // LORIKEET_MIDDLEWARE_H
