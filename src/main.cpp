/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"
#include "malbolge/version.hpp"
#include "malbolge/utility/argument_parser.hpp"

#include <iostream>
#include <optional>

using namespace malbolge;
using namespace std::string_literals;

namespace
{
virtual_cpu create_vcpu(const argument_parser& parser)
{
    const auto& file = parser.file();
    if (file) {
        // Load the file off disk
        return virtual_cpu{load(*file)};
    } else {
        // Load from stdin
        return virtual_cpu{load_from_cin()};
    }
}
}

int main(int argc, char* argv[])
{
    try {
        auto arg_parser = argument_parser{argc, argv};
        if (arg_parser.help()) {
            std::cout << arg_parser << std::endl;
            return EXIT_SUCCESS;
        }

        if (arg_parser.version()) {
            std::cout << "Malbolge Virtual Machine v"
                      << version_string
                      << "\nCopyright Cam Mannett 2020" << std::endl;
            return EXIT_SUCCESS;
        }

        log::set_log_level(arg_parser.log_level());

        auto vcpu = create_vcpu(arg_parser);
        auto fut = vcpu.run();
        fut.get();
    } catch (std::system_error& e) {
        log::print(log::ERROR, e.what());
        return e.code().value();
    } catch (system_exception& e) {
        log::print(log::ERROR, e.what());
        return e.code().value();
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
