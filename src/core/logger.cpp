#include "core/logger.h"
#include "core/constants.h"

#include "spdlog/sinks/dup_filter_sink.h"

void Logger::Initialize()
{
    // console and file sinks
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogsDir / "log.txt", true);

    // duplicate filter
    auto dupFilter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(std::chrono::seconds(5));
    dupFilter->add_sink(consoleSink);
    dupFilter->add_sink(fileSink);

    // main logger
    logger = std::make_shared<spdlog::logger>("logger", dupFilter);
    spdlog::set_default_logger(logger);

    spdlog::set_pattern("[%^%l%$][%T.%e][%!] %v");

    // spdlog::flush_on(spdlog::level::info);
    spdlog::flush_on(spdlog::level::err);
    spdlog::flush_on(spdlog::level::warn);
}

void Logger::Shutdown()
{
}