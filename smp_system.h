#ifndef SMP_SYSTEM_H
#define SMP_SYSTEM_H

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <queue>
#include <memory>

// ============================================================================
// SMP SYSTEM - Traditional Shared Memory Multiprocessing
// ============================================================================

struct SMPProcess {
    int pid;
    int priority;
    int assigned_core;
    std::chrono::steady_clock::time_point creation_time;
    
    SMPProcess(int id, int prio) 
        : pid(id), priority(prio), assigned_core(-1),
          creation_time(std::chrono::steady_clock::now()) {}
};

class SMPSystem {
private:
    static const int NUM_CORES = 8;
    
    // Shared memory structures (the SMP way)
    std::vector<std::shared_ptr<SMPProcess>> global_process_table;  // SHARED!
    std::mutex global_lock;  // Single global lock - contention point!
    std::atomic<int> next_pid{0};
    std::atomic<bool> running{false};
    std::atomic<uint64_t> lock_contentions{0};
    std::atomic<uint64_t> cache_invalidations{0};
    
    // Core threads
    std::vector<std::thread> core_threads;
    
    // Statistics
    std::atomic<uint64_t> total_operations{0};
    
public:
    SMPSystem() {}
    
    ~SMPSystem() {
        stop();
    }
    
    void start() {
        if (running) return;
        
        running = true;
        
        std::cout << "\n[SMP] Starting traditional SMP system..." << std::endl;
        std::cout << "[SMP] Using SHARED MEMORY model" << std::endl;
        std::cout << "[SMP] Single global lock for all cores" << std::endl;
        
        // Start core threads - all competing for the same lock
        for (int i = 0; i < NUM_CORES; i++) {
            core_threads.emplace_back(&SMPSystem::core_worker, this, i);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void stop() {
        if (!running) return;
        
        running = false;
        
        for (auto& thread : core_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        core_threads.clear();
    }
    
    int create_process(int priority) {
        // Simulate lock contention
        {
            std::lock_guard<std::mutex> lock(global_lock);
            lock_contentions++;  // Every access causes contention
            
            int pid = next_pid++;
            auto proc = std::make_shared<SMPProcess>(pid, priority);
            
            // Assign to least loaded core (requires scanning shared memory)
            int least_loaded = find_least_loaded_core_locked();
            proc->assigned_core = least_loaded;
            
            global_process_table.push_back(proc);
            
            // Simulate cache invalidation - all cores must reload
            cache_invalidations++;
            
            std::cout << "[SMP] Process " << pid << " created (GLOBAL LOCK held)" << std::endl;
            
            return pid;
        }
    }
    
    void run_workload() {
        std::cout << "\n[SMP] Running workload..." << std::endl;
        
        for (int i = 0; i < 20; i++) {
            create_process(5);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void print_statistics() {
        std::cout << "\n========== SMP SYSTEM STATISTICS ==========" << std::endl;
        std::cout << "  Lock Contentions:     " << lock_contentions << std::endl;
        std::cout << "  Cache Invalidations:  " << cache_invalidations << std::endl;
        std::cout << "  Total Processes:      " << global_process_table.size() << std::endl;
        std::cout << "  Operations:           " << total_operations << std::endl;
        std::cout << "\n  ⚠️  High contention overhead!" << std::endl;
        std::cout << "  ⚠️  Cache coherency cost increases with cores!" << std::endl;
        std::cout << "============================================\n" << std::endl;
    }
    
private:
    void core_worker(int core_id) {
        while (running) {
            // Simulate trying to access shared memory
            {
                std::lock_guard<std::mutex> lock(global_lock);
                lock_contentions++;
                
                // Do some "work" on shared data
                total_operations++;
                
                // Simulate cache invalidation every access
                cache_invalidations++;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    int find_least_loaded_core_locked() {
        // Must be called with lock held
        // In SMP, this requires accessing shared memory
        
        std::vector<int> core_loads(NUM_CORES, 0);
        
        for (const auto& proc : global_process_table) {
            if (proc->assigned_core >= 0 && proc->assigned_core < NUM_CORES) {
                core_loads[proc->assigned_core]++;
            }
        }
        
        int min_core = 0;
        int min_load = core_loads[0];
        
        for (int i = 1; i < NUM_CORES; i++) {
            if (core_loads[i] < min_load) {
                min_load = core_loads[i];
                min_core = i;
            }
        }
        
        return min_core;
    }
};

#endif // SMP_SYSTEM_H
