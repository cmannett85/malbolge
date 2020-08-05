/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <filesystem>
#include <vector>

using namespace malbolge;
using namespace std::string_literals;

namespace
{
using stream_iterator = std::istreambuf_iterator<char>;
}

virtual_memory malbolge::load(const std::filesystem::path& path)
{
    log::print(log::INFO, "Loading file: ", path);

    try {
        // Unfortunately we cannot pass the istreambuf_iterators directly to
        // load_impl, as a range requires a ForwardIterator whilst
        // istreambuf_iterator is only an InputIterator.  So we have to load
        // into an intermediary buffer first
        const auto file_size = std::filesystem::file_size(path);
        log::print(log::DEBUG, "File size: ", file_size);

        auto data = std::vector<char>{};
        data.reserve(file_size);

        {
            auto stream = std::ifstream{};
            stream.exceptions(std::ios::badbit | std::ios::failbit);
            stream.open(path, std::ios::binary);

            std::copy(stream_iterator{stream},
                      stream_iterator{},
                      std::back_inserter(data));
        }

        log::print(log::INFO, "File loaded");

        return detail::load_impl(data.begin(), data.end());
    } catch (std::exception& e) {
        throw parse_exception{"Failed to load program: "s + e.what()};
    }
}

virtual_memory malbolge::load_from_cin()
{
    log::print(log::INFO, "Loading file from stdin");

    auto program_data = ""s;
    for (auto line = ""s; std::getline(std::cin, line); ) {
        program_data += line;
    }

    log::print(log::INFO, "File loaded");

    return load(std::begin(program_data), std::end(program_data));
}
