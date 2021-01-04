/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/argument_parser.hpp"
#include "malbolge/utility/string_view_ops.hpp"
#include "malbolge/algorithm/container_ops.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/version.hpp"

#include <deque>

using namespace malbolge;
using namespace utility::string_view_ops;
using namespace std::literals::string_view_literals;
using namespace std::string_literals;

namespace
{
constexpr auto help_flags           = std::array{"--help", "-h"};
constexpr auto version_flags        = std::array{"--version", "-v"};
constexpr auto log_flag_prefix      = "-l";
constexpr auto string_flag          = "--string";
constexpr auto debugger_script_flag = "--debugger-script";
constexpr auto force_nn_flag        = "--force-non-normalised";
}

argument_parser::argument_parser(int argc, char* argv[]) :
    help_{false},
    version_{false},
    p_{program_source::STDIN, ""},
    log_level_{log::ERROR},
    force_nn_{false}
{
    // Convert to string_views, they're easier to work with
    auto args = std::deque<std::string_view>(argc-1);
    std::copy(argv+1, argv+argc, args.begin());

    // Help
    if (algorithm::any_of_container(args, help_flags)) {
        if (args.size() != 1) {
            throw system_exception{"Help flag must be unique",
                                   std::errc::invalid_argument};
        }

        help_ = true;
        return;
    }

    // Application version
    if (algorithm::any_of_container(args, version_flags)) {
        if (args.size() != 1) {
            throw system_exception{"Version flag must be unique",
                                   std::errc::invalid_argument};
        }

        version_ = true;
        return;
    }

    // Force non-normalised
    auto force_nn_it = std::find(args.begin(), args.end(), force_nn_flag);
    if (force_nn_it != args.end()) {
        force_nn_ = true;
        args.erase(force_nn_it);
    }

    auto string_it = std::find(args.begin(), args.end(), string_flag);
    if (string_it != args.end()) {
        // Move the iterator forward one to extract the program data
        if (++string_it == args.end()) {
            throw system_exception{
                "String flag set but no program source present",
                std::errc::invalid_argument
            };
        }

        p_.source = program_source::STRING;
        p_.data = std::move(*string_it);

        string_it = args.erase(--string_it);
        args.erase(string_it);
    }

    auto debugger_script_it = std::find(args.begin(), args.end(), debugger_script_flag);
    if (debugger_script_it != args.end()) {
        // Move the iterator forward one to extract the script path
        if (++debugger_script_it == args.end()) {
            throw system_exception{
                "String flag set but no program source present",
                std::errc::invalid_argument
            };
        }

        debugger_script_ = *debugger_script_it;

        debugger_script_it = args.erase(--debugger_script_it);
        args.erase(debugger_script_it);
    }

    // Log level
    if (args.size() && args.front().starts_with(log_flag_prefix)) {
        // There must only be 'l's
        const auto level = static_cast<std::size_t>(std::count(args.front().begin(),
                                                               args.front().end(),
                                                               'l'));
        if (level == (args.front().size()-1)) {
            if (level >= log::NUM_LOG_LEVELS) {
                throw system_exception{
                    "Maximum log level is "s + to_string(log::VERBOSE_DEBUG) +
                    " (" + std::to_string(log::NUM_LOG_LEVELS-1) + ")",
                    std::errc::invalid_argument
                };
            }

            log_level_ = static_cast<log::level>(log::NUM_LOG_LEVELS - 1 - level);
            args.pop_front();
        }
    }

    // There should be no other flags here
    {
        auto it = std::find_if(args.begin(), args.end(), [](auto&& a) {
            return a.starts_with('-');
        });
        if (it != args.end()) {
            throw system_exception{"Unknown argument: "s + *it,
                                   std::errc::invalid_argument};
        }
    }

    // There should either be a path for a file to load, or nothing
    if (!args.empty()) {
        // Make sure the string flag hadn't already been set
        if (p_.source == program_source::STRING) {
            throw system_exception{
                "String flag already set",
                EINVAL
            };
        }

        p_.source = program_source::DISK;
        p_.data = std::move(args.front());
        args.pop_front();

        if (!args.empty()) {
            throw system_exception{"Unknown argument: "s + args.front(),
                                   std::errc::invalid_argument};
        }
    }
}

std::ostream& malbolge::operator<<(std::ostream& stream,
                                   const argument_parser::program_source& source)
{
    static_assert(static_cast<std::size_t>(argument_parser::program_source::MAX) == 3,
                  "program_source enum has changed size, update operator<<");

    switch (source) {
    case argument_parser::program_source::DISK:
        return stream << "DISK";
    case argument_parser::program_source::STDIN:
        return stream << "STDIN";
    case argument_parser::program_source::STRING:
        return stream << "STRING";
    default:
        return stream << "Unknown";
    }
}

std::ostream& malbolge::operator<<(std::ostream& stream, const argument_parser&)
{
    return stream << "Malbolge virtual machine v" << project_version
                  << "\nUsage:"
                     "\n\tmalbolge [options] <file>\n"
                     "\tcat <file> | malbolge [options]\n\n"
                     "Options:\n"
                  << "\t" << help_flags[1] << " " << help_flags[0]
                  << "\t\tDisplay this help message\n"
                  << "\t" << version_flags[1] << " " << version_flags[0]
                  << "\t\tDisplay the full application version\n"
                  << "\t" << log_flag_prefix
                  << "\t\t\tLog level, repeat the l character for higher logging levels\n"
                  << "\t" << string_flag
                  << "\t\tPass a string argument as the program to run\n"
                  << "\t" << debugger_script_flag
                  << "\tRun the given debugger script on the program\n"
                  << "\t" << force_nn_flag
                  << "\tOverride normalised program detection to force to non-normalised";
}
