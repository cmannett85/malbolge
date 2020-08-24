/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/c_interface.hpp"
#include "malbolge/version.hpp"
#include "malbolge/loader.hpp"

#include <unordered_map>
#include <sstream>

using namespace malbolge;

namespace
{
std::unordered_map<void*, std::stringstream> input_streams;
}

unsigned int malbolge_log_level()
{
    return log::level::ERROR - log::log_level();
}

int malbolge_set_log_level(unsigned int level)
{
    if (level >= log::level::NUM_LOG_LEVELS) {
        log::print(log::level::ERROR, "Log level is too high");
        return -EINVAL;
    }

    log::set_log_level(static_cast<log::level>(log::level::ERROR - level));
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
    if (!buffer) {
        log::print(log::level::ERROR, "NULL program source pointer");
        return nullptr;
    }

    try {
        auto vmem = load(buffer, buffer + size);
        return new virtual_memory(std::move(vmem));
    } catch (parse_exception& e) {
        log::print(log::level::ERROR, e.what());

        if (e.has_location()) {
            if (fail_line) {
                *fail_line = e.location()->line;
            }
            if (fail_column) {
                *fail_column = e.location()->column;
            }
        }
    } catch (std::exception& e) {
        log::print(log::level::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    return nullptr;
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
        log::print(log::level::ERROR, "NULL virtual memory pointer");
        return nullptr;
    }

    auto handle = new virtual_cpu{std::move(*static_cast<virtual_memory*>(vmem))};
    malbolge_free_virtual_memory(vmem);

    auto wrapped_stopped_cb = [=](std::exception_ptr eptr) {
        input_streams.erase(handle);

        auto err = 0;
        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
        } catch (system_exception& e) {
            log::print(log::level::ERROR, e.what());
            err = e.code().value();
        } catch (std::exception& e) {
            log::print(log::level::ERROR, e.what());
            err = -1;
        } catch (...) {
            log::print(log::ERROR, "Unknown exception");
            err = -1;
        }

        stopped_cb(err, handle);
    };

    auto wrapped_waiting_cb = [=]() {
        waiting_cb(handle);
    };

    try {
        if (use_cin) {
            handle->run(std::move(wrapped_stopped_cb),
                        std::move(wrapped_waiting_cb),
                        std::cin);
        } else {
            auto it = input_streams.emplace(handle, std::stringstream{});
            handle->run(std::move(wrapped_stopped_cb),
                        std::move(wrapped_waiting_cb),
                        it.first->second);
        }

        return handle;
    } catch (std::exception& e) {
        log::print(log::level::ERROR, e.what());
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
    }

    input_streams.erase(handle);
    delete handle;
    return nullptr;
}

void malbolge_vcpu_stop(malbolge_virtual_cpu vcpu)
{
    if (!vcpu) {
        log::print(log::level::ERROR, "NULL virtual CPU pointer");
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
        log::print(log::level::ERROR, "NULL virtual CPU pointer");
        return -EINVAL;
    }

    auto it = input_streams.find(vcpu);
    if (it == input_streams.end()) {
        log::print(log::level::ERROR, "vCPU set to use cin");
        return -EINVAL;
    }

    auto& stream = it->second;
    stream.write(buffer, size);

    return 0;
}
