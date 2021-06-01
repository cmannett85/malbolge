/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"
#include "malbolge/utility/file_load.hpp"

#include <vector>

using namespace malbolge;
using namespace std::string_literals;

namespace
{
using stream_iterator = std::istreambuf_iterator<char>;
}

std::ostream& malbolge::operator<<(std::ostream& stream,
                                   load_normalised_mode mode)
{
    switch (mode) {
    case load_normalised_mode::AUTO:
        return stream << "AUTO";
    case load_normalised_mode::ON:
        return stream << "ON";
    case load_normalised_mode::OFF:
        return stream << "OFF";
    default:
        return stream << "Unknown";
    }
}

virtual_memory malbolge::load(const std::filesystem::path& path,
                              load_normalised_mode mode)
{
    log::print(log::INFO, "Loading file: ", path);

    try {
        // Unfortunately we cannot pass the istreambuf_iterators directly to
        // load_impl, as a range requires a ForwardIterator whilst
        // istreambuf_iterator is only an InputIterator.  So we have to load
        // into an intermediary buffer first
        auto data = utility::file_load<std::vector<char>>(path);
        log::print(log::INFO, "File loaded (", data.size(), " bytes)");

        return load(data.begin(), data.end(), mode);
    } catch (parse_exception& e) {
        throw;
    } catch (std::exception& e) {
        throw parse_exception{"Failed to load program: "s + e.what()};
    }
}

virtual_memory malbolge::load_from_cin(load_normalised_mode mode)
{
    log::print(log::INFO, "Loading file from stdin");

    auto program_data = ""s;
    for (auto line = ""s; std::getline(std::cin, line); ) {
        program_data += line;
    }

    log::print(log::INFO, "File loaded");

    return load(std::begin(program_data), std::end(program_data), mode);
}
