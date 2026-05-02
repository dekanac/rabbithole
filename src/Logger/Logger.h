#pragma once

#include "common.h"
#include "signal.h"
#include <memory>
#include <spdlog/spdlog.h>

using externLogger_t = spdlog::logger;

class Logger
{
  public:
    static void Init();

    inline static externLogger_t& GetLogger() { return *ms_Logger; }

  private:
    inline static std::unique_ptr<externLogger_t> ms_Logger;

    NonCopyableAndMovable(Logger);
};

#ifdef _WIN32
    #define LOG_BREAK() DebugBreak()
#elif defined(__linux__) || defined(__unix__)
    #define LOG_BREAK() raise(SIGTRAP)
#else
    #define LOG_BREAK()
#endif

#ifdef RABBITHOLE_DEBUG

#define LOG_CRITICAL(...)                                                                          \
    do                                                                                             \
    {                                                                                              \
        ::Logger::GetLogger().critical(__VA_ARGS__);                                               \
        LOG_BREAK();                                                                               \
    } while (0)

#define LOG_ERROR(...)                                                                             \
    do                                                                                             \
    {                                                                                              \
        ::Logger::GetLogger().error(__VA_ARGS__);                                                  \
        LOG_BREAK();                                                                               \
    } while (0)

#define LOG_WARNING(...)                                                                           \
    do                                                                                             \
    {                                                                                              \
        ::Logger::GetLogger().warn(__VA_ARGS__);                                                   \
    } while (0)

#define LOG_INFO(...)                                                                              \
    do                                                                                             \
    {                                                                                              \
        ::Logger::GetLogger().info(__VA_ARGS__);                                                   \
    } while (0)

#define ASSERT(x, ...)                                                                             \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            ::Logger::GetLogger().critical(__VA_ARGS__);                                           \
           LOG_BREAK();                                                                            \
        }                                                                                          \
    } while (0)

#else // RABBITHOLE_DEBUG

// These are the release mode definitions for the macros above.
#define LOG_CRITICAL(...)
#define LOG_ERROR(...)
#define LOG_WARNING(...)
#define LOG_INFO(...)
#define ASSERT(x, ...)

#endif // !defined NDEBUG