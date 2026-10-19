//
// Created by PAWEL on 19/10/2026.
//

#ifndef LORIKEET_STATICFILES_H
#define LORIKEET_STATICFILES_H

#include <chrono>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HTTPServer {
    struct ServerRequest;
    struct ServerResponse;
}

namespace keet::StaticFiles {

    // Options controlling static file serving behavior
    struct StaticOptions {
        std::string defaultFile = "index.html";           // Default file served when a directory is requested
        int maxAge = 86400;                               // Cache-Control max-age in seconds (default: 1 day)
        bool isImmutable = false;                         // Append 'immutable' directive to Cache-Control
        bool allowHiddenFiles = false;                    // Whether to serve dotfiles (e.g. .hidden)
        bool enableETag = true;                           // Generate and validate HTTP ETag headers
        bool enableLastModified = true;                   // Generate and validate Last-Modified headers
        std::map<std::string, std::string> customHeaders; // Additional headers attached to static responses
    };

    // Retrieve the standard MIME type for a given file path or extension (e.g. ".css" -> "text/css")
    [[nodiscard]] std::string getMimeType(std::string_view pathOrExt);

    // Determine if a MIME type represents text/readable content (useful for appending charset=utf-8)
    [[nodiscard]] bool isTextMimeType(std::string_view mimeType);

    // Format a system clock time_point as an RFC 7231 / RFC 9110 HTTP-date string (GMT)
    [[nodiscard]] std::string formatHttpDate(std::chrono::system_clock::time_point tp);

    // Format a time_t epoch as an RFC 7231 / RFC 9110 HTTP-date string (GMT)
    [[nodiscard]] std::string formatHttpDate(std::time_t t);

    // Parse an incoming RFC 7231 / RFC 850 HTTP-date string into a system clock time_point
    [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parseHttpDate(std::string_view dateStr);

    // Convert std::filesystem::file_time_type to std::chrono::system_clock::time_point
    [[nodiscard]] std::chrono::system_clock::time_point toSystemTime(std::filesystem::file_time_type fileTime);

    // Generate an ETag token based on file size and last modification time
    [[nodiscard]] std::string generateETag(size_t fileSize, std::chrono::system_clock::time_point mtime);

    // Generate an ETag token based on in-memory payload content
    [[nodiscard]] std::string generateContentETag(std::string_view content);

    // Generate an ETag token for a file on disk
    [[nodiscard]] std::string generateFileETag(const std::filesystem::path& filePath);

    // Format a standard HTTP Cache-Control header value
    [[nodiscard]] std::string formatCacheControl(int maxAgeSeconds, bool isPublic = true, bool isImmutable = false);

    // Evaluate incoming conditional HTTP headers (If-None-Match, If-Modified-Since).
    // Returns true if the client's cached copy is still valid (yielding HTTP 304 Not Modified).
    [[nodiscard]] bool isNotModified(const HTTPServer::ServerRequest& req,
                                     std::string_view etag,
                                     std::chrono::system_clock::time_point lastModified);

    // Helper applying 304 Not Modified status, ETag, and caching headers to a response
    void applyNotModified(HTTPServer::ServerResponse& res,
                          std::string_view etag = "",
                          std::chrono::system_clock::time_point lastModified = {},
                          std::string_view cacheControl = "");

} // namespace keet::StaticFiles

namespace HTTPServer::StaticFiles {
    using namespace keet::StaticFiles;
}

#endif // LORIKEET_STATICFILES_H
