// sync.h - Cross-platform synchronization primitives
// Supports: Linux, Windows (XP+), macOS, BSD, WASM
// Uses direct syscalls, no runtime library dependencies

#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__EMSCRIPTEN__)
    #define PLATFORM_WASM
#elif defined(__linux__)
    #define PLATFORM_LINUX
#elif defined(__APPLE__)
    #define PLATFORM_DARWIN
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    #define PLATFORM_BSD
#endif

// =============================================================================
// FUTEX-LIKE BASE PRIMITIVE
// =============================================================================

// Returns: 0 on success, -1 on timeout, -2 on error
// timeout_ms: -1 for infinite, 0 for no wait, >0 for milliseconds
// Note: Always process-private. For cross-process sync, use platform APIs.
// Note: Sub-millisecond precision is not reliable due to OS scheduler limits.
int32_t futex_wait(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms);

// Returns: number of waiters woken, or -1 on error
int32_t futex_wake(atomic_int32_t* addr, int32_t count);

// =============================================================================
// MUTEX (Non-recursive)
// =============================================================================

typedef struct {
    atomic_int32_t state; // 0 = unlocked, 1 = locked no waiters, 2 = locked with waiters
} mutex_t;

void mutex_init(mutex_t* m);
void mutex_destroy(mutex_t* m);
void mutex_lock(mutex_t* m);
bool mutex_trylock(mutex_t* m);
void mutex_unlock(mutex_t* m);

// =============================================================================
// EVENT (Auto-reset and Manual-reset)
// =============================================================================

typedef struct {
    atomic_int32_t state; // 0 = non-signaled, 1 = signaled
    atomic_int32_t waiters;
    bool manual_reset;
} event_t;

void event_init(event_t* e, bool manual_reset, bool initial_state);
void event_destroy(event_t* e);
void event_wait(event_t* e);
bool event_wait_timeout(event_t* e, int64_t timeout_ns);
void event_set(event_t* e);
void event_reset(event_t* e);

// =============================================================================
// SEMAPHORE (Counting)
// =============================================================================

typedef struct {
    atomic_int32_t count;
    atomic_int32_t waiters;
} semaphore_t;

void semaphore_init(semaphore_t* s, int32_t initial_count);
void semaphore_destroy(semaphore_t* s);
void semaphore_wait(semaphore_t* s);
bool semaphore_wait_timeout(semaphore_t* s, int64_t timeout_ns);
bool semaphore_trywait(semaphore_t* s);
void semaphore_post(semaphore_t* s);

// =============================================================================
// READ-WRITE LOCK (Reader-writer lock)
// =============================================================================

typedef struct {
    atomic_int32_t state; // <0 = write locked, 0 = unlocked, >0 = read locked (count)
    atomic_int32_t write_waiters;
} rwlock_t;

void rwlock_init(rwlock_t* rw);
void rwlock_destroy(rwlock_t* rw);
void rwlock_read_lock(rwlock_t* rw);
void rwlock_write_lock(rwlock_t* rw);
void rwlock_read_unlock(rwlock_t* rw);
void rwlock_write_unlock(rwlock_t* rw);

// =============================================================================
// CONDITION VARIABLE (For use with mutex)
// =============================================================================

typedef struct {
    atomic_int32_t waiters;
    atomic_int32_t signal_gen; // Generation counter for signal tracking
} condvar_t;

void condvar_init(condvar_t* cv);
void condvar_destroy(condvar_t* cv);
void condvar_wait(condvar_t* cv, mutex_t* m);
bool condvar_wait_timeout(condvar_t* cv, mutex_t* m, int64_t timeout_ns);
void condvar_signal(condvar_t* cv);
void condvar_broadcast(condvar_t* cv);

#endif // SYNC_H

// =============================================================================
// IMPLEMENTATION
// =============================================================================

#ifdef PLATFORM_WINDOWS
#include <windows.h>

// Windows NT syscall numbers (stable across versions)
#define STATUS_SUCCESS 0x00000000
#define STATUS_TIMEOUT 0x00000102
#define STATUS_ALERTED 0x00000101

// Windows 8+ syscalls (when available)
typedef BOOL (WINAPI *WaitOnAddressFn)(volatile VOID*, PVOID, SIZE_T, DWORD);
typedef VOID (WINAPI *WakeByAddressSingleFn)(PVOID);
typedef VOID (WINAPI *WakeByAddressAllFn)(PVOID);

// Windows 7+ undocumented syscalls
typedef NTSTATUS (NTAPI *NtWaitForKeyedEventFn)(HANDLE, PVOID, BOOLEAN, PLARGE_INTEGER);
typedef NTSTATUS (NTAPI *NtReleaseKeyedEventFn)(HANDLE, PVOID, BOOLEAN);

// Fallback for Windows XP/Vista: kernel event objects
typedef struct {
    CRITICAL_SECTION lock;
    HANDLE event;
} win_futex_fallback_t;

static struct {
    enum { METHOD_UNKNOWN, METHOD_WAIT_ON_ADDRESS, METHOD_KEYED_EVENT, METHOD_FALLBACK } method;
    union {
        struct {
            WaitOnAddressFn wait;
            WakeByAddressSingleFn wake_single;
            WakeByAddressAllFn wake_all;
        } wait_on_address;
        struct {
            HANDLE handle;
            NtWaitForKeyedEventFn wait;
            NtReleaseKeyedEventFn release;
        } keyed_event;
    } impl;
} g_futex_impl = { METHOD_UNKNOWN };

static void init_futex_impl(void) {
    if (g_futex_impl.method != METHOD_UNKNOWN) return;
    
    // Try Windows 8+ WaitOnAddress
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (kernel32) {
        g_futex_impl.impl.wait_on_address.wait = 
            (WaitOnAddressFn)GetProcAddress(kernel32, "WaitOnAddress");
        g_futex_impl.impl.wait_on_address.wake_single = 
            (WakeByAddressSingleFn)GetProcAddress(kernel32, "WakeByAddressSingle");
        g_futex_impl.impl.wait_on_address.wake_all = 
            (WakeByAddressAllFn)GetProcAddress(kernel32, "WakeByAddressAll");
        
        if (g_futex_impl.impl.wait_on_address.wait) {
            g_futex_impl.method = METHOD_WAIT_ON_ADDRESS;
            return;
        }
    }
    
    // Try Windows 7+ NtWaitForKeyedEvent (undocumented but stable)
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        g_futex_impl.impl.keyed_event.wait = 
            (NtWaitForKeyedEventFn)GetProcAddress(ntdll, "NtWaitForKeyedEvent");
        g_futex_impl.impl.keyed_event.release = 
            (NtReleaseKeyedEventFn)GetProcAddress(ntdll, "NtReleaseKeyedEvent");
        
        if (g_futex_impl.impl.keyed_event.wait) {
            // Get global keyed event handle
            typedef NTSTATUS (NTAPI *NtCreateKeyedEventFn)(PHANDLE, ACCESS_MASK, PVOID, ULONG);
            NtCreateKeyedEventFn create_keyed = 
                (NtCreateKeyedEventFn)GetProcAddress(ntdll, "NtCreateKeyedEvent");
            
            if (create_keyed) {
                NTSTATUS status = create_keyed(&g_futex_impl.impl.keyed_event.handle,
                                               0x1F0003, NULL, 0);
                if (status == STATUS_SUCCESS) {
                    g_futex_impl.method = METHOD_KEYED_EVENT;
                    return;
                }
            }
        }
    }
    
    // Fallback: use kernel events (Windows XP compatible but slow)
    g_futex_impl.method = METHOD_FALLBACK;
}

// Keyed event uses the address itself as the key
static int32_t futex_wait_keyed_event(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms) {
    // Spin a bit first (keyed events are expensive)
    for (int i = 0; i < 100; i++) {
        if (atomic_load(addr) != expected) return 0;
        YieldProcessor();
    }
    
    // Check again before syscall
    if (atomic_load(addr) != expected) return 0;
    
    LARGE_INTEGER timeout;
    PLARGE_INTEGER timeout_ptr = NULL;
    if (timeout_ms >= 0) {
        // Negative value means relative time in 100ns units
        // 1ms = 10,000 * 100ns
        timeout.QuadPart = -(timeout_ms * 10000);
        timeout_ptr = &timeout;
    }
    
    NTSTATUS status = g_futex_impl.impl.keyed_event.wait(
        g_futex_impl.impl.keyed_event.handle,
        addr, // Use address as key
        FALSE,
        timeout_ptr
    );
    
    if (status == STATUS_SUCCESS) return 0;
    if (status == STATUS_TIMEOUT) return -1;
    return -2;
}

static int32_t futex_wake_keyed_event(atomic_int32_t* addr, int32_t count) {
    // Wake waiters (we don't know exact count, wake up to 'count' times)
    for (int32_t i = 0; i < count; i++) {
        g_futex_impl.impl.keyed_event.release(
            g_futex_impl.impl.keyed_event.handle,
            addr,
            FALSE
        );
    }
    return count;
}

// Fallback implementation using kernel events
#define FUTEX_FALLBACK_HASH_SIZE 256
static win_futex_fallback_t g_futex_fallback[FUTEX_FALLBACK_HASH_SIZE];
static LONG g_futex_fallback_init = 0;

static void init_futex_fallback(void) {
    if (InterlockedCompareExchange(&g_futex_fallback_init, 1, 0) == 0) {
        for (int i = 0; i < FUTEX_FALLBACK_HASH_SIZE; i++) {
            InitializeCriticalSection(&g_futex_fallback[i].lock);
            g_futex_fallback[i].event = CreateEventA(NULL, FALSE, FALSE, NULL);
        }
    } else {
        while (InterlockedCompareExchange(&g_futex_fallback_init, 1, 1) != 1) {
            Sleep(0);
        }
    }
}

static int32_t futex_wait_fallback(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms) {
    init_futex_fallback();
    
    size_t hash = ((uintptr_t)addr >> 2) % FUTEX_FALLBACK_HASH_SIZE;
    win_futex_fallback_t* bucket = &g_futex_fallback[hash];
    
    EnterCriticalSection(&bucket->lock);
    
    if (atomic_load(addr) != expected) {
        LeaveCriticalSection(&bucket->lock);
        return 0;
    }
    
    DWORD timeout_dw = (timeout_ms < 0) ? INFINITE : 
                       (timeout_ms == 0) ? 0 : 
                       (DWORD)timeout_ms;
    
    LeaveCriticalSection(&bucket->lock);
    
    DWORD result = WaitForSingleObject(bucket->event, timeout_dw);
    
    if (result == WAIT_OBJECT_0) return 0;
    if (result == WAIT_TIMEOUT) return -1;
    return -2;
}

static int32_t futex_wake_fallback(atomic_int32_t* addr, int32_t count) {
    init_futex_fallback();
    
    size_t hash = ((uintptr_t)addr >> 2) % FUTEX_FALLBACK_HASH_SIZE;
    win_futex_fallback_t* bucket = &g_futex_fallback[hash];
    
    // Wake all waiters in this bucket (can't target specific address)
    for (int32_t i = 0; i < count; i++) {
        SetEvent(bucket->event);
    }
    
    return count;
}

int32_t futex_wait(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms) {
    init_futex_impl();
    
    switch (g_futex_impl.method) {
    case METHOD_WAIT_ON_ADDRESS: {
        DWORD timeout_dw = (timeout_ms < 0) ? INFINITE : 
                           (timeout_ms == 0) ? 0 : 
                           (DWORD)timeout_ms;
        
        if (!g_futex_impl.impl.wait_on_address.wait(addr, &expected, sizeof(int32_t), timeout_dw)) {
            return (GetLastError() == ERROR_TIMEOUT) ? -1 : -2;
        }
        return 0;
    }
    case METHOD_KEYED_EVENT:
        return futex_wait_keyed_event(addr, expected, timeout_ms);
    
    case METHOD_FALLBACK:
        return futex_wait_fallback(addr, expected, timeout_ms);
    
    default:
        return -2;
    }
}

int32_t futex_wake(atomic_int32_t* addr, int32_t count) {
    init_futex_impl();
    
    switch (g_futex_impl.method) {
    case METHOD_WAIT_ON_ADDRESS:
        if (count == 1) {
            g_futex_impl.impl.wait_on_address.wake_single(addr);
        } else {
            g_futex_impl.impl.wait_on_address.wake_all(addr);
        }
        return count;
    
    case METHOD_KEYED_EVENT:
        return futex_wake_keyed_event(addr, count);
    
    case METHOD_FALLBACK:
        return futex_wake_fallback(addr, count);
    
    default:
        return -1;
    }
}

#elif defined(PLATFORM_LINUX)
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

int32_t futex_wait(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms) {
    struct timespec* ts_ptr = NULL;
    struct timespec ts;
    
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        ts_ptr = &ts;
    }
    
    long ret = syscall(SYS_futex, addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 
                       expected, ts_ptr, NULL, 0);
    if (ret == 0) return 0;
    if (errno == ETIMEDOUT) return -1;
    if (errno == EAGAIN) return 0; // Value changed, not an error
    return -2;
}

int32_t futex_wake(atomic_int32_t* addr, int32_t count) {
    long ret = syscall(SYS_futex, addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 
                       count, NULL, NULL, 0);
    return (int32_t)ret;
}

#elif defined(PLATFORM_BSD)
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#ifdef __FreeBSD__
#include <sys/umtx.h>
#else
// OpenBSD/NetBSD futex
extern int futex(volatile int*, int, int, const struct timespec*, volatile int*);
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_WAIT_PRIVATE (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#endif

int32_t futex_wait(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms) {
    struct timespec* ts_ptr = NULL;
    struct timespec ts;
    
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        ts_ptr = &ts;
    }
    
    #ifdef __FreeBSD__
    int ret = _umtx_op(addr, UMTX_OP_WAIT_UINT_PRIVATE, expected, 
                       (void*)(uintptr_t)sizeof(struct timespec), ts_ptr);
    #else
    int ret = futex((volatile int*)addr, FUTEX_WAIT_PRIVATE, expected, ts_ptr, NULL);
    #endif
    
    if (ret == 0) return 0;
    if (errno == ETIMEDOUT) return -1;
    if (errno == EAGAIN) return 0;
    return -2;
}

int32_t futex_wake(atomic_int32_t* addr, int32_t count) {
    #ifdef __FreeBSD__
    return _umtx_op(addr, UMTX_OP_WAKE_PRIVATE, count, NULL, NULL);
    #else
    return futex((volatile int*)addr, FUTEX_WAKE_PRIVATE, count, NULL, NULL);
    #endif
}

#elif defined(PLATFORM_DARWIN)
#include <errno.h>
#include <time.h>
#include <stdint.h>

// macOS private API (stable, used by libc++ and Swift runtime)
extern int __ulock_wait(uint32_t operation, void* addr, uint64_t value, uint32_t timeout_us);
extern int __ulock_wake(uint32_t operation, void* addr, uint64_t wake_value);

#define UL_COMPARE_AND_WAIT 1
#define ULF_WAKE_ALL 0x00000100
#define ULF_NO_ERRNO 0x01000000

int32_t futex_wait(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms) {
    // Convert milliseconds to microseconds for __ulock_wait
    uint32_t timeout_us = (timeout_ms < 0) ? 0 : 
                          (timeout_ms == 0) ? 0 :
                          (uint32_t)(timeout_ms * 1000);
    
    int ret = __ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, addr, expected, timeout_us);
    if (ret == 0) return 0;
    if (ret == -ETIMEDOUT) return -1;
    if (ret == -EAGAIN) return 0;
    return -2;
}

int32_t futex_wake(atomic_int32_t* addr, int32_t count) {
    uint32_t op = (count == 1) ? 
        (UL_COMPARE_AND_WAIT | ULF_NO_ERRNO) : 
        (UL_COMPARE_AND_WAIT | ULF_WAKE_ALL | ULF_NO_ERRNO);
    
    int ret = __ulock_wake(op, addr, 0);
    return (ret < 0) ? -1 : count;
}

#elif defined(PLATFORM_WASM)
#include <emscripten/threading.h>
#include <errno.h>

int32_t futex_wait(atomic_int32_t* addr, int32_t expected, int64_t timeout_ms) {
    double timeout_dbl = (timeout_ms < 0) ? INFINITY : 
                         (timeout_ms == 0) ? 0 : 
                         (double)timeout_ms;
    
    int ret = emscripten_futex_wait(addr, expected, timeout_dbl);
    if (ret == 0) return 0;
    if (ret == -ETIMEDOUT) return -1;
    return 0; // EAGAIN means value changed
}

int32_t futex_wake(atomic_int32_t* addr, int32_t count) {
    return emscripten_futex_wake(addr, count);
}

#endif

// =============================================================================
// MUTEX IMPLEMENTATION
// =============================================================================

void mutex_init(mutex_t* m) {
    atomic_init(&m->state, 0);
}

void mutex_destroy(mutex_t* m) {
    (void)m; // No cleanup needed
}

void mutex_lock(mutex_t* m) {
    // Fast path: try to acquire unlocked mutex
    int32_t expected = 0;
    if (atomic_compare_exchange_strong(&m->state, &expected, 1)) {
        return;
    }
    
    // Slow path: contention
    do {
        // If state is 2, someone is waiting, just wait
        if (expected == 2 || 
            atomic_compare_exchange_strong(&m->state, &expected, 2)) {
            futex_wait(&m->state, 2, -1);
        }
        expected = 0;
    } while (!atomic_compare_exchange_strong(&m->state, &expected, 2));
}

bool mutex_trylock(mutex_t* m) {
    int32_t expected = 0;
    return atomic_compare_exchange_strong(&m->state, &expected, 1);
}

void mutex_unlock(mutex_t* m) {
    // Fast path: no waiters
    int32_t prev = atomic_exchange(&m->state, 0);
    if (prev == 2) {
        // Had waiters, wake one
        futex_wake(&m->state, 1);
    }
}

// =============================================================================
// EVENT IMPLEMENTATION
// =============================================================================

void event_init(event_t* e, bool manual_reset, bool initial_state) {
    atomic_init(&e->state, initial_state ? 1 : 0);
    atomic_init(&e->waiters, 0);
    e->manual_reset = manual_reset;
}

void event_destroy(event_t* e) {
    (void)e;
}

void event_wait(event_t* e) {
    event_wait_timeout(e, -1);
}

bool event_wait_timeout(event_t* e, int64_t timeout_ns) {
    while (true) {
        if (atomic_load(&e->state) == 1) {
            if (!e->manual_reset) {
                // Auto-reset: try to consume the signal
                int32_t expected = 1;
                if (atomic_compare_exchange_strong(&e->state, &expected, 0)) {
                    return true;
                }
                continue;
            }
            return true;
        }
        
        atomic_fetch_add(&e->waiters, 1);
        int32_t result = futex_wait(&e->state, 0, timeout_ns);
        atomic_fetch_sub(&e->waiters, 1);
        
        if (result == -1) return false; // Timeout
        if (atomic_load(&e->state) == 1) {
            if (!e->manual_reset) {
                int32_t expected = 1;
                if (atomic_compare_exchange_strong(&e->state, &expected, 0)) {
                    return true;
                }
            } else {
                return true;
            }
        }
    }
}

void event_set(event_t* e) {
    atomic_store(&e->state, 1);
    int32_t waiters = atomic_load(&e->waiters);
    if (waiters > 0) {
        futex_wake(&e->state, e->manual_reset ? INT32_MAX : 1);
    }
}

void event_reset(event_t* e) {
    atomic_store(&e->state, 0);
}

// =============================================================================
// SEMAPHORE IMPLEMENTATION
// =============================================================================

void semaphore_init(semaphore_t* s, int32_t initial_count) {
    atomic_init(&s->count, initial_count);
    atomic_init(&s->waiters, 0);
}

void semaphore_destroy(semaphore_t* s) {
    (void)s;
}

void semaphore_wait(semaphore_t* s) {
    semaphore_wait_timeout(s, -1);
}

bool semaphore_wait_timeout(semaphore_t* s, int64_t timeout_ns) {
    while (true) {
        int32_t count = atomic_load(&s->count);
        if (count > 0) {
            if (atomic_compare_exchange_weak(&s->count, &count, count - 1)) {
                return true;
            }
            continue;
        }
        
        atomic_fetch_add(&s->waiters, 1);
        int32_t result = futex_wait(&s->count, 0, timeout_ns);
        atomic_fetch_sub(&s->waiters, 1);
        
        if (result == -1) return false; // Timeout
    }
}

bool semaphore_trywait(semaphore_t* s) {
    int32_t count = atomic_load(&s->count);
    while (count > 0) {
        if (atomic_compare_exchange_weak(&s->count, &count, count - 1)) {
            return true;
        }
    }
    return false;
}

void semaphore_post(semaphore_t* s) {
    atomic_fetch_add(&s->count, 1);
    if (atomic_load(&s->waiters) > 0) {
        futex_wake(&s->count, 1);
    }
}

// =============================================================================
// READ-WRITE LOCK IMPLEMENTATION
// =============================================================================

void rwlock_init(rwlock_t* rw) {
    atomic_init(&rw->state, 0);
    atomic_init(&rw->write_waiters, 0);
}

void rwlock_destroy(rwlock_t* rw) {
    (void)rw;
}

void rwlock_read_lock(rwlock_t* rw) {
    while (true) {
        int32_t state = atomic_load(&rw->state);
        if (state >= 0 && atomic_load(&rw->write_waiters) == 0) {
            if (atomic_compare_exchange_weak(&rw->state, &state, state + 1)) {
                return;
            }
        } else {
            futex_wait(&rw->state, state, -1);
        }
    }
}

void rwlock_write_lock(rwlock_t* rw) {
    atomic_fetch_add(&rw->write_waiters, 1);
    
    while (true) {
        int32_t expected = 0;
        if (atomic_compare_exchange_weak(&rw->state, &expected, -1)) {
            atomic_fetch_sub(&rw->write_waiters, 1);
            return;
        }
        futex_wait(&rw->state, expected, -1);
    }
}

void rwlock_read_unlock(rwlock_t* rw) {
    if (atomic_fetch_sub(&rw->state, 1) == 1) {
        if (atomic_load(&rw->write_waiters) > 0) {
            futex_wake(&rw->state, 1);
        }
    }
}

void rwlock_write_unlock(rwlock_t* rw) {
    atomic_store(&rw->state, 0);
    futex_wake(&rw->state, INT32_MAX);
}

// =============================================================================
// CONDITION VARIABLE IMPLEMENTATION
// =============================================================================

void condvar_init(condvar_t* cv) {
    atomic_init(&cv->waiters, 0);
    atomic_init(&cv->signal_gen, 0);
}

void condvar_destroy(condvar_t* cv) {
    (void)cv;
}

void condvar_wait(condvar_t* cv, mutex_t* m) {
    condvar_wait_timeout(cv, m, -1);
}

bool condvar_wait_timeout(condvar_t* cv, mutex_t* m, int64_t timeout_ns) {
    atomic_fetch_add(&cv->waiters, 1);
    int32_t gen = atomic_load(&cv->signal_gen);
    
    mutex_unlock(m);
    
    int32_t result = futex_wait(&cv->signal_gen, gen, timeout_ns);
    
    atomic_fetch_sub(&cv->waiters, 1);
    mutex_lock(m);
    
    return result != -1;
}

void condvar_signal(condvar_t* cv) {
    if (atomic_load(&cv->waiters) > 0) {
        atomic_fetch_add(&cv->signal_gen, 1);
        futex_wake(&cv->signal_gen, 1);
    }
}

void condvar_broadcast(condvar_t* cv) {
    if (atomic_load(&cv->waiters) > 0) {
        atomic_fetch_add(&cv->signal_gen, 1);
        futex_wake(&cv->signal_gen, INT32_MAX);
    }
}