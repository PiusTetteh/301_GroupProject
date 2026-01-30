#include "multikernel.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

// ============================================================================
// DEMONSTRATION SCENARIOS
// ============================================================================

void demo_basic_operation(MultikernelSystem& system) {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO 1: Basic Process Creation               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\nCreating 8 processes with different priorities..." << std::endl;

    for (int i = 0; i < 8; i++) {
        int priority = (i % 10) + 1;
        system.create_process(priority);
        std::this_thread::sleep_for(100ms);
    }

    std::cout << "\n✓ Processes created and distributed across cores" << std::endl;
    std::this_thread::sleep_for(1s);
}

void demo_message_passing(MultikernelSystem& system) {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO 2: Inter-Core Message Passing           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\nDemonstrating message-based communication..." << std::endl;
    std::cout << "Creating processes that trigger inter-core messages..." << std::endl;

    for (int i = 0; i < 12; i++) {
        system.create_process(5);
        std::this_thread::sleep_for(50ms);
    }

    std::cout << "\n✓ Messages exchanged between cores for load distribution" << std::endl;
    std::this_thread::sleep_for(1500ms);
}

void demo_load_balancing(MultikernelSystem& system) {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO 3: Dynamic Load Balancing                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\nCreating moderate load across cores..." << std::endl;

    // Create only 10 processes with consistent timing
    for (int i = 0; i < 10; i++) {
        system.create_process(5);
        std::this_thread::sleep_for(80ms);
    }

    std::cout << "\nAllowing processes to accumulate briefly..." << std::endl;
    std::this_thread::sleep_for(500ms); // Let some load build up

    std::cout << "Running load balancer..." << std::endl;
    system.balance_load();

    std::cout << "\n✓ Load balanced across cores using NUMA-aware algorithms" << std::endl;
    std::this_thread::sleep_for(800ms);
}

void demo_scalability(MultikernelSystem& system) {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO 4: Scalability Test                      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\nCreating high process count to demonstrate scalability..." << std::endl;

    auto start = std::chrono::steady_clock::now();

    // Create 20 processes quickly
    for (int i = 0; i < 20; i++) {
        system.create_process((i % 10) + 1);
        std::this_thread::sleep_for(40ms);
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n✓ Created 20 processes in " << duration.count() << "ms" << std::endl;
    std::cout << "✓ Demonstrates multikernel scalability advantages" << std::endl;

    std::this_thread::sleep_for(1000ms);
}

// ============================================================================
// NEW MESSAGE-PASSING DEMONSTRATIONS
// ============================================================================

void demo_explicit_migration(MultikernelSystem& system) {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO: Explicit Process Migration Messages    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\nCreating processes on specific cores and migrating them..." << std::endl;
    
    // Create several processes
    std::vector<int> pids;
    for (int i = 0; i < 6; i++) {
        int pid = system.create_process(5);
        pids.push_back(pid);
        std::this_thread::sleep_for(100ms);
    }
    
    std::cout << "\nNow migrating processes between cores..." << std::endl;
    std::this_thread::sleep_for(500ms);
    
    // Trigger migrations which send messages
    for (size_t i = 0; i < pids.size() && i < 3; i++) {
        int source = i % 8;
        int target = (i + 4) % 8;
        std::cout << "\n[MIGRATION] Attempting to migrate PID " << pids[i] 
                  << " from Core " << source << " to Core " << target << std::endl;
        system.migrate_process(pids[i], source, target);
        std::this_thread::sleep_for(300ms);
    }
    
    std::cout << "\n✓ Process migrations completed - check message counts!" << std::endl;
    std::this_thread::sleep_for(1s);
}

void demo_heartbeat_messages(MultikernelSystem& system) {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO: Core Heartbeat Messages                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\nSending heartbeat messages between cores..." << std::endl;
    std::cout << "Core 0 will ping all other cores...\n" << std::endl;
    
    system.send_heartbeat_messages();
    
    std::this_thread::sleep_for(1s);
    std::cout << "\n✓ Heartbeat messages sent and received!" << std::endl;
    std::this_thread::sleep_for(500ms);
}

void demo_resource_contention(MultikernelSystem& system) {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO: Resource Request/Release Messages      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\nSimulating resource contention across cores..." << std::endl;
    std::cout << "Multiple cores requesting shared resources via messages...\n" << std::endl;
    
    system.demo_resource_messages();
    
    std::this_thread::sleep_for(1s);
    std::cout << "\n✓ Resource management messages exchanged!" << std::endl;
    std::this_thread::sleep_for(500ms);
}

void demo_comparison_with_smp() {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DEMO 5: Multikernel vs Traditional SMP       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\n--- Traditional SMP Approach ---" << std::endl;
    std::cout << "  ✗ Shared memory across all cores" << std::endl;
    std::cout << "  ✗ Cache coherency overhead increases with cores" << std::endl;
    std::cout << "  ✗ Lock contention becomes bottleneck" << std::endl;
    std::cout << "  ✗ Poor NUMA performance" << std::endl;

    std::cout << "\n--- Multikernel Approach (This System) ---" << std::endl;
    std::cout << "  ✓ Message-passing: no shared memory" << std::endl;
    std::cout << "  ✓ Per-core OS instances: minimal cache coherency" << std::endl;
    std::cout << "  ✓ No global locks: lock-free message queues" << std::endl;
    std::cout << "  ✓ NUMA-aware: processes stay local" << std::endl;
    std::cout << "  ✓ Scales linearly with core count" << std::endl;

    std::this_thread::sleep_for(2500ms);
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

int main() {
    std::cout << R"(
    ╔══════════════════════════════════════════════════════════╗
    ║                                                          ║
    ║       MULTIKERNEL OPERATING SYSTEM                       ║
    ║       University of Ghana - DCIT301                      ║
    ║       Group 54                                           ║
    ║                                                          ║
    ║   Treating Multi-Core Systems as Distributed Systems    ║
    ║                                                          ║
    ╚══════════════════════════════════════════════════════════╝
    )" << std::endl;

    std::cout << "\nInitializing system..." << std::endl;

    // Create and start the multikernel system
    MultikernelSystem system;
    system.start();

    try {
        // Run demonstrations
        demo_basic_operation(system);
        system.print_statistics();

        demo_message_passing(system);
        system.print_statistics();
        
        // NEW: Explicit message-passing demos
        demo_explicit_migration(system);
        system.print_statistics();
        
        demo_heartbeat_messages(system);
        system.print_statistics();
        
        demo_resource_contention(system);
        system.print_statistics();

        demo_load_balancing(system);
        system.print_statistics();

        demo_scalability(system);
        system.print_statistics();

        demo_comparison_with_smp();

        // Final statistics
        system.print_statistics();

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
    }

    std::cout << "\n\nShutting down system..." << std::endl;

    // Shutdown
    system.shutdown();

    std::cout << R"(
    ╔══════════════════════════════════════════════════════════╗
    ║                                                          ║
    ║   DEMONSTRATION COMPLETE                                 ║
    ║                                                          ║
    ║   Key Features Demonstrated:                             ║
    ║   ✓ Per-core OS instances                                ║
    ║   ✓ Message-passing communication                        ║
    ║   ✓ NUMA-aware load balancing                            ║
    ║   ✓ Distributed system algorithms                        ║
    ║   ✓ Scalability advantages over SMP                      ║
    ║                                                          ║
    ╚══════════════════════════════════════════════════════════╝
    )" << std::endl;

    return 0;
}