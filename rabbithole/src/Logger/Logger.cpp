#include "Logger.h"
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>

using fileSink_t = spdlog::sinks::basic_file_sink_st;

void Logger::Init()
{
    auto fileSink = std::make_shared<fileSink_t>("logs/engine.log");

    ms_Logger = std::make_unique<spdlog::logger>("multi_sink", spdlog::sinks_init_list{ fileSink });
}

