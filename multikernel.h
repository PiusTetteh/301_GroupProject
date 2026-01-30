#ifndef MULTIKERNEL_H
#define MULTIKERNEL_H

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <functional>
#include <cstring>
#include <cstdint>

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================
const int NUM_CORES = 8;                    // Number of cores in the system
const int MAX_MESSAGE_SIZE = 512;           // Maximum message payload size
const int MESSAGE_QUEUE_SIZE = 100;         // Max messages per core queue
const int MAX_PROCESSES = 64;               // Maximum processes system-wide

// ============================================================================
// MESSAGE TYPES - Inter-core communication protocol
// ============================================================================
enum MessageType {
    MSG_PROCESS_CREATE,      // Request to create new process
    MSG_PROCESS_MIGRATE,     // Migrate process to another core
    MSG_PROCESS_TERMINATE,   // Terminate a process
    MSG_RESOURCE_REQUEST,    // Request shared resource
    MSG_RESOURCE_RELEASE,    // Release shared resource
    MSG_SYNC_BARRIER,        // Synchronization barrier
    MSG_HEARTBEAT,           // Core health check
    MSG_SHUTDOWN             // Shutdown signal
};

// ============================================================================
// MESSAGE STRUCTURE - Core communication packet
// ============================================================================
struct Message {
    int source_core;                    // Sender core ID
    int dest_core;                      // Destination core ID (-1 for broadcast)
    MessageType type;                   // Message type
    int process_id;                     // Related process ID
    char data[MAX_MESSAGE_SIZE];        // Payload data
    std::chrono::steady_clock::time_point timestamp;  // For latency tracking
    
    Message() : source_core(-1), dest_core(-1), type(MSG_HEARTBEAT), 
                process_id(-1), timestamp(std::chrono::steady_clock::now()) {
        memset(data, 0, MAX_MESSAGE_SIZE);
    }
};

// ============================================================================
// PROCESS CONTROL BLOCK - Per-process metadata
// ============================================================================
enum ProcessState {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
};

struct ProcessControlBlock {
    int pid;                            // Process ID
    int core_id;                        // Currently assigned core
    ProcessState state;                 // Current state
    int priority;                       // Scheduling priority (0-10)
    std::chrono::steady_clock::time_point creation_time;
    std::chrono::milliseconds cpu_time; // Total CPU time used
    
    ProcessControlBlock(int id, int core, int prio = 5) 
        : pid(id), core_id(core), state(PROCESS_READY), 
          priority(prio), creation_time(std::chrono::steady_clock::now()),
          cpu_time(0) {}
};

// ============================================================================
// STATISTICS - Performance monitoring
// ============================================================================
struct CoreStatistics {
    std::atomic<uint64_t> messages_sent{0};
    std::atomic<uint64_t> messages_received{0};
    std::atomic<uint64_t> processes_executed{0};
    std::atomic<uint64_t> context_switches{0};
    std::atomic<int64_t> avg_message_latency_us{0};
    std::atomic<int> current_load{0};  // Number of active processes

    CoreStatistics() = default;
    CoreStatistics(const CoreStatistics& other) {
        messages_sent.store(other.messages_sent.load());
        messages_received.store(other.messages_received.load());
        processes_executed.store(other.processes_executed.load());
        context_switches.store(other.context_switches.load());
        avg_message_latency_us.store(other.avg_message_latency_us.load());
        current_load.store(other.current_load.load());
    }
    CoreStatistics& operator=(const CoreStatistics& other) {
        if (this != &other) {
            messages_sent.store(other.messages_sent.load());
            messages_received.store(other.messages_received.load());
            processes_executed.store(other.processes_executed.load());
            context_switches.store(other.context_switches.load());
            avg_message_latency_us.store(other.avg_message_latency_us.load());
            current_load.store(other.current_load.load());
        }
        return *this;
    }
};

// ============================================================================
// CORE KERNEL - Per-core OS instance
// ============================================================================
class CoreKernel {
private:
    int core_id;
    std::atomic<bool> running;
    
    // Message passing infrastructure
    std::queue<Message> inbox;
    std::mutex inbox_mutex;
    std::condition_variable inbox_cv;
    
    // Process management
    std::vector<std::shared_ptr<ProcessControlBlock>> process_table;
    std::mutex process_mutex;
    
    // Statistics
    CoreStatistics stats;
    
    // Worker thread
    std::thread worker_thread;
    
    // Reference to other cores for message routing
    std::vector<CoreKernel*>* all_cores;
    
public:
    CoreKernel(int id);
    ~CoreKernel();
    
    // Lifecycle management
    void start(std::vector<CoreKernel*>* cores);
    void stop();
    bool is_running() const { return running; }
    
    // Message passing
    void send_message(const Message& msg);
    bool receive_message(Message& msg, int timeout_ms = 0);
    void broadcast_message(const Message& msg);
    
    // Process management
    int create_process(int priority = 5);
    bool migrate_process(int pid, int target_core);
    void terminate_process(int pid);
    
    // Statistics and monitoring
    CoreStatistics get_statistics() const { return stats; }
    int get_load() const { return stats.current_load; }
    int get_core_id() const { return core_id; }
    
private:
    void worker_loop();
    void process_message(const Message& msg);
    void execute_processes();
    void handle_process_create(const Message& msg);
    void handle_process_migrate(const Message& msg);
    void handle_process_terminate(const Message& msg);
};

// ============================================================================
// MULTIKERNEL SYSTEM - System coordinator
// ============================================================================
class MultikernelSystem {
private:
    std::vector<std::unique_ptr<CoreKernel>> cores;
    std::vector<CoreKernel*> core_ptrs;  // Persistent pointers for inter-core communication
    std::atomic<int> next_pid{0};
    std::atomic<bool> system_running{false};
    
    // Load balancing
    std::mutex load_balancer_mutex;
    
public:
    MultikernelSystem();
    ~MultikernelSystem();
    
    // System lifecycle
    void start();
    void shutdown();
    
    // Process management (delegates to least loaded core)
    int create_process(int priority = 5);
    bool migrate_process(int pid, int source_core, int target_core);
    
    // Load balancing
    void balance_load();
    int get_least_loaded_core();
    
    // System-wide statistics
    void print_statistics();
    float get_comm_overhead_pct() const;
    
    // Message-passing demonstrations
    void send_heartbeat_messages();
    void demo_resource_messages();
    
private:
    void load_balancer_thread();
};

#endif // MULTIKERNEL_H
