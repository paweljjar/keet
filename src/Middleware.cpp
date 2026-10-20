//
// Created by PAWEL on 19/10/2026.
//

#include "keet/Middleware.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace keet::Middleware {

    namespace {
        std::string join(const std::vector<std::string>& items, const std::string& delimiter) {
            std::ostringstream ss;
            for (size_t i = 0; i < items.size(); ++i) {
                if (i > 0) ss << delimiter;
                ss << items[i];
            }
            return ss.str();
        }
    }

    HTTPServer::Middleware logger(LoggerOptions options) {
        if (!options.logFunction) {
            options.logFunction = [](const std::string& line) {
                std::cout << line << std::endl;
            };
        }

        return [options = std::move(options)](HTTPServer::ServerRequest& req, HTTPServer::ServerResponse& res) -> bool {
            const auto start = std::chrono::steady_clock::now();

            res.onComplete.push_back([start, options](const HTTPServer::ServerRequest& completedReq, const HTTPServer::ServerResponse& completedRes) {
                std::ostringstream ss;

                if (options.includeTimestamp) {
                    const auto now = std::chrono::system_clock::now();
                    const auto in_time_t = std::chrono::system_clock::to_time_t(now);
                    std::tm tmBuffer{};
                    localtime_r(&in_time_t, &tmBuffer);
                    char timeBuf[32];
                    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmBuffer);
                    ss << "[" << timeBuf << "] ";
                }

                if (options.includeRemoteAddr && !completedReq.remoteAddr.empty()) {
                    ss << completedReq.remoteAddr << " - ";
                }

                ss << completedReq.method << " " << completedReq.path;

                if (options.includeStatus) {
                    ss << " " << completedRes.status << " " << HTTPServer::Server::getStatusMessage(completedRes.status);
                }

                if (options.includeDuration) {
                    const auto end = std::chrono::steady_clock::now();
                    const auto elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
                    ss << " (" << std::fixed << std::setprecision(2) << elapsedMs << " ms)";
                }

                options.logFunction(ss.str());
            });

            return true;
        };
    }

    HTTPServer::Middleware cors(CorsOptions options) {
        return [options = std::move(options)](HTTPServer::ServerRequest& req, HTTPServer::ServerResponse& res) -> bool {
            const std::string reqOrigin = req.getHeader("origin");
            std::string matchedOrigin;

            if (!options.allowedOrigins.empty()) {
                for (const auto& allowed : options.allowedOrigins) {
                    if (allowed == "*" || allowed == reqOrigin) {
                        matchedOrigin = (allowed == "*" && !reqOrigin.empty()) ? reqOrigin : allowed;
                        break;
                    }
                }
            } else if (!options.origin.empty()) {
                if (options.origin == "*" && options.allowCredentials && !reqOrigin.empty()) {
                    matchedOrigin = reqOrigin;
                } else {
                    matchedOrigin = options.origin;
                }
            }

            if (!matchedOrigin.empty()) {
                res.setHeader("Access-Control-Allow-Origin", matchedOrigin);
            }

            if (options.allowCredentials) {
                res.setHeader("Access-Control-Allow-Credentials", "true");
            }

            if (!options.exposedHeaders.empty()) {
                res.setHeader("Access-Control-Expose-Headers", join(options.exposedHeaders, ", "));
            }

            if (req.method == "OPTIONS") {
                if (!options.allowedMethods.empty()) {
                    res.setHeader("Access-Control-Allow-Methods", join(options.allowedMethods, ", "));
                }
                if (!options.allowedHeaders.empty()) {
                    res.setHeader("Access-Control-Allow-Headers", join(options.allowedHeaders, ", "));
                }
                if (options.maxAge > 0) {
                    res.setHeader("Access-Control-Max-Age", std::to_string(options.maxAge));
                }

                res.setHeader("Vary", "Origin");
                res.status = 204;
                res.body = "";
                return false; // Short-circuit preflight
            }

            return true;
        };
    }

    HTTPServer::Middleware securityHeaders(SecurityOptions options) {
        return [options = std::move(options)](HTTPServer::ServerRequest&, HTTPServer::ServerResponse& res) -> bool {
            if (!options.xContentTypeOptions.empty()) {
                res.setHeader("X-Content-Type-Options", options.xContentTypeOptions);
            }
            if (!options.xFrameOptions.empty()) {
                res.setHeader("X-Frame-Options", options.xFrameOptions);
            }
            if (!options.xXssProtection.empty()) {
                res.setHeader("X-XSS-Protection", options.xXssProtection);
            }
            if (!options.referrerPolicy.empty()) {
                res.setHeader("Referrer-Policy", options.referrerPolicy);
            }
            if (!options.contentSecurityPolicy.empty()) {
                res.setHeader("Content-Security-Policy", options.contentSecurityPolicy);
            }
            if (!options.strictTransportSecurity.empty()) {
                res.setHeader("Strict-Transport-Security", options.strictTransportSecurity);
            }
            if (!options.permissionsPolicy.empty()) {
                res.setHeader("Permissions-Policy", options.permissionsPolicy);
            }
            return true;
        };
    }

} // namespace keet::Middleware
