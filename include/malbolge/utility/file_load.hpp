/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/traits.hpp"
#include "malbolge/exception.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <algorithm>

namespace malbolge
{
namespace utility
{
namespace file_load_detail
{
template <typename T>
using has_size_constructor = decltype(T(std::declval<typename T::size_type>(),
                                        std::declval<typename T::value_type>()));
}

/** Loads the file at @a path into a @a Container instance.
 *
 * If @a Container has a constructor with a single size_type argument then it is
 * assumed to be constructor that sets the container size, otherwise it is
 * assumed to be fixed-size array.
 * @tparam Container Container type to return
 * @param path File path to load
 * @exception parse_exception Thrown if the file cannot be read, or if
 * @a Container is fixed-size and is not big enough to hold the file's contents
 * @return @a Container instance holding the file's data
 */
template <typename Container>
[[nodiscard]]
Container file_load(const std::filesystem::path& path)
{
    using namespace std::string_literals;
    using std::begin;

    auto data = Container{};
    try {
        const auto file_size = std::filesystem::file_size(path);
        if constexpr (traits::is_detected_v<file_load_detail::has_size_constructor,
                                            Container>) {
            data = Container(file_size, 0);
        } else if (file_size > data.size()) {
            throw parse_exception{"Failed to load program: File size too large "
                                  "for fixed-size container"};
        }

        auto stream = std::ifstream{};
        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.open(path, std::ios::binary);

        std::copy(std::istreambuf_iterator<char>{stream},
                  std::istreambuf_iterator<char>{},
                  begin(data));
    } catch (parse_exception&) {
        throw;
    } catch (std::exception& e) {
        throw parse_exception{"Failed to load program: "s + e.what()};
    }

    return data;
}
}
}
