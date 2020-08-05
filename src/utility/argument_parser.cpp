/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/argument_parser.hpp"
#include "malbolge/algorithm/container_ops.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/version.hpp"

#include <deque>

using namespace malbolge;
using namespace std::literals::string_view_literals;
using namespace std::string_literals;

namespace
{
constexpr auto help_flags = std::array{"--help", "-h"};
constexpr auto version_flags = std::array{"--version", "-v"};
constexpr auto log_flag_prefix = "-l";
constexpr auto string_flag = "--string";
}

argument_parser::argument_parser(int argc, char* argv[]) :
    help_{false},
    version_{false},
    p_{program_source::STDIN, ""},
    log_level_{log::ERROR}
{
    // Convert to strings, they're easier to work with and we only do this
    // once
    auto args = std::deque<std::string>(argc-1);
    std::copy(argv+1, argv+argc, args.begin());

    // Help
    if (any_of_container(args, help_flags)) {
        if (args.size() != 1) {
            throw system_exception{"Help flag must be unique", EINVAL};
        }

        help_ = true;
        return;
    }

    // Application version
    if (any_of_container(args, version_flags)) {
        if (args.size() != 1) {
            throw system_exception{"Version flag must be unique", EINVAL};
        }

        version_ = true;
        return;
    }

#ifdef EMSCRIPTEN
    auto string_it = std::find(args.begin(), args.end(), string_flag);
#else
    auto string_it = std::ranges::find(args, string_flag);
#endif
    if (string_it != args.end()) {
        // Move the iterator forward one to extract the program data
        if (++string_it == args.end()) {
            throw system_exception{
                "String flag set but no program source present",
                EINVAL
            };
        }

        p_.source = program_source::STRING;
        p_.data = std::move(*string_it);

        string_it = args.erase(--string_it);
        args.erase(string_it);
    }

    // Log level
    if (args.size() && args.front().starts_with(log_flag_prefix)) {
        // There must only be 'l's
#ifdef EMSCRIPTEN
        const auto level = static_cast<std::size_t>(std::count(args.front().begin(),
                                                               args.front().end(),
                                                               'l'));
#else
        const auto level = static_cast<std::size_t>(std::ranges::count(args.front(), 'l'));
#endif
        if (level == (args.front().size()-1)) {
            if (level > 3) {
                throw system_exception{
                    "Maximum log level is "s + to_string(log::VERBOSE_DEBUG) +
                    " (" + std::to_string(log::NUM_LOG_LEVELS-1) + ")",
                    EINVAL
                };
            }

            log_level_ = static_cast<log::level>(log::ERROR - level);
            args.pop_front();
        }
    }

    // There should be no other flags here
    {
#ifdef EMSCRIPTEN
        auto it = std::find_if(args.begin(), args.end(), [](auto&& a) {
#else
        auto it = std::ranges::find_if(args, [](auto&& a) {
#endif
            return a.starts_with('-');
        });
        if (it != args.end()) {
            throw system_exception{"Unknown argument: " + *it, EINVAL};
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
            throw system_exception{"Unknown argument: " + args.front(),
                                   EINVAL};
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
                     "\n\tmalbolge <file>\n"
                     "\tcat <file> | malbolge\n\n"
                     "Options:\n"
                  << help_flags[1] << " " << help_flags[0]
                  << "\tDisplay this help message\n"
                  << version_flags[1] << " " << version_flags[0]
                  << "\tDisplay the full application version\n"
                  << log_flag_prefix
                  << "\t\tLog level, repeat the l character for higher logging levels\n";
}
