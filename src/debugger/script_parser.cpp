/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/script_parser.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/utility/from_chars.hpp"
#include "malbolge/utility/string_view_ops.hpp"
#include "malbolge/utility/tuple_iterator.hpp"
#include "malbolge/utility/unescaper.hpp"
#include "malbolge/algorithm/trim.hpp"

#include "malbolge/log.hpp"

#include <vector>
#include <fstream>

using namespace malbolge;
using namespace debugger;
using namespace utility::string_view_ops;
using namespace std::string_literals;


namespace
{
using function_types = traits::arg_extractor<script::functions::function_variant>;

struct argument_string
{
    explicit argument_string(std::string_view n) :
        name(n)
    {}

    std::string_view name;
    std::string_view value;
    source_location src_loc;
};

void trim_whitespace(std::string_view& str)
{
    algorithm::trim(str, [](auto c) { return std::isspace(c); });
}

bool only_whitespace(std::string_view str)
{
    return std::all_of(str.begin(), str.end(), [](auto c) { return std::isspace(c); });
}

void update_src_loc(std::string_view str, source_location& src_loc)
{
    if (str.empty()) {
        return;
    }

    // Count the number of new lines
    const auto new_lines = std::count(str.begin(), str.end(), '\n');
    if (new_lines) {
        // If we have new lines, find the distance between the last newline and
        // the string end, as that is the number of columns set
        src_loc.line += new_lines;

        const auto last_new_line_index = str.find_last_of('\n');
        src_loc.column = (str.size() - 1) - last_new_line_index;
    } else {
        src_loc.column += str.size();
    }
}

void check_fn_name(std::string_view fn_name, const source_location& src_loc)
{
    auto result = false;
    utility::tuple_type_iterator<function_types>([&](auto, auto ptr) {
        using Fn = std::remove_pointer_t<decltype(ptr)>;

        if (Fn::name() == fn_name) {
            result = true;
        }
    });

    if (!result) {
        throw parse_exception{"Unrecognised function name: "s + fn_name, src_loc};
    }
}

std::string_view extract_fn_name(std::string_view cmd, source_location& src_loc)
{
    // Read up to the first bracket
    const auto first_bracket_index = cmd.find_first_of('(');
    if (first_bracket_index == std::string_view::npos) {
        update_src_loc(cmd, src_loc);
        throw parse_exception{"No open bracket in function", src_loc};
    } else if (first_bracket_index == 0) {
        ++src_loc.column; // '('
        throw parse_exception{"No function name", src_loc};
    }

    cmd = cmd.substr(0, first_bracket_index);
    update_src_loc(cmd, src_loc);
    if (only_whitespace(cmd)) {
        throw parse_exception{"No function name", src_loc};
    }

    trim_whitespace(cmd);
    check_fn_name(cmd, src_loc);

    return cmd;
}

std::vector<argument_string> extract_fn_args(std::string_view cmd,
                                             source_location& src_loc)
{
    {
        // This has already been tested, so we know open_index is not npos
        const auto open_index = cmd.find_first_of('(');
        cmd = cmd.substr(open_index+1);
        src_loc.column += 1; // '('

        const auto close_index = cmd.find_last_of(')');
        if (close_index == std::string_view::npos) {
            update_src_loc(cmd, src_loc);
            throw parse_exception{"No close bracket in function", src_loc};
        }

        cmd = cmd.substr(0, close_index);
    }

    // Iterate through each character, and mark important characters:
    // = is the argument/value divider
    // , is the argument divider
    // \ is the escape character
    // " is the string start/end character
    //
    // This task is complicated by the fact that any of these characters can be
    // appear inside a string value (including the double quotes if preceded by
    // the escape character), but must be ignored
    auto result = std::vector<argument_string>{};
    auto inside_string = false;
    auto escaped = false;
    auto word = std::string_view{cmd.data(), 0};
    for (auto it = cmd.begin(); it != cmd.end(); ++it) {
        switch (*it) {
        case '=':
        {
            if (inside_string) {
                break;
            }
            escaped = false;
            result.emplace_back(word);
            word = std::string_view{word.data() + word.size() + 1, 0};
            continue;
        }
        case ',':
        {
            if (inside_string) {
                break;
            }
            if (result.empty() || !result.back().value.empty()) {
                const auto index = static_cast<std::size_t>(std::distance(cmd.begin(), it));
                update_src_loc({cmd.data(), index+1}, src_loc);
                throw parse_exception{"Missing argument value", src_loc};
            }
            escaped = false;

            result.back().value = word;
            word = std::string_view{word.data() + word.size() + 1, 0};
            continue;
        }
        case '\\':
        {
            escaped = true;
            break;
        }
        case '"':
        {
            if (escaped) {
                escaped = false;
                break;
            }
            inside_string = !inside_string;
            break;
        }
        default:
            escaped = false;
            break;
        }

        word = std::string_view{word.data(), word.size() + 1};
    }

    if (inside_string) {
        update_src_loc(cmd, src_loc);
        throw parse_exception{"Unterminated string", src_loc};
    }

    // Last value
    if (!word.empty() && !only_whitespace(word)) {
        if (result.empty() || !result.back().value.empty()) {
            update_src_loc(cmd, src_loc);
            throw parse_exception{"Missing argument name", src_loc};
        } else if (!result.empty()) {
            result.back().value = word;
        }
    }

    // Check that each field has been populated, and trim off whitespace
    for (auto& arg : result) {
        // Name
        arg.src_loc = src_loc;
        update_src_loc(arg.name, src_loc);
        src_loc.column += 1; // '='
        trim_whitespace(arg.name);
        if (arg.name.empty()) {
            throw parse_exception{"Missing argument name", src_loc};
        }

        // Value
        arg.src_loc = src_loc;
        update_src_loc(arg.value, src_loc);
        src_loc.column += 1; // ','
        trim_whitespace(arg.value);
        if (arg.value.empty()) {
            throw parse_exception{"Missing argument value", src_loc};
        }
    }
    src_loc.column -= 1; // Removing trailing comma compensation for last entry

    return result;
}

template <typename Arg>
auto create_arg_value(const argument_string& arg_str)
{
    static_assert(std::tuple_size_v<script::type::all> == 4,
                  "Number of script types changed, update this function");

    try {
        if constexpr (std::is_same_v<typename Arg::value_type, script::type::uint>) {
            return utility::from_chars<typename Arg::value_type>(arg_str.value);
        } else if constexpr (std::is_same_v<typename Arg::value_type, script::type::ternary>) {
            return utility::from_chars<typename Arg::value_type>(arg_str.value);
        } else if constexpr (std::is_same_v<typename Arg::value_type, script::type::reg>) {
            if (arg_str.value == "A") {
                return script::type::reg::A;
            } else if (arg_str.value == "C") {
                return script::type::reg::C;
            } else if (arg_str.value == "D") {
                return script::type::reg::D;
            } else {
                throw parse_exception{"Unrecognised vCPU register ID: "s + arg_str.value,
                                    arg_str.src_loc};
            }
        } else if constexpr (std::is_same_v<typename Arg::value_type, script::type::string>) {
            // Strip off the leading and trailing double quotes
            auto string_arg = arg_str.value;
            string_arg.remove_prefix(1);
            string_arg.remove_suffix(1);
            return utility::unescape_ascii(string_arg);
        } else {
            static_assert(traits::always_false_v<Arg>, "Unhandled argument type");
        }
    } catch (parse_exception& e) {
        throw;
    } catch (std::exception& e) {
        throw parse_exception{e.what(), arg_str.src_loc};
    }
}

script::functions::function_variant
create_fn(std::string_view fn_name, const std::vector<argument_string>& fn_args)
{
    // Find the matching function type, then iterate over the args to call the
    // function type's constructor
    auto result = std::optional<script::functions::function_variant>{};
    utility::tuple_type_iterator<function_types>([&](auto, auto fn_ptr) {
        using Fn = std::remove_pointer_t<decltype(fn_ptr)>;

        if (Fn::name() == fn_name) {
            auto tuple_args = typename Fn::args_type{};
            for (auto&& arg : fn_args) {
                auto found = false;
                utility::tuple_type_iterator<typename Fn::args_type>([&](auto j, auto arg_ptr) {
                    using Arg = std::remove_pointer_t<decltype(arg_ptr)>;

                    if (Arg::name() == arg.name) {
                        std::get<j>(tuple_args).value = create_arg_value<Arg>(arg);
                        found = true;
                    }
                });

                if (!found) {
                    throw parse_exception{"Unrecognised argument name: "s + arg.name,
                                          arg.src_loc};
                }
            }

            result = std::make_from_tuple<Fn>(std::move(tuple_args));
        }
    });

    if (!result) {
        // Should never get here as the function names have already been checked
        throw parse_exception{"DEV_ERROR: Unrecognised function name: "s + fn_name};
    }

    return *result;
}
}

script::functions::sequence script::parse(std::istream& stream)
{
    auto fn_seq = script::functions::sequence{};

    auto src_loc = source_location{1, 0};
    for (auto fn_cmd = ""s; std::getline(stream, fn_cmd, ';'); ) {
        if (only_whitespace(fn_cmd)) {
            update_src_loc(fn_cmd, src_loc);
            continue;
        }

        // Extract the function name and check it is one of the known functions
        const auto fn_name = extract_fn_name(fn_cmd, src_loc);
        const auto fn_args = extract_fn_args(fn_cmd, src_loc);
        fn_seq.push_back(create_fn(fn_name, fn_args));
    }

    return fn_seq;
}

script::functions::sequence script::parse(const std::filesystem::path& path)
{
    auto stream = std::ifstream{path};
    if (!stream) {
        throw parse_exception{"Unable to read "s + path.string()};
    }

    return parse(stream);
}
