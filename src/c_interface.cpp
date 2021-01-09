/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/c_interface.hpp"
#include "malbolge/version.hpp"
#include "malbolge/loader.hpp"
#include "malbolge/virtual_cpu.hpp"

#ifdef EMSCRIPTEN
#include "malbolge/c_interface_wasm.hpp"
#include <emscripten.h>
#endif

#include <unordered_map>
#include <sstream>

using namespace malbolge;
using namespace std::string_literals;

namespace
{
static_assert(static_cast<int>(MALBOLGE_VCPU_NUM_STATES) ==
                static_cast<int>(virtual_cpu::execution_state::NUM_STATES),
              "malbolge_vcpu_execution_state and virtual_cpu::execution_state mismatch");
static_assert(static_cast<int>(MALBOLGE_VCPU_REGISTER_MAX) ==
                static_cast<int>(virtual_cpu::vcpu_register::NUM_REGISTERS),
              "malbolge_vcpu_register and virtual_cpu::vcpu_register mismatch");

class vcpu_signal_manager
{
public:
    using callback_address = void*;

    enum class signal_type {
        STATE,
        OUTPUT,
        BREAKPOINT,
        NUM_TYPES
    };

    void connect(malbolge_virtual_cpu vcpu,
                 signal_type signal,
                 callback_address cb_address)
    {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);

        switch (signal) {
        case signal_type::STATE:
        {
            auto state_cb = [vcpu, cb_address](auto state, auto eptr) {
                auto cb = reinterpret_cast<malbolge_vcpu_state_callback>(cb_address);
                const auto c_state = static_cast<malbolge_vcpu_execution_state>(state);
                auto err = MALBOLGE_ERR_SUCCESS;

                if (eptr) {
                    try {
                        std::rethrow_exception(eptr);
                    } catch (system_exception& e) {
                        log::print(log::ERROR, e.what());
                        err = static_cast<malbolge_result>(e.code().value());
                    } catch (std::exception& e) {
                        log::print(log::ERROR, e.what());
                        err = MALBOLGE_ERR_EXECUTION_FAIL;
                    } catch (...) {
                        log::print(log::ERROR, "Unknown exception");
                        err = MALBOLGE_ERR_UNKNOWN;
                    }
                }

                cb(vcpu, c_state, err);
            };

            auto& conn = state_[vcpu][cb_address];
            conn = vcpu_ptr->register_for_state_signal(std::move(state_cb));
            break;
        }
        case signal_type::OUTPUT:
        {
            auto output_cb = [vcpu, cb_address](auto c) {
                auto cb = reinterpret_cast<malbolge_vcpu_output_callback>(cb_address);
                cb(vcpu, c);
            };
            auto& conn = output_[vcpu][cb_address];
            conn = vcpu_ptr->register_for_output_signal(std::move(output_cb));
            break;
        }
        case signal_type::BREAKPOINT:
        {
            auto bp_cb = [vcpu, cb_address](auto address) {
                auto cb = reinterpret_cast<malbolge_vcpu_breakpoint_hit_callback>(cb_address);
                cb(vcpu, static_cast<unsigned int>(address));
            };
            auto& conn = bp_[vcpu][cb_address];
            conn = vcpu_ptr->register_for_breakpoint_hit_signal(std::move(bp_cb));
            break;
        }
        default:
            // Will never get here due to the strong enum typing and the
            // static_assert check below
            break;
        }
    }

    void disconnect(malbolge_virtual_cpu vcpu,
                    signal_type signal,
                    callback_address cb_address)
    {
        auto simple_remover = [&](auto& map) {
            auto vcpu_it = map.find(vcpu);
            if (vcpu_it == map.end()) {
                return;
            }

            auto it = vcpu_it->second.find(cb_address);
            if (it == vcpu_it->second.end()) {
                return;
            }

            it->second.disconnect();
            vcpu_it->second.erase(it);

            if (vcpu_it->second.empty()) {
                map.erase(vcpu_it);
            }
        };

        switch (signal) {
        case signal_type::STATE:
            simple_remover(state_);
            break;
        case signal_type::OUTPUT:
            simple_remover(output_);
            break;
        case signal_type::BREAKPOINT:
            simple_remover(bp_);
            break;
        default:
            // Will never get here due to the strong enum typing and the
            // static_assert check below
            break;
        }
    }

private:
    static_assert(static_cast<int>(signal_type::NUM_TYPES) == 3,
                  "Number of C API signal types has changed");

    template <typename Connection>
    using address_map = std::unordered_map<malbolge_virtual_cpu,
                                           std::unordered_map<callback_address,
                                                              Connection>>;

    address_map<virtual_cpu::state_signal_type::connection> state_;
    address_map<virtual_cpu::output_signal_type::connection> output_;
    address_map<virtual_cpu::breakpoint_hit_signal_type::connection> bp_;
};

vcpu_signal_manager signal_manager_;
}

unsigned int malbolge_log_level()
{
    return log::ERROR - log::log_level();
}

int malbolge_set_log_level(unsigned int level)
{
    if (level >= log::NUM_LOG_LEVELS) {
        log::print(log::ERROR, "Log level is too high");
        return MALBOLGE_ERR_INVALID_LOG_LEVEL;
    }

    log::set_log_level(static_cast<log::level>(log::NUM_LOG_LEVELS - 1 - level));
    return MALBOLGE_ERR_SUCCESS;
}

const char *malbolge_version()
{
    return version_string;
}

int malbolge_is_likely_normalised_source(const char *buffer,
                                         unsigned long size)
{
    if (!buffer) {
        log::print(log::ERROR, "NULL program source pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    return is_likely_normalised_source(buffer, buffer + size);
}

int malbolge_normalise_source(char *buffer,
                              unsigned long size,
                              unsigned long *new_size,
                              unsigned int *fail_line,
                              unsigned int *fail_column)
{
    if (!buffer) {
        log::print(log::ERROR, "NULL program source pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }
    if (!new_size) {
        log::print(log::ERROR, "NULL normalised program size pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto it = normalise_source(buffer, buffer + size);
        *new_size = std::distance(buffer, it);

        if (*new_size < size) {
            buffer[*new_size] = '\n';
        }

        return MALBOLGE_ERR_SUCCESS;
    } catch (parse_exception& e) {
        log::print(log::ERROR, e.what());

        if (e.has_location()) {
            if (fail_line) {
                *fail_line = e.location()->line;
            }
            if (fail_column) {
                *fail_column = e.location()->column;
            }
        }
        return MALBOLGE_ERR_PARSE_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_denormalise_source(char *buffer,
                                unsigned long size,
                                unsigned int *fail_column)
{
    if (!buffer) {
        log::print(log::ERROR, "NULL program source pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        denormalise_source(buffer, buffer + size);
        return MALBOLGE_ERR_SUCCESS;
    } catch (parse_exception& e) {
        log::print(log::ERROR, e.what());

        if (e.has_location()) {
            if (fail_column) {
                *fail_column = e.location()->column;
            }
        }
        return MALBOLGE_ERR_PARSE_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

malbolge_virtual_memory malbolge_load_program(char *buffer,
                                              unsigned long size,
                                              malbolge_load_normalised_mode mode,
                                              unsigned int *fail_line,
                                              unsigned int *fail_column)
{
    if (!buffer) {
        log::print(log::ERROR, "NULL program source pointer");
        return nullptr;
    }

    try {
        auto vmem = load(buffer,
                         buffer + size,
                         static_cast<load_normalised_mode>(mode));
        return new virtual_memory(std::move(vmem));
    } catch (parse_exception& e) {
        log::print(log::ERROR, e.what());

        if (e.has_location()) {
            if (fail_line) {
                *fail_line = e.location()->line;
            }
            if (fail_column) {
                *fail_column = e.location()->column;
            }
        }
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return nullptr;
}

void malbolge_free_virtual_memory(malbolge_virtual_memory vmem)
{
    delete static_cast<virtual_memory*>(vmem);
}

malbolge_virtual_cpu malbolge_create_vcpu(malbolge_virtual_memory vmem)
{
    if (!vmem) {
        log::print(log::ERROR, "NULL virtual memory pointer");
        return nullptr;
    }

    auto vcpu = new virtual_cpu{std::move(*static_cast<virtual_memory*>(vmem))};
    malbolge_free_virtual_memory(vmem);

    return vcpu;
}

void malbolge_free_vcpu(malbolge_virtual_cpu vcpu)
{
    delete static_cast<virtual_cpu*>(vcpu);
}

int malbolge_vcpu_attach_callbacks(malbolge_virtual_cpu vcpu,
                                   malbolge_vcpu_state_callback state_cb,
                                   malbolge_vcpu_output_callback output_cb,
                                   malbolge_vcpu_breakpoint_hit_callback bp_cb)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    if (state_cb) {
        signal_manager_.connect(vcpu,
                                vcpu_signal_manager::signal_type::STATE,
                                reinterpret_cast<void*>(state_cb));
    }
    if (output_cb) {
        signal_manager_.connect(vcpu,
                                vcpu_signal_manager::signal_type::OUTPUT,
                                reinterpret_cast<void*>(output_cb));
    }
    if (bp_cb) {
        signal_manager_.connect(vcpu,
                                vcpu_signal_manager::signal_type::BREAKPOINT,
                                reinterpret_cast<void*>(bp_cb));
    }

    return MALBOLGE_ERR_SUCCESS;
}

int malbolge_vcpu_detach_callbacks(malbolge_virtual_cpu vcpu,
                                   malbolge_vcpu_state_callback state_cb,
                                   malbolge_vcpu_output_callback output_cb,
                                   malbolge_vcpu_breakpoint_hit_callback bp_cb)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    if (state_cb) {
        signal_manager_.disconnect(vcpu,
                                   vcpu_signal_manager::signal_type::STATE,
                                   reinterpret_cast<void*>(state_cb));
    }
    if (output_cb) {
        signal_manager_.disconnect(vcpu,
                                   vcpu_signal_manager::signal_type::OUTPUT,
                                   reinterpret_cast<void*>(output_cb));
    }
    if (bp_cb) {
        signal_manager_.disconnect(vcpu,
                                   vcpu_signal_manager::signal_type::BREAKPOINT,
                                   reinterpret_cast<void*>(bp_cb));
    }

    return MALBOLGE_ERR_SUCCESS;
}

int malbolge_vcpu_run(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }


    auto err = static_cast<int>(MALBOLGE_ERR_UNKNOWN);
    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
        vcpu_ptr->run();
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        err = MALBOLGE_ERR_EXECUTION_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return err;
}

int malbolge_vcpu_pause(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto err = static_cast<int>(MALBOLGE_ERR_UNKNOWN);
    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
        vcpu_ptr->pause();
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        err = MALBOLGE_ERR_EXECUTION_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return err;
}

int malbolge_vcpu_step(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto err = static_cast<int>(MALBOLGE_ERR_UNKNOWN);
    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
        vcpu_ptr->step();
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        err = MALBOLGE_ERR_EXECUTION_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return err;
}

int malbolge_vcpu_add_input(malbolge_virtual_cpu vcpu,
                            const char* buffer,
                            unsigned int size)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    if (!buffer) {
        log::print(log::ERROR, "NULL buffer pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
        vcpu_ptr->add_input({buffer, size});
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_vcpu_add_breakpoint(malbolge_virtual_cpu vcpu,
                                 unsigned int address,
                                 unsigned int ignore_count)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
        vcpu_ptr->add_breakpoint(address, ignore_count);
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_vcpu_remove_breakpoint(malbolge_virtual_cpu vcpu,
                                    unsigned int address)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
        vcpu_ptr->remove_breakpoint(address);
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_vcpu_address_value(malbolge_virtual_cpu vcpu,
                                unsigned int address,
                                malbolge_vcpu_address_value_callback cb)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }
    if (!cb) {
        log::print(log::ERROR, "NULL callback");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);

        auto wrapped_cb = [cb, vcpu](auto address, auto value) {
            cb(vcpu,
               static_cast<unsigned int>(address),
               static_cast<unsigned int>(value));
        };
        vcpu_ptr->address_value(address, std::move(wrapped_cb));
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_vcpu_register_value(malbolge_virtual_cpu vcpu,
                                 enum malbolge_vcpu_register reg,
                                 malbolge_vcpu_register_value_callback cb)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }
    if (!cb) {
        log::print(log::ERROR, "NULL callback");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);

        auto wrapped_cb = [cb, vcpu](auto reg, auto address, auto value) {
            cb(vcpu,
               static_cast<malbolge_vcpu_register>(reg),
               address ? static_cast<unsigned int>(*address) : 0,
               static_cast<unsigned int>(value));
        };
        vcpu_ptr->register_value(static_cast<virtual_cpu::vcpu_register>(reg),
                                 std::move(wrapped_cb));
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

#ifdef EMSCRIPTEN
EM_JS(void, malbolge_state_cb, (int state, int err_code, malbolge_virtual_cpu ptr),
{
    postMessage({
        cmd: "malbolgevCPUState",
        state: state,
        errorCode: err_code,
        vcpu: ptr
    });
})

EM_JS(void, malbolge_breakpoint_cb, (int address, malbolge_virtual_cpu ptr),
{
    postMessage({
        cmd: "malbolgeBreakpoint",
        address: address,
        vcpu: ptr
    });
})

EM_JS(void, malbolge_output_cb, (const char* text, malbolge_virtual_cpu ptr),
{
    postMessage({
        cmd: "malbolgeOutput",
        data: UTF8ToString(text),
        vcpu: ptr
    });
})

int malbolge_vcpu_run_wasm(malbolge_virtual_cpu vcpu)
{
    constexpr static auto max_buf_size = std::size_t{10};

    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);

    // As the signals are called from the same thread, we do not need any
    // locking around shared state
    auto output_buf = std::make_shared<std::string>();
    output_buf->reserve(max_buf_size);

    vcpu_ptr->register_for_output_signal([vcpu, buf = output_buf](auto c) {
        buf->push_back(c);
        if (buf->size() >= max_buf_size) {
            malbolge_output_cb(buf->data(), vcpu);
            buf->clear();
        }
    });
    vcpu_ptr->register_for_state_signal([vcpu, buf = std::move(output_buf)]
                                        (auto state, auto eptr) {
        // Flush the output buffer on a vCPU state change
        if (!buf->empty()) {
            malbolge_output_cb(buf->data(), vcpu);
            buf->clear();
        }

        const auto c_state = static_cast<malbolge_vcpu_execution_state>(state);
        auto err = MALBOLGE_ERR_SUCCESS;

        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (system_exception& e) {
                log::print(log::ERROR, e.what());
                err = static_cast<malbolge_result>(e.code().value());
            } catch (std::exception& e) {
                log::print(log::ERROR, e.what());
                err = MALBOLGE_ERR_EXECUTION_FAIL;
            } catch (...) {
                log::print(log::ERROR, "Unknown exception");
                err = MALBOLGE_ERR_UNKNOWN;
            }
        }

        malbolge_state_cb(c_state, err, vcpu);
    });
    vcpu_ptr->register_for_breakpoint_hit_signal([vcpu](auto address) {
        malbolge_breakpoint_cb(static_cast<int>(address), vcpu);
    });

    auto err = static_cast<int>(MALBOLGE_ERR_UNKNOWN);
    try {
        vcpu_ptr->run();
        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        err = MALBOLGE_ERR_EXECUTION_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return err;
}
#endif
