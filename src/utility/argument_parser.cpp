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
}

argument_parser::argument_parser(int argc, char* argv[]) :
    help_{false},
    version_{false},
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

    // Log level must be next
    if (args.size() && args.front().starts_with(log_flag_prefix)) {
        // There must only be 'l's
        const auto level = static_cast<std::size_t>(std::ranges::count(args.front(), 'l'));
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
        auto it = std::ranges::find_if(args, [](auto&& a) {
            return a.starts_with('-');
        });
        if (it != args.end()) {
            throw system_exception{"Unknown argument: " + *it, EINVAL};
        }
    }

    // There should either be a path for a file to load, or nothing
    if (!args.empty()) {
        file_ = args.front();
        args.pop_front();

        if (!args.empty()) {
            throw system_exception{"Unknown argument: " + args.front(),
                                   EINVAL};
        }
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
