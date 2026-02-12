
struct Test_MTSync
{


// ============================================================================
// Multithreaded Tests for Synchronization Primitives
// ============================================================================

#pragma once

#include "sync_primitives.h"

// ============================================================================
// Test 1: Basic Mutex - Counter Increment
// ============================================================================

struct TestMutexBasic
{
    static constexpr int NUM_THREADS = 8;
    static constexpr int ITERATIONS = 10000;
    
    struct ThreadData {
        Mutex* mutex;
        int* counter;
        PX::pid_t tid;
    };
    
    static sint ThreadProc(ThreadData* data)
    {
        for(int i = 0; i < ITERATIONS; i++)
        {
            data->mutex->Lock();
            (*data->counter)++;
            data->mutex->Unlock();
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestMutexBasic: Starting...");
        
        Mutex mutex;
        mutex.Init();
        int counter = 0;
        
        ThreadData td[NUM_THREADS];
        
        // Create threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            td[i].mutex = &mutex;
            td[i].counter = &counter;
            td[i].tid = PX::thread((PThreadProc)ThreadProc, &td[i], sizeof(ThreadData), nullptr);
            RT_TEST(td[i].tid, > 0, "Failed to create thread");
        }
        
        // Wait for all threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            RT_TEST(PX::thread_wait(td[i].tid, -1, nullptr), == 0, "thread_wait failed");
        }
        
        // Verify result
        RT_TEST(counter, == (NUM_THREADS * ITERATIONS), "Counter mismatch");
        
        mutex.Destroy();
        LOGMSG("TestMutexBasic: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 2: Mutex Contention - TryLock
// ============================================================================

struct TestMutexContention
{
    static constexpr int NUM_THREADS = 4;
    static constexpr int DURATION_MS = 1000;
    
    struct ThreadData {
        Mutex* mutex;
        std::atomic<int>* success_count;
        std::atomic<int>* fail_count;
        std::atomic<bool>* running;
    };
    
    static sint ThreadProc(ThreadData* data)
    {
        while(data->running->load())
        {
            if(data->mutex->TryLock() == 0)
            {
                (*data->success_count)++;
                PX::thread_sleep(100);  // Hold lock for 100us
                data->mutex->Unlock();
            }
            else
            {
                (*data->fail_count)++;
            }
            PX::thread_sleep(10);  // Small delay
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestMutexContention: Starting...");
        
        Mutex mutex;
        mutex.Init();
        std::atomic<int> success_count{0};
        std::atomic<int> fail_count{0};
        std::atomic<bool> running{true};
        
        ThreadData td[NUM_THREADS];
        PX::pid_t tids[NUM_THREADS];
        
        // Create threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            td[i].mutex = &mutex;
            td[i].success_count = &success_count;
            td[i].fail_count = &fail_count;
            td[i].running = &running;
            tids[i] = PX::thread((PThreadProc)ThreadProc, &td[i], sizeof(ThreadData), nullptr);
            RT_TEST(tids[i], > 0, "Failed to create thread");
        }
        
        // Let them run
        PX::thread_sleep(DURATION_MS * 1000);
        
        // Stop threads
        running.store(false);
        
        // Wait for all threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            RT_TEST(PX::thread_wait(tids[i], -1, nullptr), == 0, "thread_wait failed");
        }
        
        LOGMSG("  Success: %d, Fail: %d", success_count.load(), fail_count.load());
        RT_TEST(success_count.load(), > 0, "No successful locks");
        RT_TEST(fail_count.load(), > 0, "No failed locks (no contention)");
        
        mutex.Destroy();
        LOGMSG("TestMutexContention: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 3: Semaphore - Producer Consumer
// ============================================================================

struct TestSemaphore
{
    static constexpr int NUM_PRODUCERS = 3;
    static constexpr int NUM_CONSUMERS = 3;
    static constexpr int ITEMS_PER_PRODUCER = 1000;
    static constexpr int BUFFER_SIZE = 10;
    
    struct SharedData {
        Semaphore empty_slots;
        Semaphore full_slots;
        Mutex mutex;
        int buffer[BUFFER_SIZE];
        int write_idx;
        int read_idx;
        std::atomic<int> items_produced{0};
        std::atomic<int> items_consumed{0};
    };
    
    static sint ProducerProc(SharedData* data)
    {
        for(int i = 0; i < ITEMS_PER_PRODUCER; i++)
        {
            int value = data->items_produced.fetch_add(1);
            
            data->empty_slots.Wait();
            data->mutex.Lock();
            data->buffer[data->write_idx] = value;
            data->write_idx = (data->write_idx + 1) % BUFFER_SIZE;
            data->mutex.Unlock();
            data->full_slots.Signal();
        }
        return 0;
    }
    
    static sint ConsumerProc(SharedData* data)
    {
        int total_items = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
        
        while(data->items_consumed.load() < total_items)
        {
            if(data->full_slots.WaitTimeout(100) == 0)  // 100ms timeout
            {
                data->mutex.Lock();
                int value = data->buffer[data->read_idx];
                data->read_idx = (data->read_idx + 1) % BUFFER_SIZE;
                data->mutex.Unlock();
                data->empty_slots.Signal();
                
                data->items_consumed.fetch_add(1);
            }
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestSemaphore: Starting...");
        
        SharedData data;
        data.empty_slots.Init(BUFFER_SIZE);
        data.full_slots.Init(0);
        data.mutex.Init();
        data.write_idx = 0;
        data.read_idx = 0;
        
        PX::pid_t producers[NUM_PRODUCERS];
        PX::pid_t consumers[NUM_CONSUMERS];
        
        // Create producers
        for(int i = 0; i < NUM_PRODUCERS; i++)
        {
            producers[i] = PX::thread((PThreadProc)ProducerProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(producers[i], > 0, "Failed to create producer");
        }
        
        // Create consumers
        for(int i = 0; i < NUM_CONSUMERS; i++)
        {
            consumers[i] = PX::thread((PThreadProc)ConsumerProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(consumers[i], > 0, "Failed to create consumer");
        }
        
        // Wait for all threads
        for(int i = 0; i < NUM_PRODUCERS; i++)
            RT_TEST(PX::thread_wait(producers[i], -1, nullptr), == 0, "producer wait failed");
        
        for(int i = 0; i < NUM_CONSUMERS; i++)
            RT_TEST(PX::thread_wait(consumers[i], -1, nullptr), == 0, "consumer wait failed");
        
        // Verify
        int expected = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
        RT_TEST(data.items_produced.load(), == expected, "Production count mismatch");
        RT_TEST(data.items_consumed.load(), == expected, "Consumption count mismatch");
        
        data.empty_slots.Destroy();
        data.full_slots.Destroy();
        data.mutex.Destroy();
        
        LOGMSG("TestSemaphore: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 4: Condition Variable - Producer Consumer
// ============================================================================

struct TestCondition
{
    static constexpr int NUM_PRODUCERS = 2;
    static constexpr int NUM_CONSUMERS = 2;
    static constexpr int ITEMS_PER_PRODUCER = 500;
    static constexpr int BUFFER_SIZE = 5;
    
    struct SharedData {
        Mutex mutex;
        Condition not_full;
        Condition not_empty;
        int buffer[BUFFER_SIZE];
        int count;
        int write_idx;
        int read_idx;
        std::atomic<int> items_produced{0};
        std::atomic<int> items_consumed{0};
        std::atomic<bool> done{false};
    };
    
    static sint ProducerProc(SharedData* data)
    {
        for(int i = 0; i < ITEMS_PER_PRODUCER; i++)
        {
            int value = data->items_produced.fetch_add(1);
            
            data->mutex.Lock();
            while(data->count >= BUFFER_SIZE)
            {
                data->not_full.Wait(&data->mutex);
            }
            
            data->buffer[data->write_idx] = value;
            data->write_idx = (data->write_idx + 1) % BUFFER_SIZE;
            data->count++;
            
            data->not_empty.Signal();
            data->mutex.Unlock();
        }
        return 0;
    }
    
    static sint ConsumerProc(SharedData* data)
    {
        int total_items = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
        
        while(data->items_consumed.load() < total_items)
        {
            data->mutex.Lock();
            while(data->count == 0 && !data->done.load())
            {
                data->not_empty.WaitTimeout(&data->mutex, 100);  // 100ms timeout
            }
            
            if(data->count > 0)
            {
                int value = data->buffer[data->read_idx];
                data->read_idx = (data->read_idx + 1) % BUFFER_SIZE;
                data->count--;
                data->items_consumed.fetch_add(1);
                data->not_full.Signal();
            }
            
            data->mutex.Unlock();
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestCondition: Starting...");
        
        SharedData data;
        data.mutex.Init();
        data.not_full.Init();
        data.not_empty.Init();
        data.count = 0;
        data.write_idx = 0;
        data.read_idx = 0;
        
        PX::pid_t producers[NUM_PRODUCERS];
        PX::pid_t consumers[NUM_CONSUMERS];
        
        // Create producers
        for(int i = 0; i < NUM_PRODUCERS; i++)
        {
            producers[i] = PX::thread((PThreadProc)ProducerProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(producers[i], > 0, "Failed to create producer");
        }
        
        // Create consumers
        for(int i = 0; i < NUM_CONSUMERS; i++)
        {
            consumers[i] = PX::thread((PThreadProc)ConsumerProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(consumers[i], > 0, "Failed to create consumer");
        }
        
        // Wait for all threads
        for(int i = 0; i < NUM_PRODUCERS; i++)
            RT_TEST(PX::thread_wait(producers[i], -1, nullptr), == 0, "producer wait failed");
        
        data.done.store(true);
        data.not_empty.Broadcast();  // Wake all consumers to check done flag
        
        for(int i = 0; i < NUM_CONSUMERS; i++)
            RT_TEST(PX::thread_wait(consumers[i], -1, nullptr), == 0, "consumer wait failed");
        
        // Verify
        int expected = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
        RT_TEST(data.items_produced.load(), == expected, "Production count mismatch");
        RT_TEST(data.items_consumed.load(), == expected, "Consumption count mismatch");
        RT_TEST(data.count, == 0, "Buffer should be empty");
        
        data.mutex.Destroy();
        data.not_full.Destroy();
        data.not_empty.Destroy();
        
        LOGMSG("TestCondition: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 5: RWLock - Readers/Writers
// ============================================================================

struct TestRWLock
{
    static constexpr int NUM_READERS = 8;
    static constexpr int NUM_WRITERS = 2;
    static constexpr int DURATION_MS = 1000;
    
    struct SharedData {
        RWLock rwlock;
        int value;
        std::atomic<int> read_count{0};
        std::atomic<int> write_count{0};
        std::atomic<int> concurrent_readers{0};
        std::atomic<int> max_concurrent_readers{0};
        std::atomic<bool> running{true};
    };
    
    static sint ReaderProc(SharedData* data)
    {
        while(data->running.load())
        {
            data->rwlock.LockRead();
            
            // Track concurrent readers
            int concurrent = data->concurrent_readers.fetch_add(1) + 1;
            int max_val = data->max_concurrent_readers.load();
            while(concurrent > max_val && 
                  !data->max_concurrent_readers.compare_exchange_weak(max_val, concurrent));
            
            int value = data->value;  // Read
            data->read_count++;
            PX::thread_sleep(100);  // Simulate read work
            
            data->concurrent_readers.fetch_sub(1);
            data->rwlock.Unlock();
            
            PX::thread_sleep(50);
        }
        return 0;
    }
    
    static sint WriterProc(SharedData* data)
    {
        while(data->running.load())
        {
            data->rwlock.LockWrite();
            
            // Verify exclusive access
            if(data->concurrent_readers.load() != 0)
            {
                LOGERR("ERROR: Concurrent readers during write!");
            }
            
            data->value++;
            data->write_count++;
            PX::thread_sleep(200);  // Simulate write work
            
            data->rwlock.Unlock();
            
            PX::thread_sleep(100);
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestRWLock: Starting...");
        
        SharedData data;
        data.rwlock.Init();
        data.value = 0;
        
        PX::pid_t readers[NUM_READERS];
        PX::pid_t writers[NUM_WRITERS];
        
        // Create readers
        for(int i = 0; i < NUM_READERS; i++)
        {
            readers[i] = PX::thread((PThreadProc)ReaderProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(readers[i], > 0, "Failed to create reader");
        }
        
        // Create writers
        for(int i = 0; i < NUM_WRITERS; i++)
        {
            writers[i] = PX::thread((PThreadProc)WriterProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(writers[i], > 0, "Failed to create writer");
        }
        
        // Let them run
        PX::thread_sleep(DURATION_MS * 1000);
        
        // Stop threads
        data.running.store(false);
        
        // Wait for all threads
        for(int i = 0; i < NUM_READERS; i++)
            RT_TEST(PX::thread_wait(readers[i], -1, nullptr), == 0, "reader wait failed");
        
        for(int i = 0; i < NUM_WRITERS; i++)
            RT_TEST(PX::thread_wait(writers[i], -1, nullptr), == 0, "writer wait failed");
        
        LOGMSG("  Reads: %d, Writes: %d, Max concurrent readers: %d", 
               data.read_count.load(), data.write_count.load(), 
               data.max_concurrent_readers.load());
        
        RT_TEST(data.read_count.load(), > 0, "No reads occurred");
        RT_TEST(data.write_count.load(), > 0, "No writes occurred");
        RT_TEST(data.max_concurrent_readers.load(), > 1, "Readers didn't overlap");
        RT_TEST(data.value, == data.write_count.load(), "Value mismatch");
        
        data.rwlock.Destroy();
        LOGMSG("TestRWLock: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 6: Barrier - Phase Synchronization
// ============================================================================

struct TestBarrier
{
    static constexpr int NUM_THREADS = 8;
    static constexpr int NUM_PHASES = 5;
    
    struct SharedData {
        Barrier barrier;
        std::atomic<int> phase_counters[NUM_PHASES];
        std::atomic<int> leader_count{0};
    };
    
    static sint ThreadProc(SharedData* data)
    {
        for(int phase = 0; phase < NUM_PHASES; phase++)
        {
            // Do work for this phase
            PX::thread_sleep(100 + (rand() % 200));  // Random work
            
            // Increment phase counter
            data->phase_counters[phase].fetch_add(1);
            
            // Wait for all threads
            int is_leader = data->barrier.Wait();
            if(is_leader)
            {
                data->leader_count.fetch_add(1);
            }
            
            // Verify all threads completed this phase
            if(data->phase_counters[phase].load() != NUM_THREADS)
            {
                LOGERR("ERROR: Phase %d incomplete!", phase);
            }
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestBarrier: Starting...");
        
        SharedData data;
        data.barrier.Init(NUM_THREADS);
        for(int i = 0; i < NUM_PHASES; i++)
            data.phase_counters[i].store(0);
        
        PX::pid_t tids[NUM_THREADS];
        
        // Create threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            tids[i] = PX::thread((PThreadProc)ThreadProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(tids[i], > 0, "Failed to create thread");
        }
        
        // Wait for all threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            RT_TEST(PX::thread_wait(tids[i], -1, nullptr), == 0, "thread_wait failed");
        }
        
        // Verify all phases completed
        for(int i = 0; i < NUM_PHASES; i++)
        {
            RT_TEST(data.phase_counters[i].load(), == NUM_THREADS, 
                    "Phase %d incomplete", i);
        }
        
        // Verify exactly one leader per phase
        RT_TEST(data.leader_count.load(), == NUM_PHASES, "Leader count mismatch");
        
        data.barrier.Destroy();
        LOGMSG("TestBarrier: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 7: Once - One-Time Initialization
// ============================================================================

struct TestOnce
{
    static constexpr int NUM_THREADS = 16;
    
    struct SharedData {
        Once once;
        std::atomic<int> init_count{0};
        std::atomic<int> access_count{0};
        int* resource;
    };
    
    static sint ThreadProc(SharedData* data)
    {
        // All threads try to initialize
        data->once.Call([&]() {
            data->init_count.fetch_add(1);
            PX::thread_sleep(10000);  // Simulate slow initialization
            data->resource = new int(42);
        });
        
        // All threads should see initialized resource
        if(data->resource != nullptr && *data->resource == 42)
        {
            data->access_count.fetch_add(1);
        }
        
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestOnce: Starting...");
        
        SharedData data;
        data.once.Init();
        data.resource = nullptr;
        
        PX::pid_t tids[NUM_THREADS];
        
        // Create threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            tids[i] = PX::thread((PThreadProc)ThreadProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(tids[i], > 0, "Failed to create thread");
        }
        
        // Wait for all threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            RT_TEST(PX::thread_wait(tids[i], -1, nullptr), == 0, "thread_wait failed");
        }
        
        // Verify initialization happened exactly once
        RT_TEST(data.init_count.load(), == 1, "Initialization count should be 1");
        RT_TEST(data.access_count.load(), == NUM_THREADS, "Not all threads saw resource");
        RT_TEST(data.resource, != nullptr, "Resource not initialized");
        RT_TEST(*data.resource, == 42, "Resource value incorrect");
        
        delete data.resource;
        LOGMSG("TestOnce: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 8: Event - Manual Reset
// ============================================================================

struct TestEventManual
{
    static constexpr int NUM_THREADS = 10;
    
    struct SharedData {
        Event start_event;
        std::atomic<int> ready_count{0};
        std::atomic<int> started_count{0};
    };
    
    static sint ThreadProc(SharedData* data)
    {
        // Signal ready
        data->ready_count.fetch_add(1);
        
        // Wait for start signal
        data->start_event.Wait();
        
        // All threads should start together
        data->started_count.fetch_add(1);
        
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestEventManual: Starting...");
        
        SharedData data;
        data.start_event.Init(true, false);  // Manual-reset, not signaled
        
        PX::pid_t tids[NUM_THREADS];
        
        // Create threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            tids[i] = PX::thread((PThreadProc)ThreadProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(tids[i], > 0, "Failed to create thread");
        }
        
        // Wait for all threads to be ready
        while(data.ready_count.load() < NUM_THREADS)
        {
            PX::thread_sleep(1000);
        }
        
        // Release all threads at once
        data.start_event.Set();
        
        // Wait for all threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            RT_TEST(PX::thread_wait(tids[i], -1, nullptr), == 0, "thread_wait failed");
        }
        
        // Verify all threads started
        RT_TEST(data.started_count.load(), == NUM_THREADS, "Not all threads started");
        
        data.start_event.Destroy();
        LOGMSG("TestEventManual: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 9: Event - Auto Reset
// ============================================================================

struct TestEventAuto
{
    static constexpr int NUM_SIGNALS = 20;
    
    struct SharedData {
        Event event;
        std::atomic<int> wake_count{0};
    };
    
    static sint WaiterProc(SharedData* data)
    {
        for(int i = 0; i < NUM_SIGNALS; i++)
        {
            data->event.Wait();
            data->wake_count.fetch_add(1);
        }
        return 0;
    }
    
    static sint SignalerProc(SharedData* data)
    {
        for(int i = 0; i < NUM_SIGNALS; i++)
        {
            PX::thread_sleep(1000);
            data->event.Set();
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestEventAuto: Starting...");
        
        SharedData data;
        data.event.Init(false, false);  // Auto-reset, not signaled
        
        PX::pid_t waiter = PX::thread((PThreadProc)WaiterProc, &data, sizeof(SharedData*), nullptr);
        RT_TEST(waiter, > 0, "Failed to create waiter");
        
        PX::pid_t signaler = PX::thread((PThreadProc)SignalerProc, &data, sizeof(SharedData*), nullptr);
        RT_TEST(signaler, > 0, "Failed to create signaler");
        
        RT_TEST(PX::thread_wait(waiter, -1, nullptr), == 0, "waiter wait failed");
        RT_TEST(PX::thread_wait(signaler, -1, nullptr), == 0, "signaler wait failed");
        
        RT_TEST(data.wake_count.load(), == NUM_SIGNALS, "Wake count mismatch");
        
        data.event.Destroy();
        LOGMSG("TestEventAuto: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 10: Recursive Mutex
// ============================================================================

struct TestRecursiveMutex
{
    static constexpr int NUM_THREADS = 4;
    static constexpr int RECURSION_DEPTH = 10;
    
    struct SharedData {
        RecursiveMutex rmutex;
        std::atomic<int> counter{0};
    };
    
    static void RecursiveFunc(SharedData* data, int depth)
    {
        if(depth == 0)
        {
            data->counter.fetch_add(1);
            return;
        }
        
        data->rmutex.Lock();
        RecursiveFunc(data, depth - 1);
        data->rmutex.Unlock();
    }
    
    static sint ThreadProc(SharedData* data)
    {
        for(int i = 0; i < 100; i++)
        {
            RecursiveFunc(data, RECURSION_DEPTH);
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestRecursiveMutex: Starting...");
        
        SharedData data;
        data.rmutex.Init();
        
        PX::pid_t tids[NUM_THREADS];
        
        // Create threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            tids[i] = PX::thread((PThreadProc)ThreadProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(tids[i], > 0, "Failed to create thread");
        }
        
        // Wait for all threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            RT_TEST(PX::thread_wait(tids[i], -1, nullptr), == 0, "thread_wait failed");
        }
        
        RT_TEST(data.counter.load(), == (NUM_THREADS * 100), "Counter mismatch");
        
        data.rmutex.Destroy();
        LOGMSG("TestRecursiveMutex: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 11: Stress Test - Mixed Operations
// ============================================================================

struct TestStress
{
    static constexpr int NUM_THREADS = 16;
    static constexpr int DURATION_MS = 2000;
    
    struct SharedData {
        Mutex mutex;
        Semaphore sem;
        Condition cond;
        RWLock rwlock;
        std::atomic<bool> running{true};
        std::atomic<uint64> operation_count{0};
        int data[100];
    };
    
    static sint ThreadProc(SharedData* data)
    {
        uint32 tid = (uint32)(uintptr_t)PX::NtCurrentTeb()->ClientId.UniqueThread;
        
        while(data->running.load())
        {
            int op = (tid + data->operation_count.load()) % 6;
            
            switch(op)
            {
                case 0:  // Mutex
                    data->mutex.Lock();
                    data->data[0]++;
                    data->mutex.Unlock();
                    break;
                    
                case 1:  // Semaphore
                    data->sem.Signal();
                    data->sem.Wait();
                    break;
                    
                case 2:  // Condition (spurious)
                    data->mutex.Lock();
                    data->cond.WaitTimeout(&data->mutex, 1);
                    data->mutex.Unlock();
                    break;
                    
                case 3:  // Condition signal
                    data->cond.Signal();
                    break;
                    
                case 4:  // RWLock read
                    data->rwlock.LockRead();
                    volatile int val = data->data[50];
                    data->rwlock.Unlock();
                    break;
                    
                case 5:  // RWLock write
                    data->rwlock.LockWrite();
                    data->data[50]++;
                    data->rwlock.Unlock();
                    break;
            }
            
            data->operation_count.fetch_add(1);
        }
        return 0;
    }
    
    static int DoTests()
    {
        LOGMSG("TestStress: Starting...");
        
        SharedData data;
        data.mutex.Init();
        data.sem.Init(1);
        data.cond.Init();
        data.rwlock.Init();
        for(int i = 0; i < 100; i++) data.data[i] = 0;
        
        PX::pid_t tids[NUM_THREADS];
        
        // Create threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            tids[i] = PX::thread((PThreadProc)ThreadProc, &data, sizeof(SharedData*), nullptr);
            RT_TEST(tids[i], > 0, "Failed to create thread");
        }
        
        // Let them run
        PX::thread_sleep(DURATION_MS * 1000);
        
        // Stop threads
        data.running.store(false);
        
        // Wait for all threads
        for(int i = 0; i < NUM_THREADS; i++)
        {
            RT_TEST(PX::thread_wait(tids[i], -1, nullptr), == 0, "thread_wait failed");
        }
        
        LOGMSG("  Total operations: %llu", data.operation_count.load());
        RT_TEST(data.operation_count.load(), > 1000, "Too few operations");
        
        data.mutex.Destroy();
        data.sem.Destroy();
        data.cond.Destroy();
        data.rwlock.Destroy();
        
        LOGMSG("TestStress: PASSED");
        return 0;
    }
};

// ============================================================================
// Test 12: Timeout Handling
// ============================================================================

struct TestTimeout
{
    struct SharedData {
        Semaphore sem;
        Condition cond;
        Mutex mutex;
    };
    
    static int DoTests()
    {
        LOGMSG("TestTimeout: Starting...");
        
        SharedData data;
        data.sem.Init(0);
        data.cond.Init();
        data.mutex.Init();
        
        // Test semaphore timeout
        uint64 start = GetTickCount();
        sint32 res = data.sem.WaitTimeout(100);  // 100ms
        uint64 elapsed = GetTickCount() - start;
        
        RT_TEST(res, == -PX::ETIMEDOUT, "Semaphore should timeout");
        RT_TEST(elapsed, >= 90, "Timeout too short");
        RT_TEST(elapsed, < 200, "Timeout too long");
        
        // Test condition timeout
        data.mutex.Lock();
        start = GetTickCount();
        res = data.cond.WaitTimeout(&data.mutex, 100);  // 100ms
        elapsed = GetTickCount() - start;
        data.mutex.Unlock();
        
        RT_TEST(res, == -PX::ETIMEDOUT, "Condition should timeout");
        RT_TEST(elapsed, >= 90, "Timeout too short");
        RT_TEST(elapsed, < 200, "Timeout too long");
        
        data.sem.Destroy();
        data.cond.Destroy();
        data.mutex.Destroy();
        
        LOGMSG("TestTimeout: PASSED");
        return 0;
    }
    
    static uint64 GetTickCount()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
};

//====================================================================================
static int DoTests(void)
{
 LOGMSG("Begin: Synchronization Primitives Test");

 DO_TEST(TestMutexBasic);
 DO_TEST(TestMutexContention);
 DO_TEST(TestSemaphore);
 DO_TEST(TestCondition);
 DO_TEST(TestRWLock);
 DO_TEST(TestBarrier);
 DO_TEST(TestOnce);
 DO_TEST(TestEventManual);
 DO_TEST(TestEventAuto);
 DO_TEST(TestRecursiveMutex);
 DO_TEST(TestStress);
 DO_TEST(TestTimeout);

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 LOGMSG("Begin: ?????");

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
};
