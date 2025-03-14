// Written by Reito in 2024

#include "super_catch/super_catch.h"

#if defined(SUPER_CATCH_PLAT_POSIX_COMPATIBLE)

#include <cassert>
#include <system_error>
#include <atomic>
#include <csetjmp>
#include <csignal>
#include <atomic>
#include <vector>

namespace {
    struct signal_error_category final : std::error_category {
        [[nodiscard]] const char *name() const noexcept override { return "signal"; }

        [[nodiscard]] std::string message(const int ev) const noexcept override {
#define SIGNAL_CASE(x) case super_catch::error_code_enum:: sig_ ##x: return #x;
            switch (ev) {
                SIGNAL_CASE(abort)
                SIGNAL_CASE(alarm)
                SIGNAL_CASE(arithmetic_exception)
                SIGNAL_CASE(hangup)
                SIGNAL_CASE(illegal)
                SIGNAL_CASE(interrupt)
                SIGNAL_CASE(kill)
                SIGNAL_CASE(pipe)
                SIGNAL_CASE(quit)
                SIGNAL_CASE(segmentation)
                SIGNAL_CASE(terminate)
                SIGNAL_CASE(user1)
                SIGNAL_CASE(user2)
                SIGNAL_CASE(child)
                SIGNAL_CASE(cont)
                SIGNAL_CASE(stop)
                SIGNAL_CASE(terminal_stop)
                SIGNAL_CASE(terminal_in)
                SIGNAL_CASE(terminal_out)
                SIGNAL_CASE(bus)
#ifdef SIGPOLL
                SIGNAL_CASE(poll)
#endif
                SIGNAL_CASE(profiler)
                SIGNAL_CASE(system_call)
                SIGNAL_CASE(trap)
                SIGNAL_CASE(urgent_data)
                SIGNAL_CASE(virtual_timer)
                SIGNAL_CASE(cpu_limit)
                SIGNAL_CASE(file_size_limit)
                default: return "unknown";
            }
#undef SIGNAL_CASE
        }

        [[nodiscard]] std::error_condition default_error_condition(int ev) const noexcept override {
            return {ev, *this};
        }
    };

    thread_local super_catch::detail::sigjmp_buf_chain *cur_buf = nullptr;
    std::once_flag init_signal_handler_once_flag{};
    signal_error_category signal_category{};
} // anonymous namespace

namespace super_catch {
    std::error_code make_error_code(error_code_enum e) {
        return {e, sig_category()};
    }

    std::error_condition make_error_condition(error_code_enum e) {
        return {e, sig_category()};
    }

    std::error_category &sig_category() {
        return signal_category;
    }
}

namespace super_catch {
    namespace detail {
        void handler(int const sig, siginfo_t *, void *) {
            SUPER_CATCH_DEBUG_PRINTF("enter custom signal handler %d\n", sig);

            if (cur_buf) {
                SUPER_CATCH_DEBUG_PRINTF("convert signal to std exception %p\n", cur_buf);
                std::atomic_signal_fence(std::memory_order_acquire);
                siglongjmp(cur_buf->buf, sig);
            }

            // this signal was not caused within the scope of signal handler object,
            // invoke the default handler
            SUPER_CATCH_DEBUG_PRINTF("invoke default signal handler\n");
            signal(sig, SIG_DFL);
            raise(sig);
        }

        void setup_handler() {
            struct sigaction sa{};
            sa.sa_sigaction = &handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_SIGINFO;

            static std::vector<int> signals_to_catch{SIGILL, SIGABRT, SIGFPE, SIGTRAP, SIGSEGV, SIGBUS, SIGPIPE, SIGTERM};
            for (const auto to_catch: signals_to_catch) {
                sigaction(to_catch, &sa, nullptr);
            }
        }

        sigjmp_buf_chain *sigjmp_chain_push() {
            std::call_once(init_signal_handler_once_flag, []() {
                SUPER_CATCH_DEBUG_PRINTF("register global custom signal handler\n");
                setup_handler();
            });

            const auto prev_buf = cur_buf;
            cur_buf = new sigjmp_buf_chain();
            cur_buf->prev = prev_buf;

#if defined(SUPER_CATCH_PARAM_DEBUG_OUTPUT)
            cur_buf->depth = prev_buf == nullptr ? 0 : prev_buf->depth + 1;
#endif

            SUPER_CATCH_DEBUG_PRINTF("push signal handler %d %p to %p\n", cur_buf->depth, prev_buf, cur_buf);
            return cur_buf;
        }

        void sigjmp_chain_pop() {
            const auto this_buf = cur_buf;

#if defined(SUPER_CATCH_PARAM_DEBUG_OUTPUT)
            const auto depth = this_buf->depth;
#endif

            if (cur_buf != nullptr) {
                cur_buf = cur_buf->prev;
                delete this_buf;
            }

            SUPER_CATCH_DEBUG_PRINTF("pop signal handler %d %p to %p\n", depth, this_buf, cur_buf);
        }
    }
}

#endif

#if defined(SUPER_CATCH_PLAT_WIN_MSVC)

#include <windows.h>
#include <eh.h>
#include <Psapi.h>
#include <sstream>
#include <csignal>
#include <csetjmp>

namespace {
    const char *signal_description(const int &signal) {
        switch (signal) {
            case SIGABRT: return "SIGABRT";
            case SIGFPE: return "SIGFPE";
            case SIGSEGV: return "SIGSEGV";
            default: return "SIGUNKNOWN";
        }
    }
}

namespace super_catch {
    namespace detail {
        thread_local jmp_buf_chain *cur_buf = nullptr;

        void signal_handler(const int sig) {
            SUPER_CATCH_DEBUG_PRINTF("signal handler %d\n", sig);

            if (cur_buf) {
                // Although longjmp in signals except SIGFPE is not recommended by Microsoft
                // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/longjmp?view=msvc-170

                if (sig == SIGFPE) {
                    _fpreset();
                }

                SUPER_CATCH_DEBUG_PRINTF("convert signal to std exception %p\n", cur_buf);
                std::atomic_signal_fence(std::memory_order_acquire);
                longjmp(cur_buf->buf, sig);
            }

            // this signal was not caused within the scope of signal handler object,
            // invoke the default handler
            SUPER_CATCH_DEBUG_PRINTF("invoke default signal handler\n");
            signal(sig, SIG_DFL);
            raise(sig);
        }

        jmp_buf_chain *jmp_chain_push() {
            const auto prev_buf = cur_buf;

            cur_buf = new jmp_buf_chain();
            cur_buf->prev = prev_buf;
            cur_buf->sigabrt = signal(SIGABRT, signal_handler);
            cur_buf->sigfpe = signal(SIGFPE, signal_handler);
            cur_buf->sigsegv = signal(SIGSEGV, signal_handler);

#if defined(SUPER_CATCH_PARAM_DEBUG_OUTPUT)
            cur_buf->depth = prev_buf == nullptr ? 0 : prev_buf->depth + 1;
#endif

            SUPER_CATCH_DEBUG_PRINTF("push signal handler %d %p to %p\n", cur_buf->depth, prev_buf, cur_buf);
            return cur_buf;
        }

        void jmp_chain_pop() {
            const auto this_buf = cur_buf;

#if defined(SUPER_CATCH_PARAM_DEBUG_OUTPUT)
            const auto depth = this_buf->depth;
#endif

            if (cur_buf != nullptr) {
                signal(SIGABRT, cur_buf->sigabrt);
                signal(SIGFPE, cur_buf->sigfpe);
                signal(SIGSEGV, cur_buf->sigsegv);

                cur_buf = cur_buf->prev;
                delete this_buf;
            }

            SUPER_CATCH_DEBUG_PRINTF("pop signal handler %d %p to %p\n", depth, this_buf, cur_buf);
        }
    }
}

namespace {
    const char *seh_description(const unsigned int &code) {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
            case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
            case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
            case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
            case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
            case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
            case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
            case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
            case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
            case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
            case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
            case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
            case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
            case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
            case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
            case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
            case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
            default: return "EXCEPTION_UNKNOWN";
        }
    }

    const char *seh_op_description(const ULONG opcode) {
        switch (opcode) {
            case 0: return "read";
            case 1: return "write";
            case 8: return "user-mode data execution prevention (DEP) violation";
            default: return "unknown";
        }
    }
}

super_catch::seh_exception::seh_exception() noexcept: seh_exception{0, nullptr} {
}

super_catch::detail::scoped_seh::scoped_seh() noexcept: old{
    _set_se_translator(
        [](const unsigned int n, EXCEPTION_POINTERS *p) {
            SUPER_CATCH_DEBUG_PRINTF("enter se translator\n");
            throw seh_exception{n, p};
        })
} {
    SUPER_CATCH_DEBUG_PRINTF("set new se translator\n");
}

super_catch::detail::scoped_seh::~scoped_seh() noexcept {
    SUPER_CATCH_DEBUG_PRINTF("restore old se translator\n");
    _set_se_translator(old);
}

super_catch::seh_exception::seh_exception(const unsigned int n, void *rep) noexcept: se_error_{n} {
    const auto ep = static_cast<EXCEPTION_POINTERS *>(rep);

    if (ep != nullptr) {
        HMODULE hm;
        ::GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCTSTR>(ep->ExceptionRecord->ExceptionAddress), &hm);
        MODULEINFO mi;
        ::GetModuleInformation(::GetCurrentProcess(), hm, &mi, sizeof(mi));
        char fn[MAX_PATH];
        ::GetModuleFileNameExA(::GetCurrentProcess(), hm, fn, MAX_PATH);

        std::ostringstream oss;
        oss << "seh exception " << seh_description(n) << " at address 0x" << std::hex << ep->ExceptionRecord->
                ExceptionAddress << std::dec << "\n"
                << "module " << fn << " loaded at base address 0x" << std::hex << mi.lpBaseOfDll;

        if (n == EXCEPTION_ACCESS_VIOLATION || n == EXCEPTION_IN_PAGE_ERROR) {
            oss << "\n" << "invalid operation: " << seh_op_description(ep->ExceptionRecord->ExceptionInformation[0]) <<
                    " at address 0x" << std::hex << ep->ExceptionRecord->ExceptionInformation[1] << std::dec;
        }

        if (n == EXCEPTION_IN_PAGE_ERROR) {
            oss << "\n" << "underlying NTSTATUS code that resulted in the exception " << ep->ExceptionRecord->
                    ExceptionInformation[2];
        }

        msg_ = oss.str();
    } else {
        constexpr int buf_len = 64;
        char buf[buf_len];
        const int len = snprintf(buf, buf_len, "seh exception: %08x", se_error_);
        msg_ = std::string(buf, len);
    }
}

super_catch::win_signal_exception::win_signal_exception(const int sig) noexcept: sig_(sig) {
    constexpr int buf_len = 64;
    char buf[buf_len];
    const int len = snprintf(buf, buf_len, "win signal exception: %d %s", sig, signal_description(sig));
    msg_ = std::string(buf, len);
}

#endif
