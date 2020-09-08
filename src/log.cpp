/* Copyright Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/log.hpp"

#include <chrono>
#include <iomanip>

using namespace malbolge;

namespace
{
log::level filter_level = log::INFO;
}

std::ostream& log::detail::log_prefix(std::ostream& stream)
{
    const auto now = std::chrono::system_clock::now();
    const auto c_now = std::chrono::system_clock::to_time_t(now);

#ifdef EMSCRIPTEN
    // Browsers barely have millisecond accurate timers, so reduce from ns to ms
    const auto fractional = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % std::milli::den;
    constexpr auto width = 3;
#else
    const auto fractional = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count() % std::nano::den;
    constexpr auto width = 9;
#endif

    return stream << std::put_time(std::localtime(&c_now), "%F %T")
                  << "." << std::setfill('0') << std::setw(width) << fractional
                  << "[";
}

const char* log::to_string(level lvl)
{
    static_assert(NUM_LOG_LEVELS == 4, "Log levels have changed, update this");

    switch (lvl) {
    case VERBOSE_DEBUG:
        return "VERBOSE DEBUG";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case ERROR:
        return "ERROR";
    default:
        return "Unknown";
    }
}

log::level log::log_level()
{
    return filter_level;
}

void log::set_log_level(level lvl)
{
    filter_level = lvl;
}
