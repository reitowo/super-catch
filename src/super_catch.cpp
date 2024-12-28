// Written by Reito in 2024

#include "include/super_catch.h"

#if !defined(IS_WIN)

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

            const auto this_buf = cur_buf;
            cur_buf = new sigjmp_buf_chain();
            cur_buf->prev = this_buf;

            SUPER_CATCH_DEBUG_PRINTF("push signal handler %p to %p\n", this_buf, cur_buf);
            return cur_buf;
        }

        void sigjmp_chain_pop() {
            const auto this_buf = cur_buf;
            if (cur_buf != nullptr) {
                cur_buf = cur_buf->prev;
                delete this_buf;
            }

            SUPER_CATCH_DEBUG_PRINTF("pop signal handler %p to %p\n", this_buf, cur_buf);
        }
    }
}

#endif
