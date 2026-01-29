/*
 * Multikernel OS Simulation - Messaging & Distributed Algorithms
 * Role: Messaging Developer
 * Language: C++ (Refactored from C)
 */

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <atomic>
#include <iomanip>

// ==========================================
// 1. CONFIGURATION & CONSTANTS
// ==========================================
constexpr int NUM_CORES = 4;
constexpr int QUEUE_SIZE = 256;
constexpr int PAYLOAD_SIZE = 64;

// Message Types
enum class MsgType {
    PING,
    ACK,
    BARRIER_REACHED,
    BARRIER_GO,
    ELECTION_VOTE,
    ELECTION_WINNER,
    SHUTDOWN
};

// ==========================================
// 2. DATA STRUCTURES
// ==========================================

// The standard message packet
struct Message {
    int src_core_id;
    int dest_core_id;
    MsgType type;
    uint32_t sequence_num;
    char payload[PAYLOAD_SIZE];
    uint64_t timestamp;

    Message() : src_core_id(-1), dest_core_id(-1), type(MsgType::PING), sequence_num(0), timestamp(0) {
        std::memset(payload, 0, PAYLOAD_SIZE);
    }
};

// Performance Counters
struct PerfCounters {
    std::atomic<uint64_t> messages_sent{0};
    std::atomic<uint64_t> messages_received{0};
    std::atomic<uint64_t> total_latency_ns{0};
};

// Forward declaration
class Core;

// Global pointer to cores to allow inter-core communication
// In a real OS, this represents the physical interconnect address space
Core* system_cores[NUM_CORES];

// ==========================================
// 3. CLASSES
// ==========================================

// A Thread-Safe Ring Buffer Class
class MessageQueue {
private:
    Message buffer[QUEUE_SIZE];
    int head = 0;
    int tail = 0;
    std::mutex mtx;
    std::condition_variable cv;

public:
    // Pushes a message into the queue (Non-blocking if full, for simulation simplicity)
    bool push(const Message& msg) {
        std::lock_guard<std::mutex> lock(mtx);
        
        int next_tail = (tail + 1) % QUEUE_SIZE;
        if (next_tail == head) {
            return false; // Queue full
        }

        buffer[tail] = msg;
        tail = next_tail;
        
        cv.notify_one(); // Wake up the receiver
        return true;
    }

    // Pops a message (Blocking wait)
    Message pop(bool& running_flag) {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Wait while buffer is empty AND the core is still meant to be running
        while (head == tail && running_flag) {
            cv.wait(lock);
        }

        if (!running_flag && head == tail) {
            // Return a shutdown message if we are exiting
            Message exit_msg;
            exit_msg.type = MsgType::SHUTDOWN;
            return exit_msg;
        }

        Message msg = buffer[head];
        head = (head + 1) % QUEUE_SIZE;
        return msg;
    }

    // Force wake up for shutdown
    void wake_all() {
        std::lock_guard<std::mutex> lock(mtx);
        cv.notify_all();
    }
};

class Core {
public:
    int id;
    MessageQueue inbox;
    PerfCounters stats;
    int current_leader = -1;
    bool running = true;
    std::thread core_thread;

    Core(int core_id) : id(core_id) {}

    // --- Helper: Get Time ---
    uint64_t get_time_ns() {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    }

    // --- API: Send Message ---
    bool send_message(int dest_id, MsgType type, const std::string& data = "") {
        if (dest_id < 0 || dest_id >= NUM_CORES) return false;

        Message msg;
        msg.src_core_id = id;
        msg.dest_core_id = dest_id;
        msg.type = type;
        msg.sequence_num = stats.messages_sent;
        msg.timestamp = get_time_ns();
        
        if (!data.empty()) {
            std::strncpy(msg.payload, data.c_str(), PAYLOAD_SIZE - 1);
        }

        // Access global system interconnect to find destination
        bool success = system_cores[dest_id]->inbox.push(msg);
        
        if (success) stats.messages_sent++;
        return success;
    }

    // --- API: Receive Message ---
    Message receive_message() {
        Message msg = inbox.pop(running);

        if (msg.type != MsgType::SHUTDOWN) {
            stats.messages_received++;
            uint64_t now = get_time_ns();
            if (now > msg.timestamp) {
                stats.total_latency_ns += (now - msg.timestamp);
            }
        }
        return msg;
    }

    // --- Algorithm: Distributed Barrier ---
    void enter_barrier() {
        // Use a localized string stream for thread-safe printing
        {
            std::lock_guard<std::mutex> cout_lock(print_mutex);
            std::cout << "[Core " << id << "] Entering Barrier..." << std::endl;
        }

        if (id == 0) {
            // Coordinator Logic
            int cores_arrived = 1; // Count self
            while (cores_arrived < NUM_CORES) {
                Message msg = receive_message();
                if (msg.type == MsgType::BARRIER_REACHED) {
                    cores_arrived++;
                }
            }
            
            {
                std::lock_guard<std::mutex> cout_lock(print_mutex);
                std::cout << "[Core 0] All cores arrived. Releasing Barrier." << std::endl;
            }

            // Broadcast GO
            for (int i = 1; i < NUM_CORES; i++) {
                send_message(i, MsgType::BARRIER_GO, "GO");
            }
        } else {
            // Worker Logic
            send_message(0, MsgType::BARRIER_REACHED);
            while (true) {
                Message msg = receive_message();
                if (msg.type == MsgType::BARRIER_GO) break;
            }
        }
        
        {
            std::lock_guard<std::mutex> cout_lock(print_mutex);
            std::cout << "[Core " << id << "] Passed Barrier." << std::endl;
        }
    }

    // --- Algorithm: Leader Election ---
    void start_election() {
        {
            std::lock_guard<std::mutex> cout_lock(print_mutex);
            std::cout << "[Core " << id << "] Starting Election." << std::endl;
        }

        int higher_cores = 0;
        for (int i = id + 1; i < NUM_CORES; i++) {
            send_message(i, MsgType::ELECTION_VOTE);
            higher_cores++;
        }

        if (higher_cores == 0) {
            // I am the winner
            current_leader = id;
            {
                std::lock_guard<std::mutex> cout_lock(print_mutex);
                std::cout << "[Core " << id << "] I am the new Leader!" << std::endl;
            }
            for (int i = 0; i < NUM_CORES; i++) {
                if (i != id) send_message(i, MsgType::ELECTION_WINNER);
            }
        }
    }

    // --- Main Core Loop ---
    void run() {
        // Simulating boot delay
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));

        // 1. Barrier Sync
        enter_barrier();

        // 2. Trigger Election (Simulation: Core 1 starts it)
        if (id == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            start_election();
        }

        // 3. Message Processing Loop
        while (running) {
            Message msg = receive_message();
            
            switch (msg.type) {
                case MsgType::SHUTDOWN:
                    running = false;
                    break;
                case MsgType::ELECTION_WINNER:
                    current_leader = msg.src_core_id;
                    {
                        std::lock_guard<std::mutex> cout_lock(print_mutex);
                        std::cout << "[Core " << id << "] Acknowledged Leader: Core " << msg.src_core_id << std::endl;
                    }
                    break;
                case MsgType::ELECTION_VOTE:
                    // In a full Bully algo, we would respond here. 
                    // Simplified: Higher ID ignores lower ID votes essentially blocking them.
                    break;
                default:
                    break;
            }
        }
    }

    // Static mutex for clean console printing across threads
    static std::mutex print_mutex;
};

std::mutex Core::print_mutex;

// ==========================================
// 4. MAIN SIMULATION
// ==========================================

int main() {
    std::srand(std::time(nullptr));

    std::cout << "=== Multikernel OS Simulator (C++) ===" << std::endl;
    std::cout << "Initializing " << NUM_CORES << " cores..." << std::endl;

    // 1. Initialize Cores
    for (int i = 0; i < NUM_CORES; i++) {
        system_cores[i] = new Core(i);
    }

    // 2. Boot Cores (Launch Threads)
    for (int i = 0; i < NUM_CORES; i++) {
        system_cores[i]->core_thread = std::thread(&Core::run, system_cores[i]);
    }

    // 3. Run Simulation
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 4. Shutdown Sequence
    std::cout << "\n=== Initiating Shutdown ===" << std::endl;
    for (int i = 0; i < NUM_CORES; i++) {
        system_cores[i]->running = false;
        system_cores[i]->inbox.wake_all(); // Wake up any sleeping cores
    }

    // 5. Join Threads and Cleanup
    for (int i = 0; i < NUM_CORES; i++) {
        if (system_cores[i]->core_thread.joinable()) {
            system_cores[i]->core_thread.join();
        }
    }

    // 6. Report
    std::cout << "\n=== Performance Report ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    for (int i = 0; i < NUM_CORES; i++) {
        double avg_lat = 0.0;
        if (system_cores[i]->stats.messages_received > 0) {
            avg_lat = (double)system_cores[i]->stats.total_latency_ns / 
                      system_cores[i]->stats.messages_received / 1000.0;
        }

        std::cout << "Core " << i 
                  << ": Avg Latency: " << avg_lat << " us"
                  << " | Sent: " << system_cores[i]->stats.messages_sent
                  << " | Recv: " << system_cores[i]->stats.messages_received 
                  << std::endl;
        
        delete system_cores[i];
    }

    return 0;
}