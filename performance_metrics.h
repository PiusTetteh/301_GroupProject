#include "multikernel.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <cmath>

/**
 * Performance Metrics Collection and Analysis Tool
 * This provides comprehensive metrics for evaluating the multikernel OS
 */

class PerformanceMetrics {
private:
    MultikernelSystem& system;
    std::chrono::steady_clock::time_point start_time;
    
    struct Metrics {
        uint64_t total_processes_created;
        uint64_t total_messages_sent;
        uint64_t total_context_switches;
        double avg_process_creation_time_ms;
        double avg_message_latency_us;
        double throughput_processes_per_sec;
        double core_utilization_pct;
        double load_balance_efficiency;
        double scalability_factor;
    };
    
public:
    PerformanceMetrics(MultikernelSystem& sys) : system(sys) {}
    
    void start_measurement() {
        start_time = std::chrono::steady_clock::now();
    }
    
    Metrics collect_metrics() {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        Metrics m = {};
        
        // NOTE: These are placeholder/example values for demonstration
        // In a production system, these would be collected from the actual
        // MultikernelSystem via getter methods that would need to be added
        // TODO: Add getter methods to MultikernelSystem for real metrics
        m.total_processes_created = 100;
        m.total_messages_sent = 250;
        m.total_context_switches = 500;
        m.avg_process_creation_time_ms = 2.5;
        m.avg_message_latency_us = 15.3;
        
        if (duration.count() > 0) {
            m.throughput_processes_per_sec = 
                (m.total_processes_created * 1000.0) / duration.count();
        }
        
        m.core_utilization_pct = 85.0;
        m.load_balance_efficiency = 92.5;
        m.scalability_factor = 7.2; // Out of 8 cores
        
        return m;
    }
    
    void print_report(const Metrics& m) {
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════╗\n";
        std::cout << "║        MULTIKERNEL OS PERFORMANCE METRICS              ║\n";
        std::cout << "║        (Example/Placeholder Values)                    ║\n";
        std::cout << "╚════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        
        std::cout << "┌─────────────────────────────────────────────────────┐\n";
        std::cout << "│ THROUGHPUT & SCALABILITY                            │\n";
        std::cout << "├─────────────────────────────────────────────────────┤\n";
        std::cout << "│ Total Processes Created: " << std::setw(24) << m.total_processes_created << " │\n";
        std::cout << "│ Process Creation Rate:   " << std::setw(18) << std::fixed << std::setprecision(2) 
                  << m.throughput_processes_per_sec << " procs/sec │\n";
        std::cout << "│ Scalability Factor:      " << std::setw(24) << std::fixed << std::setprecision(2) 
                  << m.scalability_factor << " / 8 │\n";
        std::cout << "└─────────────────────────────────────────────────────┘\n";
        std::cout << "\n";
        
        std::cout << "┌─────────────────────────────────────────────────────┐\n";
        std::cout << "│ MESSAGING & COMMUNICATION                           │\n";
        std::cout << "├─────────────────────────────────────────────────────┤\n";
        std::cout << "│ Total Messages Sent:     " << std::setw(24) << m.total_messages_sent << " │\n";
        std::cout << "│ Avg Message Latency:     " << std::setw(20) << std::fixed << std::setprecision(2) 
                  << m.avg_message_latency_us << " μs │\n";
        std::cout << "│ Msg/Process Ratio:       " << std::setw(24) << std::fixed << std::setprecision(2) 
                  << (double)m.total_messages_sent / m.total_processes_created << " │\n";
        std::cout << "└─────────────────────────────────────────────────────┘\n";
        std::cout << "\n";
        
        std::cout << "┌─────────────────────────────────────────────────────┐\n";
        std::cout << "│ RESOURCE UTILIZATION                                │\n";
        std::cout << "├─────────────────────────────────────────────────────┤\n";
        std::cout << "│ Core Utilization:        " << std::setw(23) << std::fixed << std::setprecision(1) 
                  << m.core_utilization_pct << " % │\n";
        std::cout << "│ Load Balance Efficiency: " << std::setw(23) << std::fixed << std::setprecision(1) 
                  << m.load_balance_efficiency << " % │\n";
        std::cout << "│ Context Switches:        " << std::setw(24) << m.total_context_switches << " │\n";
        std::cout << "└─────────────────────────────────────────────────────┘\n";
        std::cout << "\n";
        
        std::cout << "┌─────────────────────────────────────────────────────┐\n";
        std::cout << "│ PERFORMANCE RATINGS                                 │\n";
        std::cout << "├─────────────────────────────────────────────────────┤\n";
        
        auto rate_performance = [](double value, double threshold) {
            if (value >= threshold) return "★★★★★ EXCELLENT";
            else if (value >= threshold * 0.8) return "★★★★☆ GOOD";
            else if (value >= threshold * 0.6) return "★★★☆☆ FAIR";
            else return "★★☆☆☆ NEEDS IMPROVEMENT";
        };
        
        std::cout << "│ Throughput:              " << std::setw(25) << rate_performance(m.throughput_processes_per_sec, 50) << " │\n";
        std::cout << "│ Message Latency:         " << std::setw(25) << rate_performance(100 - m.avg_message_latency_us, 80) << " │\n";
        std::cout << "│ Load Balancing:          " << std::setw(25) << rate_performance(m.load_balance_efficiency, 90) << " │\n";
        std::cout << "│ Scalability:             " << std::setw(25) << rate_performance(m.scalability_factor, 7.0) << " │\n";
        std::cout << "└─────────────────────────────────────────────────────┘\n";
        std::cout << "\n";
        
        // Overall score
        double overall_score = (
            (m.throughput_processes_per_sec / 50.0) * 25 +
            ((100 - m.avg_message_latency_us) / 80.0) * 25 +
            (m.load_balance_efficiency / 100.0) * 25 +
            (m.scalability_factor / 8.0) * 25
        );
        
        std::cout << "╔════════════════════════════════════════════════════════╗\n";
        std::cout << "║ OVERALL PERFORMANCE SCORE:  " << std::setw(20) << std::fixed << std::setprecision(1) 
                  << overall_score << " / 100    ║\n";
        std::cout << "╚════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
    
    void save_to_csv(const Metrics& m, const std::string& filename) {
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << " for writing\n";
            return;
        }
        
        file << "Metric,Value\n";
        file << "Total Processes Created," << m.total_processes_created << "\n";
        file << "Total Messages Sent," << m.total_messages_sent << "\n";
        file << "Total Context Switches," << m.total_context_switches << "\n";
        file << "Avg Process Creation Time (ms)," << m.avg_process_creation_time_ms << "\n";
        file << "Avg Message Latency (μs)," << m.avg_message_latency_us << "\n";
        file << "Throughput (procs/sec)," << m.throughput_processes_per_sec << "\n";
        file << "Core Utilization (%)," << m.core_utilization_pct << "\n";
        file << "Load Balance Efficiency (%)," << m.load_balance_efficiency << "\n";
        file << "Scalability Factor," << m.scalability_factor << "\n";
        
        if (!file.good()) {
            std::cerr << "Error: Write operation failed for " << filename << "\n";
            file.close();
            return;
        }
        
        file.close();
        std::cout << "Metrics saved to " << filename << "\n";
    }
};

// Benchmark runner
void run_performance_benchmark(MultikernelSystem& system) {
    PerformanceMetrics metrics(system);
    
    std::cout << "\n╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║        RUNNING PERFORMANCE BENCHMARK                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n\n";
    
    metrics.start_measurement();
    
    // Run workload
    std::cout << "Creating workload...\n";
    for (int i = 0; i < 100; i++) {
        system.create_process((i % 10) + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "Running load balancer...\n";
    system.balance_load();
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Collect and report
    auto m = metrics.collect_metrics();
    metrics.print_report(m);
    metrics.save_to_csv(m, "performance_metrics.csv");
}
