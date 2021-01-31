/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/script_parser.hpp"
#include "malbolge/utility/from_chars.hpp"
#include "malbolge/utility/string_view_ops.hpp"
#include "malbolge/utility/unescaper.hpp"
#include "malbolge/log.hpp"

#include <boost/algorithm/clamp.hpp>

#include <fstream>
#include <vector>

using namespace malbolge;
using namespace debugger;
using namespace utility::string_view_ops;
using namespace std::string_literals;

namespace
{
using function_types = traits::arg_extractor<script::functions::function_variant>;

struct argument_string
{
    explicit argument_string(std::string_view n, std::size_t index) :
        name{n},
        name_index{index}
    {}

    std::string_view name;
    std::size_t name_index;  // Index into trimmed command
    std::string_view value;
    std::size_t value_index;  // Index into trimmed command
};

class trimmed_command
{
public:
    [[nodiscard]]
    std::string_view string() const
    {
        return s_;
    }

    [[nodiscard]]
    source_location map(std::size_t cmd_index) const
    {
        if (cmd_index > m_.back().index) {
            return m_.back().untrimmed;
        }

        auto it = std::upper_bound(m_.begin(),
                                   m_.end(),
                                   cmd_index,
                                   [](auto i, auto m) { return i < m.index; });

        // Upper bound returns the element greater than cmd_index, so we need to
        // move down one.  There will always be at least one element in m_
        // because the we start the parsing with an UNKNOWN content type
        --it;

        auto src_loc = it->untrimmed;
        src_loc.column += cmd_index - it->index;
        return src_loc;
    }

    [[nodiscard]]
    source_location end_source_location() const
    {
        // There will always be at least one element in m_ because the we start
        // the parsing with an UNKNOWN content type
        return m_.back().untrimmed;
    }

    void trim(std::string_view fn_cmd)
    {
        s_.clear();
        m_.clear();

        // Because clear() doesn't release the capacity, these are only called
        // once
        m_.reserve(10);
        s_.reserve(fn_cmd.size());

        // It is very important that the start indices are zero, rather than the
        // default of 1 - as this instance will be summed wth the script-level
        // instance
        auto src_loc = source_location{0, 0};
        auto type = UNKNOWN;

        auto is_comment = [](auto it, auto end) {
            // Is this and the next character a forward slash
            return (*it == '/') && (it != --end) && (*(++it) == '/');
        };

        auto insert = [this](auto src_loc) {
            const auto index = s_.size();
            if (!m_.empty() && m_.back().index == index) {
                m_.back().untrimmed = src_loc;
            } else {
                m_.emplace_back(index, src_loc);
            }
        };

        for (auto it = fn_cmd.begin(); it != fn_cmd.end(); ++it) {
            if (*it == ' ' || *it == '\t') {
                if (type == STRING) {
                    s_.push_back(*it);
                } else if (type != COMMENT) {
                    type = WHITESPACE;
                }
                ++src_loc.column;
            } else if (*it == '\n') {
                insert(src_loc);

                if (type == STRING) {
                    s_.push_back(*it);
                } else if (type == COMMENT) {
                    type = UNKNOWN;
                }

                ++src_loc.line;
                src_loc.column = 0;
            } else if (*it == '\"') {
                if (type == STRING) {
                    // We to check if this was preceded by an escape character
                    if (*(it-1) != '\\') {
                        type = UNKNOWN;
                    }
                    s_.push_back(*it);
                } else if (type != COMMENT) {
                    type = STRING;
                    s_.push_back(*it);
                }
                ++src_loc.column;
            } else if (is_comment(it, fn_cmd.end())) {
                if (type == STRING) {
                    s_.push_back(*it);
                } else if (type != COMMENT) {
                    type = COMMENT;
                }
                ++src_loc.column;
            } else {
                if (type == STRING) {
                    s_.push_back(*it);
                } else if (type != COMMENT) {
                    if (type != COMMAND) {
                        type = COMMAND;
                        insert(src_loc);
                    }
                    s_.push_back(*it);
                }
                ++src_loc.column;
            }
        }

        if (s_.empty()) [[unlikely]] {
            throw parse_exception{"Empty command", src_loc};
        }

        // Add an end point if there isn't already one
        insert(src_loc);
    }

private:
    enum content_type
    {
        UNKNOWN,
        COMMAND,
        WHITESPACE,
        STRING,
        COMMENT
    };

    struct mapping
    {
        explicit mapping(std::size_t i, source_location u) :
            index{i},
            untrimmed{u}
        {}

        std::size_t index;
        source_location untrimmed;
    };

    std::vector<mapping> m_;
    std::string s_;
};

void update_source_location(source_location& script_src_loc,
                            optional_source_location cmd_src_loc)
{
    if (!cmd_src_loc) {
        return;
    }

    if (cmd_src_loc->line) {
        script_src_loc.line += cmd_src_loc->line;
        script_src_loc.column = cmd_src_loc->column+1;
    } else {
        script_src_loc.column += cmd_src_loc->column;
    }
}

[[nodiscard]]
bool only_whitespace(std::string_view str) noexcept
{
    return std::all_of(str.begin(), str.end(), [](auto c) { return std::isspace(c); });
}

void check_fn_name(std::string_view fn_name, const trimmed_command& trimmed)
{
    auto result = false;
    utility::tuple_type_iterator<function_types>([&](auto, auto ptr) {
        using Fn = std::remove_pointer_t<decltype(ptr)>;

        if (Fn::name() == fn_name) {
            result = true;
        }
    });

    if (!result) [[unlikely]] {
        throw parse_exception{"Unrecognised function name: "s + fn_name,
                              trimmed.map(fn_name.size()-1)};
    }
}

[[nodiscard]]
std::string_view extract_fn_name(const trimmed_command& trimmed)
{
    // Read up to the first bracket
    const auto first_bracket_index = trimmed.string().find_first_of('(');
    if (first_bracket_index == std::string_view::npos) [[unlikely]] {
        throw parse_exception{"No open bracket in function",
                              trimmed.map(first_bracket_index)};
    } else if (first_bracket_index == 0) [[unlikely]] {
        throw parse_exception{"No function name",
                              trimmed.map(first_bracket_index)};
    }

    auto fn_name = trimmed.string().substr(0, first_bracket_index);
    check_fn_name(fn_name, trimmed);

    return fn_name;
}

[[nodiscard]]
std::vector<argument_string> extract_fn_args(std::size_t open_bracket_offset,
                                             const trimmed_command& trimmed)
{
    // Pull just the arg string out of the command string
    const auto cmd = trimmed.string();
    if (cmd.back() != ')') [[unlikely]] {
        throw parse_exception{"No close bracket in function",
                              trimmed.map(cmd.size())};
    }

    ++open_bracket_offset;  // Skip over the bracket to where the args start

    // Exit early if there is nothing to parse
    auto result = std::vector<argument_string>{};
    if ((cmd.size() - 1 - open_bracket_offset) == 0) {
        return result;
    }

    // Iterate through each character, and mark important characters:
    // = is the argument/value assignment
    // , is the argument/value divider
    // \ is the escape character
    // " is the string start/end character
    //
    // This task is complicated by the fact that any of these characters can be
    // appear inside a string value (including the double quotes if preceded by
    // the escape character), but must be ignored
    auto inside_string = false;
    auto escaped = false;
    auto word = std::string_view{cmd.data() + open_bracket_offset, 0};
    for (auto it  = (cmd.begin() + open_bracket_offset); it != cmd.end(); ++it) {
        switch (*it) {
        case '=':
        {
            if (inside_string) {
                break;
            }
            if (word.empty()) [[unlikely]] {
                throw parse_exception{"Missing argument name",
                                      trimmed.map(std::distance(cmd.begin(), it))};
            }

            escaped = false;
            result.emplace_back(word, std::distance(cmd.begin(), it - word.size() - 1));
            word = std::string_view{word.data() + word.size() + 1, 0};
            continue;
        }
        case ',':
        case ')':
        {
            if (inside_string) {
                break;
            }
            if (result.empty() || !result.back().value.empty()) [[unlikely]] {
                throw parse_exception{"Missing argument value",
                                      trimmed.map(std::distance(cmd.begin(), it))};
            }
            escaped = false;

            auto& arg = result.back();
            arg.value = word;
            arg.value_index = std::distance(cmd.begin(), it - word.size() - 1);
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

    if (inside_string) [[unlikely]] {
        throw parse_exception{"Unterminated string", trimmed.map(cmd.size())};
    }

    return result;
}

template <typename Arg>
[[nodiscard]]
auto create_arg_value(const argument_string& arg_str,
                      const trimmed_command& trimmed)
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
                                      trimmed.map(arg_str.value_index)};
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
        throw parse_exception{e.what(), trimmed.map(arg_str.value_index)};
    }
}

[[nodiscard]]
script::functions::function_variant
create_fn(std::string_view fn_name,
          const std::vector<argument_string>& fn_args,
          const trimmed_command& trimmed)
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
                        std::get<j>(tuple_args).value = create_arg_value<Arg>(arg, trimmed);
                        found = true;
                    }
                });

                if (!found) [[unlikely]] {
                    throw parse_exception{"Unrecognised argument name: "s + arg.name,
                                          trimmed.map(arg.name_index)};
                }
            }

            result = std::make_from_tuple<Fn>(std::move(tuple_args));
        }
    });

    if (!result) [[unlikely]] {
        // Should never get here as the function names have already been checked
        throw parse_exception{"DEV_ERROR: Unrecognised function name: "s + fn_name};
    }

    return *result;
}
}

script::functions::sequence script::parse(std::istream& stream)
{
    auto fn_seq = script::functions::sequence{};
    auto src_loc = source_location{1, 1};   // Script level source location

    try {
        // Read in a command's worth of data.  Then strip out the whitespace
        // (not if it is inside string data) and any comments - however this
        // needs to be done in such a way that the source location data is
        // preserved so that any errors can be mapped back to the original
        // script
        auto trimmed = trimmed_command{};
        for (auto fn_cmd = ""s; std::getline(stream, fn_cmd, ';'); ) {
            // This also returns true if fn_cmd is empty
            if (only_whitespace(fn_cmd)) {
                continue;
            }

            trimmed.trim(fn_cmd);

            // Extract the function name and check it is one of the known
            // functions
            const auto fn_name = extract_fn_name(trimmed);
            const auto fn_args = extract_fn_args(fn_name.size(), trimmed);
            fn_seq.push_back(create_fn(fn_name, fn_args, trimmed));

            // Update script-level source location
            update_source_location(src_loc, trimmed.end_source_location());
        }
    } catch (parse_exception& e) {
        // Append the command-level source location with the script-level one
        update_source_location(src_loc, e.location());
        throw parse_exception{e.message(), src_loc};
    } catch (std::exception& e) {
        // If an unexpected error occurs then src_loc may be inaccurate, but it
        // at least gives a location minimum and the command end as a maximum
        throw parse_exception{e.what(), src_loc};
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
