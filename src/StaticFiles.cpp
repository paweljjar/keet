//
// Created by PAWEL on 19/10/2026.
//

#include "keet/StaticFiles.h"
#include "keet/Server.h"
#include "keet/Crypto.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace keet::StaticFiles {

    namespace {
        std::string toLower(std::string_view s) {
            std::string res;
            res.reserve(s.size());
            for (char c : s) {
                res += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return res;
        }

        std::string extractExtension(std::string_view pathOrExt) {
            const size_t dot = pathOrExt.find_last_of('.');
            if (dot != std::string_view::npos) {
                return toLower(pathOrExt.substr(dot + 1));
            }
            return toLower(pathOrExt);
        }

        const std::unordered_map<std::string, std::string> MIME_MAP = {
            // Text / Documents / Web
            {"html",  "text/html"},
            {"htm",   "text/html"},
            {"css",   "text/css"},
            {"js",    "text/javascript"},
            {"mjs",   "text/javascript"},
            {"json",  "application/json"},
            {"map",   "application/json"},
            {"wasm",  "application/wasm"},
            {"xml",   "application/xml"},
            {"txt",   "text/plain"},
            {"md",    "text/markdown"},
            {"csv",   "text/csv"},

            // Images
            {"png",   "image/png"},
            {"jpg",   "image/jpeg"},
            {"jpeg",  "image/jpeg"},
            {"gif",   "image/gif"},
            {"svg",   "image/svg+xml"},
            {"ico",   "image/x-icon"},
            {"webp",  "image/webp"},
            {"avif",  "image/avif"},
            {"bmp",   "image/bmp"},
            {"tiff",  "image/tiff"},
            {"tif",   "image/tiff"},

            // Fonts
            {"woff2", "font/woff2"},
            {"woff",  "font/woff"},
            {"ttf",   "font/ttf"},
            {"otf",   "font/otf"},
            {"eot",   "application/vnd.ms-fontobject"},

            // Audio & Video
            {"mp4",   "video/mp4"},
            {"webm",  "video/webm"},
            {"mp3",   "audio/mpeg"},
            {"wav",   "audio/wav"},
            {"ogg",   "audio/ogg"},
            {"m4a",   "audio/mp4"},

            // Archives & Packages
            {"pdf",   "application/pdf"},
            {"zip",   "application/zip"},
            {"gz",    "application/gzip"},
            {"tar",   "application/x-tar"},
            {"7z",    "application/x-7z-compressed"}
        };
    }

    std::string getMimeType(std::string_view pathOrExt) {
        const std::string ext = extractExtension(pathOrExt);
        const auto it = MIME_MAP.find(ext);
        if (it != MIME_MAP.end()) {
            return it->second;
        }
        return "application/octet-stream";
    }

    bool isTextMimeType(std::string_view mimeType) {
        return mimeType.starts_with("text/") ||
               mimeType == "application/json" ||
               mimeType == "application/xml" ||
               mimeType == "image/svg+xml";
    }

    std::string formatHttpDate(std::time_t t) {
        std::tm tmBuffer{};
        gmtime_r(&t, &tmBuffer);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tmBuffer);
        return std::string(buf);
    }

    std::string formatHttpDate(std::chrono::system_clock::time_point tp) {
        return formatHttpDate(std::chrono::system_clock::to_time_t(tp));
    }

    std::optional<std::chrono::system_clock::time_point> parseHttpDate(std::string_view dateStr) {
        const std::string str(dateStr);
        std::tm tmBuffer{};

        // RFC 7231 / RFC 1123: "Sun, 06 Nov 1994 08:49:37 GMT"
        if (strptime(str.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tmBuffer) != nullptr) {
            const time_t t = timegm(&tmBuffer);
            return std::chrono::system_clock::from_time_t(t);
        }

        // RFC 850: "Sunday, 06-Nov-94 08:49:37 GMT"
        if (strptime(str.c_str(), "%A, %d-%b-%y %H:%M:%S GMT", &tmBuffer) != nullptr) {
            const time_t t = timegm(&tmBuffer);
            return std::chrono::system_clock::from_time_t(t);
        }

        // ANSI C asctime: "Sun Nov  6 08:49:37 1994"
        if (strptime(str.c_str(), "%a %b %e %H:%M:%S %Y", &tmBuffer) != nullptr) {
            const time_t t = timegm(&tmBuffer);
            return std::chrono::system_clock::from_time_t(t);
        }

        return std::nullopt;
    }

    std::chrono::system_clock::time_point toSystemTime(std::filesystem::file_time_type fileTime) {
        return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            std::chrono::file_clock::to_sys(fileTime)
        );
    }

    std::string generateETag(size_t fileSize, std::chrono::system_clock::time_point mtime) {
        const auto epochSec = std::chrono::duration_cast<std::chrono::seconds>(mtime.time_since_epoch()).count();
        std::ostringstream ss;
        ss << "W/\"" << std::hex << fileSize << "-" << epochSec << "\"";
        return ss.str();
    }

    std::string generateContentETag(std::string_view content) {
        const auto digest = keet::Crypto::sha1(content);
        return "\"" + keet::Crypto::toHex(digest) + "\"";
    }

    std::string generateFileETag(const std::filesystem::path& filePath) {
        std::error_code ec;
        const auto size = std::filesystem::file_size(filePath, ec);
        if (ec) return "";
        const auto ftime = std::filesystem::last_write_time(filePath, ec);
        if (ec) return "";
        return generateETag(size, toSystemTime(ftime));
    }

    std::string formatCacheControl(int maxAgeSeconds, bool isPublic, bool isImmutable) {
        if (maxAgeSeconds <= 0) {
            return "no-cache, no-store, must-revalidate";
        }
        std::ostringstream ss;
        if (isPublic) ss << "public, ";
        else ss << "private, ";
        ss << "max-age=" << maxAgeSeconds;
        if (isImmutable) ss << ", immutable";
        return ss.str();
    }

    bool isNotModified(const HTTPServer::ServerRequest& req,
                       std::string_view etag,
                       std::chrono::system_clock::time_point lastModified) {
        // 1. If-None-Match check (takes precedence over If-Modified-Since per RFC 7232 Section 3.2)
        const std::string ifNoneMatch = req.getHeader("if-none-match");
        if (!ifNoneMatch.empty()) {
            if (ifNoneMatch == "*") {
                return true;
            }

            // Normalize resource etag: strip leading W/ and surrounding quotes
            std::string targetTag(etag);
            if (targetTag.starts_with("W/")) targetTag = targetTag.substr(2);
            if (targetTag.starts_with("\"") && targetTag.ends_with("\"") && targetTag.size() >= 2) {
                targetTag = targetTag.substr(1, targetTag.size() - 2);
            }

            // Parse comma-separated list of ETags
            size_t start = 0;
            while (start < ifNoneMatch.size()) {
                size_t end = ifNoneMatch.find(',', start);
                if (end == std::string::npos) end = ifNoneMatch.size();
                std::string token = ifNoneMatch.substr(start, end - start);
                start = end + 1;

                const size_t first = token.find_first_not_of(" \t");
                const size_t last = token.find_last_not_of(" \t");
                if (first == std::string::npos || last == std::string::npos) continue;
                token = token.substr(first, last - first + 1);

                if (token.starts_with("W/")) token = token.substr(2);
                if (token.starts_with("\"") && token.ends_with("\"") && token.size() >= 2) {
                    token = token.substr(1, token.size() - 2);
                }

                if (token == targetTag) {
                    return true;
                }
            }
            return false;
        }

        // 2. If-Modified-Since check
        const std::string ifModifiedSince = req.getHeader("if-modified-since");
        if (!ifModifiedSince.empty() && lastModified != std::chrono::system_clock::time_point{}) {
            const auto parsed = parseHttpDate(ifModifiedSince);
            if (parsed.has_value()) {
                const auto resourceSec = std::chrono::duration_cast<std::chrono::seconds>(lastModified.time_since_epoch()).count();
                const auto clientSec = std::chrono::duration_cast<std::chrono::seconds>(parsed->time_since_epoch()).count();
                if (resourceSec <= clientSec) {
                    return true;
                }
            }
        }

        return false;
    }

    void applyNotModified(HTTPServer::ServerResponse& res,
                          std::string_view etag,
                          std::chrono::system_clock::time_point lastModified,
                          std::string_view cacheControl) {
        res.status = 304;
        res.body.clear();
        res.contentType.clear();
        res.charset.clear();
        if (!etag.empty()) {
            res.setHeader("ETag", std::string(etag));
        }
        if (lastModified != std::chrono::system_clock::time_point{}) {
            res.setHeader("Last-Modified", formatHttpDate(lastModified));
        }
        if (!cacheControl.empty()) {
            res.setHeader("Cache-Control", std::string(cacheControl));
        }
    }

} // namespace keet::StaticFiles
