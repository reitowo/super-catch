# Super Try Catch

> Catch (almost) any std exception, signals, system errors. Using original try catch scheme.

## Usage

```c++
#include <stdio.h>
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