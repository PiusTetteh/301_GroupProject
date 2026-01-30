#include "multikernel.h"
#include <algorithm>
#include <random>

// ============================================================================
// CORE KERNEL IMPLEMENTATION
// ============================================================================

CoreKernel::CoreKernel(int id) 
    : core_id(id), running(false), all_cores(nullptr) {
    process_table.reserve(MAX_PROCESSES / NUM_CORES);
}

CoreKernel::~CoreKernel() {
    stop();
}

void CoreKernel::start(std::vector<CoreKernel*>* cores) {
    if (running) return;
    
    all_cores = cores;
    running = true;
    
    // Launch worker thread for this core
    worker_thread = std::thread(&CoreKernel::worker_loop, this);
    
    std::cout << "[Core " << core_id << "] Started successfully" << std::endl;
}

void CoreKernel::stop() {
    if (!running) return;
    
    running = false;
    inbox_cv.notify_all();
    
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    
    std::cout << "[Core " << core_id << "] Stopped" << std::endl;
}

// ============================================================================
// MESSAGE PASSING - Inter-core communication
// ============================================================================

void CoreKernel::send_message(const Message& msg) {
    if (msg.dest_core < 0 || msg.dest_core >= NUM_CORES) {
        std::cerr << "[Core " << core_id << "] Invalid destination core: " 
                  << msg.dest_core << std::endl;
        return;
    }
    
    if (!all_cores || msg.dest_core >= static_cast<int>(all_cores->size())) {
        std::cerr << "[Core " << core_id << "] Core system not initialized" << std::endl;
        return;
    }
    
    // Route message to destination core
    CoreKernel* dest = (*all_cores)[msg.dest_core];
    if (dest) {
        std::unique_lock<std::mutex> lock(dest->inbox_mutex);
        
        // Check queue size to prevent overflow
        if (dest->inbox.size() >= MESSAGE_QUEUE_SIZE) {
            std::cerr << "[Core " << core_id << "] Destination queue full" << std::endl;
            return;
        }
        
        dest->inbox.push(msg);
        dest->inbox_cv.notify_one();
        
        stats.messages_sent++;
    }
}

bool CoreKernel::receive_message(Message& msg, int timeout_ms) {
    std::unique_lock<std::mutex> lock(inbox_mutex);
    
    if (timeout_ms > 0) {
        // Wait with timeout
        if (inbox_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                              [this] { return !inbox.empty() || !running; })) {
            if (!inbox.empty()) {
                msg = inbox.front();
                inbox.pop();
                stats.messages_received++;
                
                // Calculate latency
                auto now = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    now - msg.timestamp);
                stats.avg_message_latency_us.store(latency.count()); 
                
                return true;
            }
        }
    } else {
        // Non-blocking check
        if (!inbox.empty()) {
            msg = inbox.front();
            inbox.pop();
            stats.messages_received++;
            return true;
        }
    }
    
    return false;
}

void CoreKernel::broadcast_message(const Message& msg) {
    for (int i = 0; i < NUM_CORES; i++) {
        if (i != core_id) {
            Message broadcast_msg = msg;
            broadcast_msg.dest_core = i;
            send_message(broadcast_msg);
        }
    }
}

// ============================================================================
// PROCESS MANAGEMENT
// ============================================================================

int CoreKernel::create_process(int priority) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    static std::atomic<int> global_pid{0};
    int pid = global_pid++;
    
    auto pcb = std::make_shared<ProcessControlBlock>(pid, core_id, priority);
    process_table.push_back(pcb);
    
    stats.current_load++;
    
    std::cout << "[Core " << core_id << "] Created process " << pid 
              << " (priority=" << priority << ")" << std::endl;
    
    return pid;
}

bool CoreKernel::migrate_process(int pid, int target_core) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    // Find process
    auto it = std::find_if(process_table.begin(), process_table.end(),
                          [pid](const auto& pcb) { return pcb->pid == pid; });
    
    if (it == process_table.end()) {
        return false;
    }
    
    // Create migration message
    Message msg;
    msg.source_core = core_id;
    msg.dest_core = target_core;
    msg.type = MSG_PROCESS_MIGRATE;
    msg.process_id = pid;
    
    // Copy process data to message
    snprintf(msg.data, MAX_MESSAGE_SIZE, "priority=%d", (*it)->priority);
    
    send_message(msg);
    
    // Remove from local table
    process_table.erase(it);
    stats.current_load--;
    
    std::cout << "[Core " << core_id << "] Migrated process " << pid 
              << " to Core " << target_core << std::endl;
    
    return true;
}

void CoreKernel::terminate_process(int pid) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    auto it = std::find_if(process_table.begin(), process_table.end(),
                          [pid](const auto& pcb) { return pcb->pid == pid; });
    
    if (it != process_table.end()) {
        (*it)->state = PROCESS_TERMINATED;
        process_table.erase(it);
        stats.current_load--;
        
        std::cout << "[Core " << core_id << "] Terminated process " << pid << std::endl;
    }
}

// ============================================================================
// WORKER LOOP - Main execution loop for the core
// ============================================================================

void CoreKernel::worker_loop() {
    std::cout << "[Core " << core_id << "] Worker thread started" << std::endl;
    
    while (running) {
        // Process incoming messages
        Message msg;
        while (receive_message(msg, 0)) {
            process_message(msg);
        }
        
        // Execute processes on this core
        execute_processes();
        
        // CRITICAL: Increased sleep to reduce CPU usage and prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "[Core " << core_id << "] Worker thread stopped" << std::endl;
}

void CoreKernel::process_message(const Message& msg) {
    switch (msg.type) {
        case MSG_PROCESS_CREATE:
            handle_process_create(msg);
            break;

        case MSG_PROCESS_MIGRATE:
            handle_process_migrate(msg);
            break;

        case MSG_PROCESS_TERMINATE:
            handle_process_terminate(msg);
            break;

        case MSG_HEARTBEAT:
            // Heartbeat received - core is alive
            break;

        case MSG_SHUTDOWN:
            running = false;
            break;

        default:
            std::cout << "[Core " << core_id << "] Unknown message type: "
                      << msg.type << std::endl;
    }
}

void CoreKernel::handle_process_create(const Message& msg) {
    int priority = 5; // Default priority
    sscanf(msg.data, "priority=%d", &priority);
    create_process(priority);
}

void CoreKernel::handle_process_migrate(const Message& msg) {
    std::lock_guard<std::mutex> lock(process_mutex);

    int priority = 5;
    sscanf(msg.data, "priority=%d", &priority);

    // Receive migrated process
    auto pcb = std::make_shared<ProcessControlBlock>(msg.process_id, core_id, priority);
    process_table.push_back(pcb);
    stats.current_load++;

    std::cout << "[Core " << core_id << "] Received migrated process "
              << msg.process_id << std::endl;
}

void CoreKernel::handle_process_terminate(const Message& msg) {
    terminate_process(msg.process_id);
}

void CoreKernel::execute_processes() {
    std::lock_guard<std::mutex> lock(process_mutex);

    // VERY AGGRESSIVE TERMINATION for fast demos
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 100);

    for (auto& pcb : process_table) {
        if (pcb->state == PROCESS_READY || pcb->state == PROCESS_RUNNING) {
            pcb->state = PROCESS_RUNNING;

            // Simulate process execution
            pcb->cpu_time += std::chrono::milliseconds(50);
            stats.processes_executed++;
            stats.context_switches++;

            // EXTREMELY AGGRESSIVE TERMINATION
            auto cpu_ms = std::chrono::duration_cast<std::chrono::milliseconds>(pcb->cpu_time).count();
            int termination_threshold;

            if (cpu_ms > 600) {
                termination_threshold = 20; // 80% chance after 600ms
            } else if (cpu_ms > 300) {
                termination_threshold = 50; // 50% chance after 300ms
            } else if (cpu_ms > 150) {
                termination_threshold = 70; // 30% chance after 150ms
            } else {
                termination_threshold = 80; // 20% chance for young processes
            }

            if (dis(gen) > termination_threshold) {
                pcb->state = PROCESS_TERMINATED;
            }
        }
    }

    // Remove terminated processes
    auto old_size = process_table.size();
    process_table.erase(
        std::remove_if(process_table.begin(), process_table.end(),
                      [](const auto& pcb) { return pcb->state == PROCESS_TERMINATED; }),
        process_table.end()
    );

    stats.current_load = process_table.size();

    // Only log if processes were actually terminated
    auto terminated_count = old_size - process_table.size();
    if (terminated_count > 0) {
        std::cout << "[Core " << core_id << "] Terminated " << terminated_count
                  << " processes (load now: " << stats.current_load << ")" << std::endl;
    }
}