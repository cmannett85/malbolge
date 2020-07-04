/* Copyright Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/logging.hpp"

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>

#include <iostream>

using namespace malbolge;
using ptime = boost::posix_time::ptime;
namespace boost_expr = boost::log::expressions;

namespace
{
using text_sink = boost::log::sinks::synchronous_sink<
                    boost::log::sinks::text_ostream_backend>;

const auto formatter = boost_expr::stream
    << boost_expr::format_date_time<ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
    << "[" << boost_expr::attr<logging::level>("Severity") << "]: "
    << boost_expr::smessage;
}

BOOST_LOG_GLOBAL_LOGGER_INIT(logging::source, logging::source_type)
{
    return logging::source_type{};
}

const char* logging::to_string(level lvl)
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

void logging::init_logging()
{
    boost::log::add_common_attributes();

    // Create an error stream backend
    auto sink = boost::make_shared<text_sink>();
    auto stream = boost::shared_ptr<std::ostream>{
        &std::clog,
        boost::null_deleter{}
    };

    sink->set_formatter(formatter);

    sink->locked_backend()->add_stream(std::move(stream));
    boost::log::core::get()->add_sink(std::move(sink));

    set_log_level(logging::ERROR);
}

void logging::set_log_level(level lvl)
{
    boost::log::core::get()->set_filter(boost_expr::attr<level>("Severity") >= lvl);
}
