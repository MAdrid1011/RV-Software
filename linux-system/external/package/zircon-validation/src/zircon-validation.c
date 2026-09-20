#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#define HWCAP_C (1UL << ('C' - 'A'))
#define HWCAP_D (1UL << ('D' - 'A'))
#define HWCAP_F (1UL << ('F' - 'A'))

struct riscv_f_state {
    uint32_t f[32];
    uint32_t fcsr;
};

static volatile sig_atomic_t signal_seen;

static void fail(const char *check) {
    fprintf(stderr, "Linux validation failure: %s (%s)\n", check, strerror(errno));
    exit(1);
}

static uint32_t float_bits(float value) {
    union {
        float value;
        uint32_t bits;
    } conversion = {.value = value};
    return conversion.bits;
}

static __attribute__((noinline)) float abi_fma(float left, float right, float addend) {
    return left * right + addend;
}

static inline void write_fs0(uint32_t bits) {
    __asm__ volatile("fmv.w.x f8, %0" : : "r"(bits));
}

static inline uint32_t read_fs0(void) {
    uint32_t bits;
    __asm__ volatile("fmv.x.w %0, f8" : "=r"(bits));
    return bits;
}

static inline void write_fcsr(uint32_t value) {
    __asm__ volatile("csrw fcsr, %0" : : "r"(value));
}

static inline uint32_t read_fcsr(void) {
    uint32_t value;
    __asm__ volatile("csrr %0, fcsr" : "=r"(value));
    return value;
}

static void signal_handler(int signal_number) {
    (void)signal_number;
    write_fs0(0x40400000u);
    write_fcsr(3u << 5);
    signal_seen = 1;
}

static void test_signal_state(void) {
    struct sigaction action = {0};
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGUSR1, &action, NULL) != 0)
        fail("sigaction");

    write_fs0(0x3f800000u);
    write_fcsr(2u << 5);
    if (raise(SIGUSR1) != 0)
        fail("raise");
    if (!signal_seen || read_fs0() != 0x3f800000u || ((read_fcsr() >> 5) & 7u) != 2u)
        fail("floating-point signal frame");
}

static void context_child(unsigned rounding_mode) {
    volatile float value = 1.0f;
    const uint32_t register_value = 0x3f800000u + (rounding_mode << 16);
    write_fs0(register_value);
    write_fcsr(rounding_mode << 5);
    for (unsigned iteration = 0; iteration < 20000; ++iteration) {
        value = value * 1.00001f + 0.00001f;
        if (read_fs0() != register_value || ((read_fcsr() >> 5) & 7u) != rounding_mode)
            _exit(10);
        if ((iteration & 31u) == 0)
            sched_yield();
    }
    if (value <= 1.0f)
        _exit(11);
    _exit(0);
}

static void test_context_switch(void) {
    pid_t children[2];
    for (unsigned child = 0; child < 2; ++child) {
        children[child] = fork();
        if (children[child] < 0)
            fail("fork floating-point workers");
        if (children[child] == 0)
            context_child(child + 2);
    }
    for (unsigned child = 0; child < 2; ++child) {
        int status;
        if (waitpid(children[child], &status, 0) != children[child] || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
            fail("floating-point context switch");
    }
}

static void ptrace_child(void) {
    write_fs0(0x3f800000u);
    write_fcsr(1u << 5);
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) != 0)
        _exit(20);
    if (raise(SIGSTOP) != 0)
        _exit(21);
    if (read_fs0() != 0x40000000u || ((read_fcsr() >> 5) & 7u) != 3u)
        _exit(22);
    _exit(0);
}

static void test_ptrace_state(void) {
    pid_t child = fork();
    if (child < 0)
        fail("fork ptrace worker");
    if (child == 0)
        ptrace_child();

    int status;
    if (waitpid(child, &status, 0) != child || !WIFSTOPPED(status))
        fail("wait ptrace stop");

    struct riscv_f_state state = {0};
    struct iovec vector = {.iov_base = &state, .iov_len = sizeof(state)};
    if (ptrace(PTRACE_GETREGSET, child, (void *)(uintptr_t)NT_PRFPREG, &vector) != 0)
        fail("PTRACE_GETREGSET NT_PRFPREG");
    if (vector.iov_len != sizeof(state) || state.f[8] != 0x3f800000u)
        fail("32-bit NT_PRFPREG layout");

    state.f[8] = 0x40000000u;
    state.fcsr = 3u << 5;
    vector.iov_len = sizeof(state);
    if (ptrace(PTRACE_SETREGSET, child, (void *)(uintptr_t)NT_PRFPREG, &vector) != 0)
        fail("PTRACE_SETREGSET NT_PRFPREG");
    if (ptrace(PTRACE_CONT, child, NULL, NULL) != 0)
        fail("PTRACE_CONT");
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        fail("ptrace floating-point restore");
}

static void test_core_dump(void) {
    unlink("/tmp/core");
    pid_t child = fork();
    if (child < 0)
        fail("fork core worker");
    if (child == 0) {
        const struct rlimit limit = {.rlim_cur = RLIM_INFINITY, .rlim_max = RLIM_INFINITY};
        if (setrlimit(RLIMIT_CORE, &limit) != 0 || chdir("/tmp") != 0)
            _exit(30);
        write_fs0(0x40800000u);
        abort();
    }

    int status;
    struct stat metadata;
    if (waitpid(child, &status, 0) != child || !WIFSIGNALED(status))
        fail("wait core worker");
    if (stat("/tmp/core", &metadata) != 0 || metadata.st_size <= 0)
        fail("ELF core dump");
    unlink("/tmp/core");
}

static void test_filesystem(void) {
    static const char contents[] = "zircon-linux\n";
    char readback[sizeof(contents)] = {0};
    int descriptor = open("/tmp/zircon-validation", O_CREAT | O_TRUNC | O_RDWR, 0600);
    if (descriptor < 0 || write(descriptor, contents, sizeof(contents)) != (ssize_t)sizeof(contents) ||
        lseek(descriptor, 0, SEEK_SET) != 0 ||
        read(descriptor, readback, sizeof(readback)) != (ssize_t)sizeof(readback) ||
        memcmp(contents, readback, sizeof(contents)) != 0)
        fail("initramfs filesystem");
    close(descriptor);
    unlink("/tmp/zircon-validation");
}

int main(void) {
    const unsigned long hwcap = getauxval(AT_HWCAP);
    if ((hwcap & HWCAP_F) == 0 || (hwcap & (HWCAP_C | HWCAP_D)) != 0)
        fail("F-only HWCAP");
    if (float_bits(abi_fma(1.5f, 2.0f, 0.25f)) != 0x40500000u)
        fail("ilp32f calling convention");

    atomic_uint counter = 0;
    if (atomic_fetch_add_explicit(&counter, 7, memory_order_seq_cst) != 0 || atomic_load(&counter) != 7)
        fail("atomic operation");

    test_filesystem();
    test_context_switch();
    test_signal_state();
    test_ptrace_state();
    test_core_dump();
    puts("Zircon Linux validation: all checks passed");
    return 0;
}
