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

- Supports Windows with MSVC compiler (needs `/EHa` compiler flag)
  - Structured Exceptions
  - Signals (SIGABRT, SIGSEGV, SIGFPE)
- Supports POSIX compatible systems
  - Signals (SIGILL, SIGABRT, SIGFPE, SIGTRAP, SIGSEGV, SIGBUS, SIGPIPE, SIGTERM)

## Limitation

- macOS: When invoking code which address is at non-executable segments, the `SIGSEGV` won't be captured by signal handler.
- Windows: Unsure about the behaviour if the compiler is clang-cl (likely won't work) or compiling in mingw environments with POSIX api.
