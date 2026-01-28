#include "multikernel.h"
#include <iomanip>
#include <climits>

// ============================================================================
// MULTIKERNEL SYSTEM IMPLEMENTATION
// ============================================================================

MultikernelSystem::MultikernelSystem() {
    // Create per-core kernel instances
    cores.reserve(NUM_CORES);
    for (int i = 0; i < NUM_CORES; i++) {
        cores.push_back(std::make_unique<CoreKernel>(i));
    }
    
    std::cout << "==================================================" << std::endl;
    std::cout << "  MULTIKERNEL OPERATING SYSTEM INITIALIZED" << std::endl;
    std::cout << "  Cores: " << NUM_CORES << std::endl;
    std::cout << "  Message Queue Size: " << MESSAGE_QUEUE_SIZE << std::endl;
    std::cout << "  Max Processes: " << MAX_PROCESSES << std::endl;
    std::cout << "==================================================" << std::endl;
}

MultikernelSystem::~MultikernelSystem() {
    shutdown();
}

void MultikernelSystem::start() {
    if (system_running) {
        std::cout << "System already running!" << std::endl;
        return;
    }
    
    system_running = true;
    
    // Create vector of core pointers for inter-core communication
    std::vector<CoreKernel*> core_ptrs;
    for (auto& core : cores) {
        core_ptrs.push_back(core.get());
    }
    
    // Start all cores
    for (auto& core : cores) {
        core->start(&core_ptrs);
    }
    
    std::cout << "\n[SYSTEM] All cores started successfully" << std::endl;
    std::cout << "[SYSTEM] Message-passing infrastructure active" << std::endl;
    std::cout << "[SYSTEM] Ready for process creation\n" << std::endl;
}

void MultikernelSystem::shutdown() {
    if (!system_running) return;
    
    std::cout << "\n[SYSTEM] Initiating shutdown..." << std::endl;
    
    // Send shutdown messages to all cores
    Message shutdown_msg;
    shutdown_msg.type = MSG_SHUTDOWN;
    shutdown_msg.source_core = -1; // System message
    
    for (int i = 0; i < NUM_CORES; i++) {
        shutdown_msg.dest_core = i;
        cores[i]->send_message(shutdown_msg);
    }
    
    // Stop all cores
    for (auto& core : cores) {
        core->stop();
    }
    
    system_running = false;
    std::cout << "[SYSTEM] Shutdown complete" << std::endl;
}

// ============================================================================
// PROCESS MANAGEMENT WITH LOAD BALANCING
// ============================================================================

int MultikernelSystem::create_process(int priority) {
    if (!system_running) {
        std::cerr << "[SYSTEM] Cannot create process: system not running" << std::endl;
        return -1;
    }
    
    // Find least loaded core (NUMA-aware load balancing)
    int target_core = get_least_loaded_core();
    
    // Create process on that core
    int pid = cores[target_core]->create_process(priority);
    
    std::cout << "[SYSTEM] Process " << pid << " assigned to Core " 
              << target_core << " (load=" << cores[target_core]->get_load() 
              << ")" << std::endl;
    
    return pid;
}

bool MultikernelSystem::migrate_process(int pid, int source_core, int target_core) {
    if (source_core < 0 || source_core >= NUM_CORES ||
        target_core < 0 || target_core >= NUM_CORES) {
        std::cerr << "[SYSTEM] Invalid core IDs for migration" << std::endl;
        return false;
    }
    
    return cores[source_core]->migrate_process(pid, target_core);
}

// ============================================================================
// LOAD BALANCING - NUMA-aware distribution
// ============================================================================

int MultikernelSystem::get_least_loaded_core() {
    std::lock_guard<std::mutex> lock(load_balancer_mutex);
    
    int min_load = INT_MAX;
    int best_core = 0;
    
    for (int i = 0; i < NUM_CORES; i++) {
        int load = cores[i]->get_load();
        if (load < min_load) {
            min_load = load;
            best_core = i;
        }
    }
    
    return best_core;
}

void MultikernelSystem::balance_load() {
    std::lock_guard<std::mutex> lock(load_balancer_mutex);
    
    // Calculate average load
    int total_load = 0;
    for (const auto& core : cores) {
        total_load += core->get_load();
    }
    
    if (total_load == 0) return;
    
    float avg_load = static_cast<float>(total_load) / NUM_CORES;
    
    std::cout << "\n[LOAD BALANCER] Average load: " << avg_load << std::endl;
    
    // Find overloaded and underloaded cores
    for (int i = 0; i < NUM_CORES; i++) {
        int load = cores[i]->get_load();
        
        if (load > avg_load * 1.5) { // Core is overloaded
            std::cout << "[LOAD BALANCER] Core " << i << " overloaded (load=" 
                      << load << ")" << std::endl;
            
            // Find underloaded core
            int target = get_least_loaded_core();
            if (target != i && cores[target]->get_load() < avg_load * 0.7) {
                std::cout << "[LOAD BALANCER] Would migrate process from Core " 
                          << i << " to Core " << target << std::endl;
                // In a real implementation, we'd migrate a process here
            }
        }
    }
}

// ============================================================================
// STATISTICS AND MONITORING
// ============================================================================

void MultikernelSystem::print_statistics() {
    std::cout << "\n";
    std::cout << "========================================================" << std::endl;
    std::cout << "           MULTIKERNEL OS STATISTICS" << std::endl;
    std::cout << "========================================================" << std::endl;
    
    // Per-core statistics
    for (int i = 0; i < NUM_CORES; i++) {
        auto stats = cores[i]->get_statistics();
        
        std::cout << "\n--- Core " << i << " ---" << std::endl;
        std::cout << "  Current Load:      " << stats.current_load << " processes" << std::endl;
        std::cout << "  Messages Sent:     " << stats.messages_sent << std::endl;
        std::cout << "  Messages Received: " << stats.messages_received << std::endl;
        std::cout << "  Processes Executed:" << stats.processes_executed << std::endl;
        std::cout << "  Context Switches:  " << stats.context_switches << std::endl;
        std::cout << "  Avg Msg Latency:   " << stats.avg_message_latency_us.load() 
                  << " μs" << std::endl;
    }
    
    // System-wide statistics
    uint64_t total_messages_sent = 0;
    uint64_t total_messages_received = 0;
    uint64_t total_processes = 0;
    uint64_t total_context_switches = 0;
    
    for (const auto& core : cores) {
        auto stats = core->get_statistics();
        total_messages_sent += stats.messages_sent;
        total_messages_received += stats.messages_received;
        total_processes += stats.processes_executed;
        total_context_switches += stats.context_switches;
    }
    
    std::cout << "\n--- System Totals ---" << std::endl;
    std::cout << "  Total Messages Sent:     " << total_messages_sent << std::endl;
    std::cout << "  Total Messages Received: " << total_messages_received << std::endl;
    std::cout << "  Total Processes Executed:" << total_processes << std::endl;
    std::cout << "  Total Context Switches:  " << total_context_switches << std::endl;
    
    // Calculate message throughput
    if (total_messages_sent > 0) {
        float message_efficiency = (static_cast<float>(total_messages_received) / 
                                   total_messages_sent) * 100.0f;
        std::cout << "  Message Delivery Rate:   " << std::fixed 
                  << std::setprecision(2) << message_efficiency << "%" << std::endl;
    }
    
    std::cout << "========================================================\n" << std::endl;
}
