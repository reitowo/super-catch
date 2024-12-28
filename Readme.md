# Super Catch

> Cross-platform catch (almost) any error (std::exception, signal, structured exception, system error), converting it to `std::exception`. Using original C++ try-catch syntax.

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

## Feature

- Supports Windows with MSVC compiler, needs use `/EHa` compiler flag.
- Supports POSIX compatible systems.

## Limitation

- macOS: Unable to catch the exception, when invoking code which address is on non-executable segments.
- Windows: Unsure about the behaviour if the compiler is clang-cl (likely won't work) or compiling in mingw environments with POSIX api.
