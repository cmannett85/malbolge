/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/get_char.hpp"

#ifdef EMSCRIPTEN
#include <emscripten.h>

#include <boost/core/ignore_unused.hpp>

#include <map>
#include <array>
#endif

using namespace malbolge;

#ifdef EMSCRIPTEN
namespace
{
struct em_istream {
    em_istream() :
        pos{0},
        len{0}
    {}

    std::size_t pos;
    std::size_t len;
    std::array<char, 4096> buffer;
};

std::map<std::istream*, em_istream> streams;
}

// From https://github.com/curiousdannii/emglken/blob/master/src/getc.c
EM_JS(int, malbolge_input, (char* buffer, int maxlen), {
    return Asyncify.handleAsync(async () => {
        // Wrap the worker's onmessage function so we can catch our messages.
        // This IS a hack, but Emscripten doesn't allow us to add our own
        // custom message handler for pthread emulation
        if (typeof Module.malbolgeConfigured == "undefined") {
            Module.malbolgeConfigured = true;
            Module.malbolgeInputQueue = [];
            Module.malbolgeInputNotify = function() {};

            let standardHandlers = self.onmessage;
            self.onmessage = function(e) {
                if (e.data.cmd === "input_text") {
                    const encoder = new TextEncoder();
                    const u8Text = encoder.encode(e.data.input_text);
                    Module.malbolgeInputQueue.push(u8Text);
                    Module.malbolgeInputNotify();
                } else {
                    standardHandlers(e);
                }
            };
        }

        // If there is no input waiting to be processed, block until notified
        // that there is
        if (!Module.malbolgeInputQueue.length) {
            await new Promise(resolve => {
                Module.malbolgeInputNotify = resolve;
            });
        }

        // Extract the first buffer's contents (limited to the max length), and
        // pass to the input buffer
        const input = Module.malbolgeInputQueue[0];
        const len = Math.min(input.length, maxlen);
        HEAPU8.set(input.subarray(0, len), buffer);

        if (len == input.length) {
            Module.malbolgeInputQueue.shift();
        } else {
            Module.malbolgeInputQueue[0] = input.subarray(len);
        }

        return len;
    });
});
#endif

utility::get_char_result utility::get_char(std::istream& str,
                                           char& c,
                                           bool block)
{
#ifdef EMSCRIPTEN
    boost::ignore_unused(block);

    auto& [pos, len, buffer] = streams[&str];

    if (pos == len) {
        len = malbolge_input(buffer.data(), buffer.size());
        pos = 0;
    }

    c = buffer[pos++];
    return utility::get_char_result::CHAR;
#else
    if (!block && !str.rdbuf()->in_avail()) {
        return utility::get_char_result::NO_DATA;
    }

    return str.get(c) ? utility::get_char_result::CHAR :
                        utility::get_char_result::ERROR;
#endif
}
