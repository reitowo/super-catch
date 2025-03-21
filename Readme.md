# Super Catch

Cross-platform catch (almost) any error (signal, structured exception, system error), converting it to `std::exception`. Using original C++ try-catch syntax.

## Usage

```c++
#include <super_catch.h>

int main() {
    SUPER_TRY {
        // Trigger segmentation fault
        std::unique_ptr<TestClass> test;
        test->TestMethod();
    } SUPER_CATCH (const std::exception &e) {
        fprintf(stderr, "super catched exception: %s\n", e.what());
    }
}
```

See `test/main.cpp` for detail.

## Feature

- Original C++ try-catch syntax. (`SUPER_TRY`, `SUPER_CATCH`)
- Convert all types of system errors (`SEH`, `signal`) to `std::exception`
- Supports Windows with MSVC compiler (needs `/EHa` compiler flag)
  - Structured Exceptions
  - Signals (`SIGABRT`, `SIGSEGV`, `SIGFPE`)
- Supports POSIX compatible systems
  - Signals (`SIGILL`, `SIGABRT`, `SIGFPE`, `SIGTRAP`, `SIGSEGV`, `SIGBUS`, `SIGPIPE`, `SIGTERM`)

## Limitation

- macOS: When invoking code which address is at non-executable segments, the `SIGSEGV` won't be captured by signal handler.
- Windows: Unsure about the behaviour if the compiler is clang-cl (likely won't work) or compiling in mingw environments with POSIX api.
- The recover code won't release locks, use with cautious when the code contains lock. 
- The recover code won't destruct C++ objects like std::unique_ptr, use with cautions if the code allocates memory.
