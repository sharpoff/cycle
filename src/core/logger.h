#pragma once

#include <stdio.h> // IWYU pragma: keep

#include <spdlog/fmt/ranges.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/printf.h>

class Logger
{
    friend class Engine;

public:
    void Initialize();
    void Shutdown();

    template <typename... Args>
    using format_string_t = fmt::format_string<Args...>;

    template <typename... Args>
    static void Info(format_string_t<Args...> fmt, Args &&...args)
    {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }

    template <typename T>
    static void Info(const T &msg)
    {
        spdlog::info(msg);
    }

    template <typename... Args>
    static void Warning(format_string_t<Args...> fmt, Args &&...args)
    {
        spdlog::warn(fmt, std::forward<Args>(args)...);
    }

    template <typename T>
    static void Warning(const T &msg)
    {
        spdlog::warn(msg);
    }

    template <typename... Args>
    static void Error(format_string_t<Args...> fmt, Args &&...args)
    {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }

    template <typename T>
    static void Error(const T &msg)
    {
        spdlog::error(msg);
    }

private:
    Logger() {}
    Logger(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger &operator=(Logger &&) = delete;

    std::shared_ptr<spdlog::logger> logger;
};

#define LOGI(...) SPDLOG_INFO(__VA_ARGS__)
#define LOGW(...) SPDLOG_WARN(__VA_ARGS__)
#define LOGE(...) SPDLOG_ERROR(__VA_ARGS__)

#define LOG_PRINTF(format, ...) \
    SPDLOG_INFO("{}", fmt::sprintf(format __VA_OPT__(,) __VA_ARGS__))

inline Logger *gLogger = nullptr;