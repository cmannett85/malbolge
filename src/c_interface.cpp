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
// The stopped callback can be called by the worker thread so we need to
// serialise access to the underlying container - hence this class.  The
// element contents do not protecting here as the stream and mutex are not in
// use when the stopped callback is fired
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
        return -EINVAL;
    }

    log::set_log_level(static_cast<log::level>(log::ERROR - level));
    return 0;
}

const char *malbolge_version()
{
    return version_string;
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

malbolge_virtual_cpu malbolge_vcpu_run(malbolge_virtual_memory vmem,
                                       malbolge_program_stopped stopped_cb,
                                       malbolge_program_waiting_for_input waiting_cb,
                                       int use_cin)
{
    if (!vmem) {
        log::print(log::ERROR, "NULL virtual memory pointer");
        return nullptr;
    }

    auto vcpu = new virtual_cpu{std::move(*static_cast<virtual_memory*>(vmem))};
    malbolge_free_virtual_memory(vmem);

    auto wrapped_stopped_cb = [=](std::exception_ptr eptr) {
        input_data.erase(vcpu);

        auto err = 0;
        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
        } catch (system_exception& e) {
            log::print(log::ERROR, e.what());
            err = e.code().value();
        } catch (std::exception& e) {
            log::print(log::ERROR, e.what());
            err = -1;
        } catch (...) {
            log::print(log::ERROR, "Unknown exception");
            err = -1;
        }

        stopped_cb(err, vcpu);
    };

    auto wrapped_waiting_cb = [=]() {
        waiting_cb(vcpu);
    };

    try {
        if (use_cin) {
            vcpu->run(std::move(wrapped_stopped_cb),
                      std::move(wrapped_waiting_cb));
        } else {
            input_data.run(*vcpu,
                           std::move(wrapped_stopped_cb),
                           std::move(wrapped_waiting_cb));
        }

        return vcpu;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    input_data.erase(vcpu);
    delete vcpu;
    return nullptr;
}

void malbolge_vcpu_stop(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return;
    }

    auto vcpu_ptr = static_cast<virtual_cpu*>(vcpu);
    vcpu_ptr->stop();
}

int malbolge_vcpu_input(malbolge_virtual_cpu vcpu,
                        const char* buffer,
                        unsigned int size)
{
    if (!vcpu) {
        log::print(log::ERROR, "NULL virtual CPU pointer");
        return -EINVAL;
    } 

    auto written = input_data.write(vcpu, {buffer, size});
    if (!written) {
        log::print(log::ERROR, "vCPU set to use cin, or already stopped");
        return -EINVAL;
    }

    return 0;
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

malbolge_virtual_cpu malbolge_vcpu_run_wasm(malbolge_virtual_memory vmem)
{
    if (!vmem) {
        log::print(log::ERROR, "NULL virtual memory pointer");
        return nullptr;
    }

    auto vcpu = new virtual_cpu{std::move(*static_cast<virtual_memory*>(vmem))};
    malbolge_free_virtual_memory(vmem);

    auto wrapped_stopped_cb = [=](std::exception_ptr eptr) {
        input_data.erase(vcpu);

        auto err = 0;
        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
        } catch (system_exception& e) {
            log::print(log::ERROR, e.what());
            err = e.code().value();
        } catch (std::exception& e) {
            log::print(log::ERROR, e.what());
            err = -1;
        } catch (...) {
            log::print(log::ERROR, "Unknown exception");
            err = -1;
        }

        malbolge_worker_stopped_cb(err, vcpu);
    };

    auto wrapped_waiting_cb = [=]() {
        malbolge_worker_waiting_cb(vcpu);
    };

    try {
        input_data.run(*vcpu,
                       std::move(wrapped_stopped_cb),
                       std::move(wrapped_waiting_cb));

        return vcpu;
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    input_data.erase(vcpu);
    delete vcpu;
    return nullptr;
}
#endif
