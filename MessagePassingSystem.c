/*
 * Multikernel OS Simulation - Messaging & Distributed Algorithms
 * Role: Messaging Developer
 * Language: C (Standard for OS Development)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// ==========================================
// 1. CONFIGURATION & CONSTANTS
// ==========================================
#define NUM_CORES 4
#define QUEUE_SIZE 256
#define PAYLOAD_SIZE 64

// Message Types
typedef enum {
    MSG_TYPE_PING,
    MSG_TYPE_ACK,
    MSG_TYPE_BARRIER_REACHED,
    MSG_TYPE_BARRIER_GO,
    MSG_TYPE_ELECTION_VOTE,
    MSG_TYPE_ELECTION_WINNER,
    MSG_TYPE_SHUTDOWN
} MsgType;

// ==========================================
// 2. DATA STRUCTURES (Message Definitions)
// ==========================================

// The standard message packet
typedef struct {
    int src_core_id;
    int dest_core_id;
    MsgType type;
    uint32_t sequence_num;  // For ordering/reliability
    char payload[PAYLOAD_SIZE];
    uint64_t timestamp;     // For latency tracking
} Message;

// A Ring Buffer to simulate a hardware message queue (NIC/Interconnect)
typedef struct {
    Message buffer[QUEUE_SIZE];
    int head;
    int tail;
    pthread_mutex_t lock;   // Simulating hardware lock/atomic operation
    pthread_cond_t not_empty;
} MessageQueue;

// Performance Counters
typedef struct {
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t total_latency_ns;
} PerfCounters;

// The "OS Instance" per Core
// In a real OS, this would be the kernel data structure located in core-local memory
typedef struct {
    int core_id;
    MessageQueue inbox;     // Each core has its own inbox
    PerfCounters stats;
    int current_leader;     // Who this core thinks the leader is
    bool running;
} CoreOS;

// Global interconnect (The "Bus")
// Note: Cores can ONLY access this to push messages to other cores' inboxes.
CoreOS system_cores[NUM_CORES];

// ==========================================
// 3. MESSAGING PRIMITIVES (API)
// ==========================================

void init_queue(MessageQueue *q) {
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

// Get current time in nanoseconds
uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// SEND: Puts a message into the destination core's hardware queue
// Returns 0 on success, -1 if queue full (simulating packet drop or backpressure)
int send_message(int src_id, int dest_id, MsgType type, const char* data) {
    if (dest_id < 0 || dest_id >= NUM_CORES) return -1;

    CoreOS *dest_core = &system_cores[dest_id];
    CoreOS *src_core = &system_cores[src_id];

    pthread_mutex_lock(&dest_core->inbox.lock);

    int next_tail = (dest_core->inbox.tail + 1) % QUEUE_SIZE;
    if (next_tail == dest_core->inbox.head) {
        // Queue is full - Drop packet or spin (Design Choice: Drop/Error)
        pthread_mutex_unlock(&dest_core->inbox.lock);
        return -1; 
    }

    Message *msg = &dest_core->inbox.buffer[dest_core->inbox.tail];
    msg->src_core_id = src_id;
    msg->dest_core_id = dest_id;
    msg->type = type;
    msg->sequence_num = src_core->stats.messages_sent; 
    msg->timestamp = get_time_ns();
    if (data) strncpy(msg->payload, data, PAYLOAD_SIZE);
    else memset(msg->payload, 0, PAYLOAD_SIZE);

    dest_core->inbox.tail = next_tail;

    // Update stats
    src_core->stats.messages_sent++;

    // Signal the destination core (Simulating an interrupt)
    pthread_cond_signal(&dest_core->inbox.not_empty);
    pthread_mutex_unlock(&dest_core->inbox.lock);
    
    return 0;
}

// RECEIVE: Blocking call to get next message
Message receive_message(int core_id) {
    CoreOS *core = &system_cores[core_id];
    Message msg;

    pthread_mutex_lock(&core->inbox.lock);

    // Wait until message arrives
    while (core->inbox.head == core->inbox.tail && core->running) {
        pthread_cond_wait(&core->inbox.not_empty, &core->inbox.lock);
    }

    if (!core->running) {
        pthread_mutex_unlock(&core->inbox.lock);
        Message exit_msg = {0}; 
        exit_msg.type = MSG_TYPE_SHUTDOWN;
        return exit_msg;
    }

    // Copy message out
    msg = core->inbox.buffer[core->inbox.head];
    core->inbox.head = (core->inbox.head + 1) % QUEUE_SIZE;

    // Update stats
    core->stats.messages_received++;
    uint64_t now = get_time_ns();
    if (now > msg.timestamp) {
        core->stats.total_latency_ns += (now - msg.timestamp);
    }

    pthread_mutex_unlock(&core->inbox.lock);
    return msg;
}

// ==========================================
// 4. DISTRIBUTED ALGORITHMS
// ==========================================

// Algorithm A: Distributed Barrier
// All cores must reach this function before any proceed.
// Core 0 acts as the coordinator.
void enter_barrier(int core_id) {
    printf("[Core %d] Entering Barrier...\n", core_id);

    if (core_id == 0) {
        // Coordinator Logic
        int cores_arrived = 1; // Count self
        while (cores_arrived < NUM_CORES) {
            Message msg = receive_message(core_id);
            if (msg.type == MSG_TYPE_BARRIER_REACHED) {
                cores_arrived++;
            }
        }
        printf("[Core 0] All cores arrived. Releasing Barrier.\n");
        // Broadcast GO
        for (int i = 1; i < NUM_CORES; i++) {
            send_message(0, i, MSG_TYPE_BARRIER_GO, "GO");
        }
    } else {
        // Worker Logic
        send_message(core_id, 0, MSG_TYPE_BARRIER_REACHED, NULL);
        // Wait for GO
        while (1) {
            Message msg = receive_message(core_id);
            if (msg.type == MSG_TYPE_BARRIER_GO) break;
            // If we receive other messages while waiting, we might queue them or discard (simplified here)
        }
    }
    printf("[Core %d] Passed Barrier.\n", core_id);
}

// Algorithm B: Leader Election (Bully Algorithm Simplified)
// Highest ID wins.
void start_election(int core_id) {
    printf("[Core %d] Detecting no leader. Starting Election.\n", core_id);
    
    // Broadcast vote to all cores with higher ID
    int higher_cores = 0;
    for (int i = core_id + 1; i < NUM_CORES; i++) {
        send_message(core_id, i, MSG_TYPE_ELECTION_VOTE, "VOTE");
        higher_cores++;
    }

    if (higher_cores == 0) {
        // I am the highest, I win.
        printf("[Core %d] I am the new Leader!\n", core_id);
        system_cores[core_id].current_leader = core_id;
        // Announce victory
        for (int i = 0; i < NUM_CORES; i++) {
            if (i != core_id) send_message(core_id, i, MSG_TYPE_ELECTION_WINNER, "I_WON");
        }
    }
}

// ==========================================
// 5. CORE SIMULATION LOOP
// ==========================================

void* core_routine(void* arg) {
    int id = *(int*)arg;
    free(arg);
    
    printf("[Core %d] Booted up.\n", id);

    // 1. Random work simulation
    usleep(rand() % 100000); 

    // 2. Synchronization Point (Barrier)
    enter_barrier(id);

    // 3. Distributed Task (Leader Election Simulation)
    // Only Core 1 decides to start an election for demonstration
    if (id == 1) {
        usleep(50000);
        start_election(id);
    }

    // 4. Message Loop (Runtime)
    // Cores process messages until shutdown
    while (system_cores[id].running) {
        Message msg = receive_message(id);
        
        switch (msg.type) {
            case MSG_TYPE_ELECTION_VOTE:
                // If I get a vote request, I am higher, so I should take over election
                // (Simplified: just ack or ignore for this demo)
                break;
            case MSG_TYPE_ELECTION_WINNER:
                system_cores[id].current_leader = msg.src_core_id;
                printf("[Core %d] Acknowledged Leader: Core %d\n", id, msg.src_core_id);
                break;
            case MSG_TYPE_SHUTDOWN:
                goto exit_thread;
            default:
                break;
        }
    }

exit_thread:
    printf("[Core %d] Shutting down. Stats: Sent=%lu, Recv=%lu\n", 
           id, system_cores[id].stats.messages_sent, system_cores[id].stats.messages_received);
    return NULL;
}

// ==========================================
// 6. MAIN (The "Hypervisor" / Hardware Setup)
// ==========================================
int main() {
    srand(time(NULL));
    pthread_t threads[NUM_CORES];

    printf("=== Multikernel OS Simulator ===\n");
    printf("Initializing %d cores...\n", NUM_CORES);

    // Initialize Hardware (Queues)
    for (int i = 0; i < NUM_CORES; i++) {
        system_cores[i].core_id = i;
        system_cores[i].running = true;
        system_cores[i].current_leader = -1;
        system_cores[i].stats.messages_sent = 0;
        system_cores[i].stats.messages_received = 0;
        system_cores[i].stats.total_latency_ns = 0;
        init_queue(&system_cores[i].inbox);
    }

    // Boot Cores
    for (int i = 0; i < NUM_CORES; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        if (pthread_create(&threads[i], NULL, core_routine, id) != 0) {
            perror("Failed to create core thread");
            return 1;
        }
    }

    // Let simulation run for 2 seconds
    sleep(2);

    // Shutdown Signal
    printf("\n=== Initiating Shutdown ===\n");
    for (int i = 0; i < NUM_CORES; i++) {
        system_cores[i].running = false;
        // Wake up sleeping cores so they can see running=false
        pthread_mutex_lock(&system_cores[i].inbox.lock);
        pthread_cond_signal(&system_cores[i].inbox.not_empty);
        pthread_mutex_unlock(&system_cores[i].inbox.lock);
    }

    // Join Threads
    for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Final Report
    printf("\n=== Performance Report ===\n");
    for (int i = 0; i < NUM_CORES; i++) {
        double avg_lat = system_cores[i].stats.messages_received > 0 ? 
            (double)system_cores[i].stats.total_latency_ns / system_cores[i].stats.messages_received / 1000.0 : 0.0;
            
        printf("Core %d: Avg Msg Latency: %.2f us | Sent: %lu | Recv: %lu\n", 
            i, avg_lat, system_cores[i].stats.messages_sent, system_cores[i].stats.messages_received);
    }

    return 0;
}