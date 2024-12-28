// Written by Reito in 2024

#include <super_catch.h>
#include <functional>
#include <memory>

#if defined(SUPER_CATCH_IS_POSIX_COMPATIBLE)
#include <sys/mman.h>
#include <unistd.h>
#endif

class TestClass {
    int test_ = 0;

public:
    void TestMethod() {
        test_++;
    }
};

#define SUPER_CATCH_TEST_START(); SUPER_CATCH_DEBUG_PRINTF("test %s start\n", __FUNCTION__);
#define SUPER_CATCH_TEST_END(); SUPER_CATCH_DEBUG_PRINTF("test %s end\n\n", __FUNCTION__);

void TestIllegalInstruction() {
    SUPER_CATCH_TEST_START();

#if defined(SUPER_CATCH_IS_POSIX_COMPATIBLE)
    size_t page_size = sysconf(_SC_PAGESIZE);
    void *mem = mmap(nullptr, page_size, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mem == MAP_FAILED) {
        SUPER_CATCH_DEBUG_PRINTF("mmap failed");
        return;
    }

    SUPER_TRY {
        const auto bad = reinterpret_cast<void (*)()>(mem);
        bad();
    } SUPER_CATCH (const std::exception &e) {
        SUPER_CATCH_DEBUG_PRINTF("super catched exception: %s\n", e.what());
    }

    if (munmap(mem, page_size) != 0) {
        SUPER_CATCH_DEBUG_PRINTF("munmap failed");
    }
#endif

    SUPER_CATCH_TEST_END();
}

void TestExecuteStack() {
    SUPER_CATCH_TEST_START();

    SUPER_TRY {
        char inst[256];
        const auto bad = reinterpret_cast<void (*)()>(reinterpret_cast<void *>(inst));
        bad();
    } SUPER_CATCH (const std::exception &e) {
        SUPER_CATCH_DEBUG_PRINTF("> super catched exception: %s\n", e.what());
    }

    SUPER_CATCH_TEST_END();
}

void TestAbort() {
    SUPER_CATCH_TEST_START();

    SUPER_TRY {
        // Trigger abort
        abort();
    } SUPER_CATCH (const std::exception &e) {
        SUPER_CATCH_DEBUG_PRINTF("> super catched exception: %s\n", e.what());
    }

    SUPER_CATCH_TEST_END();
}

void TestSegFault() {
    SUPER_CATCH_TEST_START();

    SUPER_TRY {
        // Trigger segmentation fault
        std::unique_ptr<TestClass> test;
        test->TestMethod();
    } SUPER_CATCH (const std::exception &e) {
        SUPER_CATCH_DEBUG_PRINTF("> super catched exception: %s\n", e.what());
    }

    SUPER_CATCH_TEST_END();
}

int main() {
    TestIllegalInstruction();
    TestSegFault();
    TestAbort();

#if defined(SUPER_CATCH_IS_WIN_MSVC)
    TestExecuteStack();
#endif

    return 0;
}
