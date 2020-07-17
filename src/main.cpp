/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"
#include "malbolge/version.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <optional>

using namespace malbolge;
using namespace std::string_literals;
namespace po = boost::program_options;

namespace
{
virtual_cpu create_vcpu(const po::variables_map& vm)
{
    if (vm.count("file")) {
        // Load the file off disk
        const auto& file_path = vm["file"].as<const std::string&>();
        return virtual_cpu{load(std::filesystem::path{file_path})};
    } else {
        // Load from stdin
        return virtual_cpu{load_from_cin()};
    }
}

// All this just to count tokenless flags...
struct verbose_counter
{
    verbose_counter(int init = 0) :
        count{init}
    {}

    int count;
};

std::ostream& operator<<(std::ostream& stream, verbose_counter vc)
{
    return stream << vc.count;
}

void validate(boost::any& v,
              const std::vector<std::string>&,
              verbose_counter*,
              int)
{
    if (v.empty()) {
        v = verbose_counter{1};
    } else {
        auto& count = boost::any_cast<verbose_counter&>(v).count;
        if (count == logging::ERROR) {
            throw system_exception{
                "Maximum log level is "s + to_string(logging::VERBOSE_DEBUG) +
                " (" + std::to_string(static_cast<int>(logging::ERROR)) + ")",
                EINVAL
            };
        }

        ++count;
    }
}
}

int main(int argc, char* argv[])
{
    logging::init_logging();
    auto vcounter = verbose_counter{};

    auto desc = po::options_description{
        "Malbolge virtual machine v"s + project_version + "\n"
        "Usage:\n\tmalbolge <file>"
        "\n\tcat <file> | malbolge\n\n"
        "Arguments"};
    desc.add_options()
        ("help,h",      "Help message")
        ("version",     "Application version")
        ("file,f",      po::value<std::string>(), "Input file")
        ("verbose,v",   po::value<verbose_counter>(&vcounter)
                            ->default_value(verbose_counter{})
                            ->zero_tokens(),
                        "Number of times this flag appears sets the log level");

    auto pos_arg = po::positional_options_description{};
    pos_arg.add("file", 1);

    try {
        auto vm = po::variables_map{};
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(pos_arg).run(), vm);
        po::notify(vm);

        const auto log_level = logging::ERROR - vcounter.count;
        logging::set_log_level(static_cast<logging::level>(log_level));

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        if (vm.count("version")) {
            std::cout << "Malbolge Virtual Machine v"
                      << version_string
                      << "\nCopyright Cam Mannett 2020" << std::endl;
            return EXIT_SUCCESS;
        }

        auto vcpu = create_vcpu(vm);
        auto fut = vcpu.run();
        fut.get();
    } catch (po::error& e) {
        auto new_e = system_exception{e.what(), EINVAL};
        BOOST_LOG_SEV(logging::source::get(), logging::ERROR)
            << new_e.what();
        return new_e.code().value();
    } catch (std::system_error& e) {
        BOOST_LOG_SEV(logging::source::get(), logging::ERROR)
            << e.what();
        return e.code().value();
    } catch (system_exception& e) {
        BOOST_LOG_SEV(logging::source::get(), logging::ERROR)
            << e.what();
        return e.code().value();
    } catch (std::exception& e) {
        BOOST_LOG_SEV(logging::source::get(), logging::ERROR)
            << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
