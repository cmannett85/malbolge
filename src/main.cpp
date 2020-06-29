/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <optional>

using namespace malbolge;
using namespace std::string_literals;
namespace po = boost::program_options;

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

int main(int argc, char* argv[])
{
    auto desc = po::options_description{"Malbolge virtual machine\n"
        "Usage:\n\tmalbolge <file>"
        "\n\tcat <file> | malbolge\n\n"
        "Arguments"};
    desc.add_options()
        ("help,h", "Help message")
        ("file,f", po::value<std::string>(), "Input file");

    auto pos_arg = po::positional_options_description{};
    pos_arg.add("file", 1);

    auto vm = po::variables_map{};
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(pos_arg).run(), vm);
    po::notify(vm);

    try {
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        auto vcpu = create_vcpu(vm);
        auto fut = vcpu.run();
        fut.get();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
