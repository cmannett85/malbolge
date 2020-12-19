/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/exception.hpp"

using namespace malbolge;
using namespace std::string_literals;

namespace
{
std::error_code to_ec(int ec)
{
    return std::error_code{ec, std::system_category()};
}
}

std::string malbolge::to_string(const optional_source_location& loc)
{
    if (loc) {
        return "{l:" + std::to_string(loc->line) +
               ", c:" + std::to_string(loc->column) + "}";
    }

    return "";
}

parse_exception::parse_exception(const std::string& msg,
                                 optional_source_location loc) :
    basic_exception{"Parse error " + to_string(loc) + ": " + msg},
    loc_{loc}
{}

execution_exception::execution_exception(const std::string& msg,
                                         std::size_t execution_step) :
    basic_exception{"Execution error (" + std::to_string(execution_step) +
                    "): " + msg},
    step_{execution_step}
{}

system_exception::system_exception(const std::string& msg, int error_code) :
    basic_exception{"System error: " + to_ec(error_code).message() + " - " +
                    msg},
    code_{to_ec(error_code)}
{}

system_exception::system_exception(const std::string& msg, std::errc error_code) :
    system_exception{msg, static_cast<int>(error_code)}
{}
