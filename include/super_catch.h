// Written by Reito in 2024

#pragma once

#include <system_error>

#ifdef __unix__
#include <unistd.h>
#endif

// Define platform macros
#if defined(_WIN32) || defined(_WIN64)
#define SUPER_CATCH_IS_WIN
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(_POSIX_VERSION)
#define SUPER_CATCH_IS_POSIX
#endif

// Platforms
#if defined(SUPER_CATCH_IS_WIN) && defined(_MSC_VER) && !defined(__clang__)
#define SUPER_CATCH_PLAT_WIN_MSVC
#endif

#if defined(SUPER_CATCH_IS_POSIX)
#define SUPER_CATCH_PLAT_POSIX_COMPATIBLE
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
#if defined(SUPER_CATCH_PLAT_WIN_MSVC)

#include <csignal>
#include <csetjmp>

// Windows(MSVC) version using se_translator
namespace super_catch {
    namespace detail {
        class scoped_seh {
            const _se_translator_function old;

        public:
            scoped_seh() noexcept;

            ~scoped_seh() noexcept;
        };

        struct jmp_buf_chain {
            jmp_buf_chain *prev;
            jmp_buf buf;

            _crt_signal_t sigabrt;
            _crt_signal_t sigfpe;
            _crt_signal_t sigsegv;
        };

        jmp_buf_chain *jmp_chain_push();
        void jmp_chain_pop();
    }

    class seh_exception final : public std::exception {
        friend class detail::scoped_seh;

        const unsigned int se_error_;
        std::string msg_;

        seh_exception() noexcept;

        seh_exception(unsigned int n, void *rep) noexcept;

    public:
        unsigned int se_error() const noexcept { return se_error_; }

        const char *what() const noexcept override {
            return msg_.c_str();
        }
    };

    class win_signal_exception final : public std::exception {
        const int sig_;
        std::string msg_;

    public:
        explicit win_signal_exception(int sig) noexcept;

        int signal() const noexcept { return sig_; }

        const char *what() const noexcept override {
            return msg_.c_str();
        }
    };
}

#define SUPER_CATCH_WIN_PUSH_SIGNAL_HANDLER(ln) \
    super_catch::detail::scoped_seh SUPER_CATCH_CONCATENATE(win_seh_, ln); \
    class SUPER_CATCH_CONCATENATE(win_signal_handler_pop_, ln) { \
    public: \
        ~SUPER_CATCH_CONCATENATE(win_signal_handler_pop_, ln)() { \
            super_catch::detail::jmp_chain_pop(); \
        } \
    } SUPER_CATCH_CONCATENATE(win_signal_handler_pop_inst_, ln); \
    const auto SUPER_CATCH_CONCATENATE(win_cur_buf, ln) = super_catch::detail::jmp_chain_push(); \
    int SUPER_CATCH_CONCATENATE(sig, ln) = setjmp(SUPER_CATCH_CONCATENATE(win_cur_buf, ln)->buf); \
    std::atomic_signal_fence(std::memory_order_release); \
    if (SUPER_CATCH_CONCATENATE(sig, ln) != 0) { \
        SUPER_CATCH_DEBUG_PRINTF("restore from setjmp %p\n", SUPER_CATCH_CONCATENATE(win_cur_buf, ln)); \
        throw super_catch::win_signal_exception(SUPER_CATCH_CONCATENATE(sig, ln)); \
    } \
    SUPER_CATCH_DEBUG_PRINTF("setup setjmp %p\n", SUPER_CATCH_CONCATENATE(win_cur_buf, ln));

#define SUPER_TRY \
    try { \
        SUPER_CATCH_WIN_PUSH_SIGNAL_HANDLER(__COUNTER__) \
        do
#define SUPER_CATCH while(0); } catch

#elif defined(SUPER_CATCH_PLAT_POSIX_COMPATIBLE)

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
    const auto SUPER_CATCH_CONCATENATE(posix_cur_buf, ln) = super_catch::detail::sigjmp_chain_push(); \
    int SUPER_CATCH_CONCATENATE(sig, ln) = sigsetjmp(SUPER_CATCH_CONCATENATE(posix_cur_buf, ln)->buf, 1); \
    std::atomic_signal_fence(std::memory_order_release); \
    if (SUPER_CATCH_CONCATENATE(sig, ln) != 0) { \
        SUPER_CATCH_DEBUG_PRINTF("restore from sigsetjmp %p\n", SUPER_CATCH_CONCATENATE(posix_cur_buf, ln)); \
        throw std::system_error(static_cast<super_catch::error_code_enum>(SUPER_CATCH_CONCATENATE(sig, ln))); \
    } \
    SUPER_CATCH_DEBUG_PRINTF("setup sigsetjmp %p\n", SUPER_CATCH_CONCATENATE(posix_cur_buf, ln));


#define SUPER_TRY \
    try { \
        SUPER_CATCH_POSIX_PUSH_SIGNAL_HANDLER(__COUNTER__); \
        do
#define SUPER_CATCH \
        while(0); \
    } catch

#else

#error "super-catch not supported on this platform. currently supports Windows (MSVC) and POSIX compatible systems.

#endif
