//
// Created by August on 21-2-23.
//

#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <memory>

namespace sylk {

    enum class ELogLevel : uint8_t {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        CRITICAL,
        OFF
    };

        static constexpr ELogLevel TRACE {ELogLevel::TRACE};
        static constexpr ELogLevel DEBUG {ELogLevel::DEBUG};
        static constexpr ELogLevel INFO {ELogLevel::INFO};
        static constexpr ELogLevel WARN {ELogLevel::WARN};
        static constexpr ELogLevel ERROR {ELogLevel::ERROR};
        static constexpr ELogLevel CRITICAL {ELogLevel::CRITICAL}; // Terminates execution

#ifndef SYLK_LOG_LEVEL
#   if defined(SYLK_VERBOSE)
#       define SYLK_LOG_LEVEL ELogLevel::TRACE
#   elif defined(SYLK_DEBUG)
#       define SYLK_LOG_LEVEL ELogLevel::DEBUG
#   elif defined(SYLK_RELEASE)
#       define SYLK_LOG_LEVEL ELogLevel::INFO
#   else
#       define SYLK_LOG_LEVEL ELogLevel::OFF
#   endif
#endif

    template<typename... Args>
    void log(ELogLevel log_level, const char* msg, Args&&... args);

    class Internal_Log_ {
        using SpdLogger = std::shared_ptr<spdlog::logger>;

    public:
        template<typename... Args>
        friend void log(ELogLevel log_level, const char* msg, Args&&... args);

    private:
        Internal_Log_() = default;

        static void init() {
            if (!was_initialized) {
                was_initialized = true;

                logger->set_pattern("%^%v%$");
                logger->set_level(static_cast<spdlog::level::level_enum>(SYLK_LOG_LEVEL));
                logger->log(static_cast<spdlog::level::level_enum>(ELogLevel::INFO), "--- Sylk v{}\n", SYLK_VERSION);

                logger->set_pattern("%^<%n>%$ %v");
            }
        }
        inline static bool was_initialized {false};
        inline static SpdLogger logger {spdlog::stdout_color_st("Sylk")};
    };

    template<typename... Args>
    void log(ELogLevel log_level, const char* msg, Args&&... args) {
        if (!Internal_Log_::was_initialized) {
            Internal_Log_::init();
        }

        const auto runtime_msg = fmt::runtime(msg);

        switch (log_level) {
            case ELogLevel::TRACE:
                Internal_Log_::logger->trace(runtime_msg, args...);
                break;
            case ELogLevel::DEBUG:
                Internal_Log_::logger->debug(runtime_msg, args...);
                break;
            case ELogLevel::INFO:
                Internal_Log_::logger->info(runtime_msg, args...);
                break;
            case ELogLevel::WARN:
                Internal_Log_::logger->warn(runtime_msg, args...);
                break;
            case ELogLevel::ERROR:
                Internal_Log_::logger->error(runtime_msg, args...);
                break;
            case ELogLevel::CRITICAL:
                Internal_Log_::logger->critical(runtime_msg, args...);
                std::abort();
            default:
                Internal_Log_::logger->error("Invalid log level specified.");
        }
    }

    template<typename... Args>
    void log(const char* msg, Args&&... args) {
        log(INFO, msg, args...);
    }
}
