#pragma once

#include <spdlog/spdlog.h>

#include <cstdint>
#include <stdexcept>

namespace Pilot
{

    class LogSystem final
    {
    public:

        // 日志打印等级
        enum class LogLevel : uint8_t
        {
            debug,  // 调试
            info,   // 信息
            warn,   // 警告
            error,  // 错误
            fatal   // 灾难性错误
        };

    public:
        LogSystem();
        ~LogSystem();

        template<typename... TARGS>
        void log(LogLevel level, TARGS&&... args)
        {
            switch (level)
            {
                case LogLevel::debug:
                    m_logger->debug(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::info:
                    m_logger->info(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::warn:
                    m_logger->warn(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::error:
                    m_logger->error(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::fatal:
                    m_logger->critical(std::forward<TARGS>(args)...);
                    fatalCallback(std::forward<TARGS>(args)...);
                    break;
                default:
                    break;
            }
        }

        template<typename... TARGS>
        void fatalCallback(TARGS&&... args)
        {
            const std::string format_str = fmt::format(std::forward<TARGS>(args)...);
            throw std::runtime_error(format_str);
        }

    private:
        std::shared_ptr<spdlog::logger> m_logger;
    };

} // namespace Pilot