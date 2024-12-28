# Super Catch

> Catch (almost) any error (std::exception, signal, structured exception, system error), converting it to `std::exception`. Using original C++ try catch scheme.

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
