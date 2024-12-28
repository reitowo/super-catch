// Written by Reito in 2024

#pragma once

#include <system_error>

// Define platform macros
#if defined(_WIN32) || defined(_WIN64)
#define SUPER_CATCH_IS_WIN
#endif

#if defined(SUPER_CATCH_IS_WIN) && defined(_MSC_VER) && !defined(__clang__)
#define SUPER_CATCH_IS_WIN_MSVC
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(_POSIX_VERSION)
#define SUPER_CATCH_IS_POSIX_COMPATIBLE
#endif

// Debug
#if defined(SUPER_CATCH_PARAM_DEBUG_OUTPUT)
#define SUPER_CATCH_DEBUG_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
#define SUPER_CATCH_DEBUG_PRINTF(...) (void)0
#endif

// Helper
#define SUPER_CATCH_CONCATENATE_DETAIL(x, y) x##y
#define SUPER_CATCH_CONCATENATE(x, y) SUPER_CATCH_CONCATENATE_DETAIL(x, y)

// Platform specific code
#if defined(SUPER_CATCH_IS_WIN_MSVC)

// Windows(MSVC) version using se_translator
namespace super_catch::detail {
    class seh_exception : public std::exception {
        const unsigned int se_error_;
        std::string msg_;

    public:
        seh_exception() noexcept : seh_exception{0} {
        }

        explicit seh_exception(const unsigned int n) noexcept : se_error_{n} {
            constexpr int buf_len = 64;
            char buf[buf_len];
            const int len = snprintf(buf, buf_len, "seh exception: %08x", se_error_);
            msg_ = std::string(buf, len);
        }

        unsigned int se_error() const noexcept { return se_error_; }

        const char *what() const noexcept override {
            return msg_.c_str();
        }
    };

    class scoped_seh {
        const _se_translator_function old;

    public:
        scoped_seh() noexcept
            : old{
                _set_se_translator([](unsigned int n, EXCEPTION_POINTERS *p) {
                    throw seh_exception{n};
                })
            } {
        }

        ~scoped_seh() noexcept { _set_se_translator(old); }
    };
}

#define SUPER_TRY scoped_seh seh; try {
#define SUPER_CATCH } catch

#elif defined(SUPER_CATCH_IS_POSIX_COMPATIBLE)

// Posix version using signal handler
#include <csetjmp>

namespace super_catch {
#define SIG_ENUM(name, sig) sig_ ##name = sig,

    enum error_code_enum: int {
        SIG_ENUM(abort, SIGABRT)
        SIG_ENUM(alarm, SIGALRM)
        SIG_ENUM(arithmetic_exception, SIGFPE)
        SIG_ENUM(hangup, SIGHUP)
        SIG_ENUM(illegal, SIGILL)
        SIG_ENUM(interrupt, SIGINT)
        SIG_ENUM(kill, SIGKILL)
        SIG_ENUM(pipe, SIGPIPE)
        SIG_ENUM(quit, SIGQUIT)
        SIG_ENUM(segmentation, SIGSEGV)
        SIG_ENUM(terminate, SIGTERM)
        SIG_ENUM(user1, SIGUSR1)
        SIG_ENUM(user2, SIGUSR2)
        SIG_ENUM(child, SIGCHLD)
        SIG_ENUM(cont, SIGCONT)
        SIG_ENUM(stop, SIGSTOP)
        SIG_ENUM(terminal_stop, SIGTSTP)
        SIG_ENUM(terminal_in, SIGTTIN)
        SIG_ENUM(terminal_out, SIGTTOU)
        SIG_ENUM(bus, SIGBUS)
#ifdef SIGPOLL
        SIG_ENUM(poll, SIGPOLL)
#endif
        SIG_ENUM(profiler, SIGPROF)
        SIG_ENUM(system_call, SIGSYS)
        SIG_ENUM(trap, SIGTRAP)
        SIG_ENUM(urgent_data, SIGURG)
        SIG_ENUM(virtual_timer, SIGVTALRM)
        SIG_ENUM(cpu_limit, SIGXCPU)
        SIG_ENUM(file_size_limit, SIGXFSZ)
    };

#undef SIG_ENUM

    std::error_code make_error_code(error_code_enum e);

    std::error_condition make_error_condition(error_code_enum e);

    std::error_category &sig_category();
}

namespace std {
    template<>
    struct is_error_code_enum<super_catch::error_code_enum> : std::true_type {
    };

    template<>
    struct is_error_condition_enum<super_catch::error_code_enum> : std::true_type {
    };
} // namespace std

namespace super_catch {
    namespace detail {
        struct sigjmp_buf_chain {
            sigjmp_buf_chain *prev = nullptr;
            sigjmp_buf buf{};
        };

        sigjmp_buf_chain *sigjmp_chain_push();

        void sigjmp_chain_pop();
    }
}

#define SUPER_CATCH_POSIX_PUSH_SIGNAL_HANDLER(ln) \
    class SUPER_CATCH_CONCATENATE(posix_signal_handler_pop_, ln) { \
    public: \
        ~SUPER_CATCH_CONCATENATE(posix_signal_handler_pop_, ln)() { \
            super_catch::detail::sigjmp_chain_pop(); \
        } \
    } SUPER_CATCH_CONCATENATE(posix_signal_handler_pop_inst_, ln); \
    const auto SUPER_CATCH_CONCATENATE(cur_bug, ln) = super_catch::detail::sigjmp_chain_push(); \
    int SUPER_CATCH_CONCATENATE(sig, ln) = sigsetjmp(SUPER_CATCH_CONCATENATE(cur_bug, ln)->buf, 1); \
    std::atomic_signal_fence(std::memory_order_release); \
    if (SUPER_CATCH_CONCATENATE(sig, ln) != 0) { \
        SUPER_CATCH_DEBUG_PRINTF("restore from sigsetjmp %p\n", SUPER_CATCH_CONCATENATE(cur_bug, ln)); \
        throw std::system_error(static_cast<super_catch::error_code_enum>(SUPER_CATCH_CONCATENATE(sig, ln))); \
    } \
    SUPER_CATCH_DEBUG_PRINTF("setup sigsetjmp %p\n", SUPER_CATCH_CONCATENATE(cur_bug, ln));


#define SUPER_TRY \
    try { \
        SUPER_CATCH_POSIX_PUSH_SIGNAL_HANDLER(__COUNTER__); \
        do
#define SUPER_CATCH \
        while(0); \
    } catch

#else

#error "super-catch not supported on this platform. currently supports Windows(MSVC) and POSIX compliant systems.

#endif
