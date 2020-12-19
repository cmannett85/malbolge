/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/c_interface.hpp"
#include "malbolge/version.hpp"
#include "malbolge/loader.hpp"

#ifdef EMSCRIPTEN
#include "malbolge/c_interface_wasm.hpp"
#include <emscripten.h>
#endif

#include <unordered_map>
#include <sstream>

using namespace malbolge;

namespace
{
static_assert(static_cast<int>(MALBOLGE_DBG_REGISTER_MAX) ==
                static_cast<int>(debugger::vcpu_register::NUM_REGISTERS),
              "malbolge::debugger::vcpu_reguster::id and "
                "malbolge_debugger_vcpu_register_id mismatch");

// The stopped callback can be called by the worker thread so we need to
// serialise access to the underlying container - hence this class.  The
// element contents do not need protecting here as the stream and mutex are not
// in use when the stopped callback is fired
class custom_input
{
public:
    template <typename StoppedCb, typename WaitingCb>
    void run(malbolge::virtual_cpu& vcpu,
             StoppedCb&& stopped_cb,
             WaitingCb&& waiting_cb)
    {
        auto lock = std::lock_guard(mtx_);
        auto it = data_.emplace(static_cast<void*>(&vcpu), per_vcpu{}).first;

        vcpu.run(std::forward<StoppedCb>(stopped_cb),
                 std::forward<WaitingCb>(waiting_cb),
                 it->second.stream,
                 std::cout,
                 *(it->second.mtx));
    }

    void erase(malbolge_virtual_cpu vcpu)
    {
        auto lock = std::lock_guard(mtx_);
        data_.erase(vcpu);
    }

    bool write(malbolge_virtual_cpu vcpu, std::string_view buffer)
    {
        auto lock = std::lock_guard(mtx_);
        auto it = data_.find(vcpu);
        if (it == data_.end())  {
            return false;
        }

        auto stream_lock = std::lock_guard{*(it->second.mtx)};
        it->second.stream << buffer;

        return true;
    }

private:
    struct per_vcpu
    {
        per_vcpu() :
            mtx{std::make_unique<std::mutex>()}
        {}

        std::stringstream stream;
        std::unique_ptr<std::mutex> mtx;
    };

    std::mutex mtx_;
    std::unordered_map<void*, per_vcpu> data_;
};

custom_input input_data;

malbolge_virtual_memory
malbolge_load_program_impl(char *buffer,
                           unsigned long size,
                           unsigned int *fail_line,
                           unsigned int *fail_column,
                           bool normalised)
{
    if (!buffer) {
        log::print(log::ERROR, "NULL program source pointer");
        return nullptr;
    }

    try {
        auto vmem = load(buffer, buffer + size, normalised);
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
                                              unsigned int *fail_line,
                                              unsigned int *fail_column)
{
    return malbolge_load_program_impl(buffer,
                                      size,
                                      fail_line,
                                      fail_column,
                                      false);
}

malbolge_virtual_memory
malbolge_load_normalised_program(char *buffer,
                                 unsigned long size,
                                 unsigned int *fail_line,
                                 unsigned int *fail_column)
{
    return malbolge_load_program_impl(buffer,
                                      size,
                                      fail_line,
                                      fail_column,
                                      true);
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

int malbolge_vcpu_run(malbolge_virtual_cpu vcpu,
                      malbolge_program_stopped stopped_cb,
                      malbolge_program_waiting_for_input waiting_cb,
                      int use_cin)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual memory pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto wrapped_stopped_cb = [=](std::exception_ptr eptr) {
        input_data.erase(vcpu);

        auto err = MALBOLGE_ERR_SUCCESS;
        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
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

        stopped_cb(err, vcpu);
    };

    auto wrapped_waiting_cb = [=]() {
        waiting_cb(vcpu);
    };

    auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
    auto err = static_cast<int>(MALBOLGE_ERR_UNKNOWN);
    try {
        if (use_cin) {
            vcpu_ptr->run(std::move(wrapped_stopped_cb),
                          std::move(wrapped_waiting_cb));
        } else {
            input_data.run(*vcpu_ptr,
                           std::move(wrapped_stopped_cb),
                           std::move(wrapped_waiting_cb));
        }

        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        err = MALBOLGE_ERR_EXECUTION_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    input_data.erase(vcpu);
    return err;
}

int malbolge_vcpu_stop(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
    vcpu_ptr->stop();
    return MALBOLGE_ERR_SUCCESS;
}

int malbolge_vcpu_input(malbolge_virtual_cpu vcpu,
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

    auto written = input_data.write(vcpu, {buffer, size});
    if (!written) {
        log::print(log::ERROR, "vCPU set to use cin, or already stopped");
        return MALBOLGE_ERR_CIN_OR_STOPPED;
    }

    return MALBOLGE_ERR_SUCCESS;
}

malbolge_debugger malbolge_debugger_attach(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return nullptr;
    }

    try {
        auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
        return new debugger::client_control{*vcpu_ptr};
    }  catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return nullptr;
}

int malbolge_debugger_pause(malbolge_debugger debugger)
{
    if (!debugger) {
        log::print(log::ERROR, "NULL debugger pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto dbg_ptr = static_cast<debugger::client_control*>(debugger);
        dbg_ptr->pause();
        return MALBOLGE_ERR_SUCCESS;
    }  catch (basic_exception& e) {
        log::print(log::ERROR, e.what());
        return MALBOLGE_ERR_DBG_WRONG_STATE;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_debugger_step(malbolge_debugger debugger)
{
    if (!debugger) {
        log::print(log::ERROR, "NULL debugger pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto dbg_ptr = static_cast<debugger::client_control*>(debugger);
        dbg_ptr->step();
        return MALBOLGE_ERR_SUCCESS;
    }  catch (basic_exception& e) {
        log::print(log::ERROR, e.what());
        return MALBOLGE_ERR_DBG_WRONG_STATE;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_debugger_resume(malbolge_debugger debugger)
{
    if (!debugger) {
        log::print(log::ERROR, "NULL debugger pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto dbg_ptr = static_cast<debugger::client_control*>(debugger);
        dbg_ptr->resume();
        return MALBOLGE_ERR_SUCCESS;
    }  catch (basic_exception& e) {
        log::print(log::ERROR, e.what());
        return MALBOLGE_ERR_DBG_WRONG_STATE;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_debugger_address_value(malbolge_debugger debugger,
                                    unsigned int address)
{
    if (!debugger) {
        log::print(log::ERROR, "NULL debugger pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto dbg_ptr = static_cast<debugger::client_control*>(debugger);
        return static_cast<int>(dbg_ptr->address_value(address));
    }  catch (basic_exception& e) {
        log::print(log::ERROR, e.what());
        return MALBOLGE_ERR_DBG_WRONG_STATE;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_debugger_register_value(malbolge_debugger debugger,
                                     enum malbolge_debugger_vcpu_register_id reg,
                                     unsigned int** address)
{
    if (!debugger) {
        log::print(log::ERROR, "NULL debugger pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    try {
        auto dbg_ptr = static_cast<debugger::client_control*>(debugger);
        const auto data = dbg_ptr->register_value(
            static_cast<debugger::vcpu_register::id>(reg));

        if (address) {
            if (data.address) {
                if (*address) {
                    **address = static_cast<unsigned int>(*data.address);
                } else {
                    log::print(log::ERROR, "NULL dereferenced address pointer");
                    return MALBOLGE_ERR_NULL_ARG;
                }
            } else {
                *address = nullptr;
            }
        }

        return static_cast<unsigned int>(data.value);
    }  catch (basic_exception& e) {
        log::print(log::ERROR, e.what());
        return MALBOLGE_ERR_DBG_WRONG_STATE;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return MALBOLGE_ERR_UNKNOWN;
}

int malbolge_debugger_add_breakpoint(malbolge_debugger debugger,
                                     unsigned int address,
                                     malbolge_debugger_breakpoint_callback cb,
                                     unsigned int ignore_count)
{
    if (!debugger) {
        log::print(log::ERROR, "NULL debugger pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto dbg_ptr = static_cast<debugger::client_control*>(debugger);
    auto wrapped_cb = debugger::client_control::breakpoint::default_callback;
    if (cb) {
        wrapped_cb = [cb](math::ternary a, debugger::vcpu_register::id r) {
            return cb(static_cast<unsigned int>(a),
                      static_cast<malbolge_debugger_vcpu_register_id>(r)) > 0;
        };
    }

    dbg_ptr->add_breakpoint({address, std::move(wrapped_cb), ignore_count});
    return MALBOLGE_ERR_SUCCESS;
}

int malbolge_debugger_remove_breakpoint(malbolge_debugger debugger,
                                        unsigned int address)
{
    if (!debugger) {
        log::print(log::ERROR, "NULL debugger pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto dbg_ptr = static_cast<debugger::client_control*>(debugger);
    return dbg_ptr->remove_breakpoint(address);
}

#ifdef EMSCRIPTEN
EM_JS(void, malbolge_worker_stopped_cb, (int err_code, malbolge_virtual_cpu ptr),
{
    postMessage({
        cmd: "malbolgeStopped",
        errorCode: err_code,
        vcpu: ptr
    });
})

EM_JS(void, malbolge_worker_waiting_cb, (malbolge_virtual_cpu ptr),
{
    postMessage({
        cmd: "malbolgeWaitingForInput",
        vcpu: ptr
    });
})

int malbolge_vcpu_run_wasm(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return MALBOLGE_ERR_NULL_ARG;
    }

    auto wrapped_stopped_cb = [=](std::exception_ptr eptr) {
        input_data.erase(vcpu);

        auto err = MALBOLGE_ERR_SUCCESS;
        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
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

        malbolge_worker_stopped_cb(err, vcpu);
    };

    auto wrapped_waiting_cb = [=]() {
        malbolge_worker_waiting_cb(vcpu);
    };

    auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
    auto err = MALBOLGE_ERR_UNKNOWN;
    try {
        input_data.run(*vcpu_ptr,
                       std::move(wrapped_stopped_cb),
                       std::move(wrapped_waiting_cb));

        return MALBOLGE_ERR_SUCCESS;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        return MALBOLGE_ERR_EXECUTION_FAIL;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    input_data.erase(vcpu);
    return err;
}
#endif
