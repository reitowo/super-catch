// Written by Reito in 2024

#include "super_catch/super_catch.h"
#include <functional>
#include <memory>

#if defined(SUPER_CATCH_PLAT_WIN_MSVC)
#include <windows.h>
#endif

#if defined(SUPER_CATCH_PLAT_POSIX_COMPATIBLE)
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

#define SUPER_CATCH_TEST_PRINTF(...) fprintf(stderr, __VA_ARGS__);

#define SUPER_CATCH_TEST_START(); SUPER_CATCH_TEST_PRINTF("++ test %s start\n", __FUNCTION__);
#define SUPER_CATCH_TEST_END(); SUPER_CATCH_TEST_PRINTF("-- test %s end\n\n", __FUNCTION__);

void TestIllegalInstruction() {
    SUPER_CATCH_TEST_START();

#if defined(SUPER_CATCH_PLAT_POSIX_COMPATIBLE)
    size_t page_size = sysconf(_SC_PAGESIZE);
    void *mem = mmap(nullptr, page_size, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mem == MAP_FAILED) {
        SUPER_CATCH_TEST_PRINTF("mmap failed");
        return;
    }

    SUPER_TRY {
        const auto bad = reinterpret_cast<void (*)()>(mem);
        bad();
    } SUPER_CATCH (const std::exception &e) {
        SUPER_CATCH_TEST_PRINTF(">> super catched exception: %s\n", e.what());
    }

    if (munmap(mem, page_size) != 0) {
        SUPER_CATCH_TEST_PRINTF("munmap failed");
    }
#endif

#if defined(SUPER_CATCH_PLAT_WIN_MSVC)
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    size_t page_size = system_info.dwPageSize;

    // Allocate executable memory
    void *mem = VirtualAlloc(nullptr, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READ);
    if (!mem) {
        SUPER_CATCH_TEST_PRINTF("VirtualAlloc failed\n");
        return;
    }

    SUPER_TRY {
        // Cast memory to a function pointer and execute
        const auto bad = reinterpret_cast<void (*)()>(mem);
        bad(); // This should trigger an access violation
    } SUPER_CATCH (const std::exception &e) {
        SUPER_CATCH_TEST_PRINTF(">> super catched exception: %s\n", e.what());
    }

    // Free the allocated memory
    if (!VirtualFree(mem, 0, MEM_RELEASE)) {
        SUPER_CATCH_TEST_PRINTF("VirtualFree failed\n");
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
        SUPER_CATCH_TEST_PRINTF(">> super catched exception: %s\n", e.what());
    }

    SUPER_CATCH_TEST_END();
}

void TestAbort() {
    SUPER_CATCH_TEST_START();

    SUPER_TRY {
        // Trigger abort
        abort();
    } SUPER_CATCH (const std::exception &e) {
        SUPER_CATCH_TEST_PRINTF(">> super catched exception: %s\n", e.what());
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
        SUPER_CATCH_TEST_PRINTF(">> super catched exception: %s\n", e.what());
    }

    SUPER_CATCH_TEST_END();
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    TestIllegalInstruction();
    TestSegFault();
    TestAbort();

#if defined(SUPER_CATCH_PLAT_WIN_MSVC)
    TestExecuteStack();
#endif

    return 0;
}
