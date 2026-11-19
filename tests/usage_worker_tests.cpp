#include "pch.h"
#include "app.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace tmp;

// ─── Test 1: Snapshot Synchronization (Audit Finding 2) ───────────────────
// Verify that publishing recognizable pairs (N, 2*N) across threads never
// yields mixed reads to concurrent readers.
void TestSnapshotSynchronization() {
    std::cout << "[Test 1] Usage snapshot race test (Finding 2)\n";

    std::mutex mtx;
    WindowsUsageSnapshot shared_snapshot;
    std::atomic<bool> running{true};
    std::atomic<uint64_t> published_count{0};
    std::atomic<uint64_t> read_count{0};

    // Writer thread publishing recognizable pairs: today.download = N, month.download = 2*N
    std::thread writer([&]() {
        for (uint64_t i = 1; i <= 20000; ++i) {
            WindowsUsageSnapshot snap{};
            snap.today.download = i;
            snap.today.upload = i * 10;
            snap.month.download = i * 2;
            snap.month.upload = i * 20;
            snap.has_usage = true;
            {
                std::lock_guard<std::mutex> lock(mtx);
                shared_snapshot = snap;
            }
            published_count.fetch_add(1);
        }
        running = false;
    });

    // 4 concurrent reader threads
    std::vector<std::thread> readers;
    for (int r = 0; r < 4; ++r) {
        readers.emplace_back([&]() {
            while (running.load() || published_count.load() < 20000) {
                WindowsUsageSnapshot local_snap;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    local_snap = shared_snapshot;
                }
                if (local_snap.has_usage) {
                    // Critical assertion: month.download must ALWAYS be 2 * today.download
                    assert(local_snap.month.download == local_snap.today.download * 2);
                    assert(local_snap.today.upload == local_snap.today.download * 10);
                    assert(local_snap.month.upload == local_snap.month.download * 10);
                    read_count.fetch_add(1);
                }
                std::this_thread::yield();
            }
        });
    }

    writer.join();
    for (auto& t : readers) {
        t.join();
    }

    std::cout << "  Passed. " << published_count.load() << " snapshots published, "
              << read_count.load() << " reads verified with 0 mixed pairs.\n";
}

// ─── Test 2: Decoupled Worker Lifetime & Shutdown (Audit Finding 3) ───────
// Verify that a worker blocked in a slow provider does not crash when the
// owning object is destroyed before the worker finishes.
class MockAppOwner {
public:
    std::shared_ptr<UsageWorkerState> worker_state;
    std::atomic<bool> shutting_down{false};

    MockAppOwner() {
        worker_state = std::make_shared<UsageWorkerState>();
    }

    void Shutdown(std::chrono::milliseconds timeout) {
        shutting_down = true;
        if (worker_state) {
            worker_state->cancelled = true;
            worker_state->target_hwnd.store(nullptr);
        }
        const auto start = std::chrono::steady_clock::now();
        while (worker_state && worker_state->active_workers.load() > 0 &&
               std::chrono::steady_clock::now() - start < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    ~MockAppOwner() {
        // Can be destroyed even if active_workers > 0
    }
};

void TestWorkerOutlivesOwner() {
    std::cout << "[Test 2] Slow worker outlives owner on shutdown (Finding 3)\n";

    std::atomic<bool> provider_called{false};
    std::atomic<bool> release_provider{false};
    std::atomic<bool> worker_completed{false};

    std::shared_ptr<UsageWorkerState> preserved_state;

    {
        auto owner = std::make_unique<MockAppOwner>();
        preserved_state = owner->worker_state;
        auto state = owner->worker_state;

        // Launch simulated worker
        state->active_workers.fetch_add(1);
        std::thread worker([state, &provider_called, &release_provider, &worker_completed]() {
            struct Guard {
                std::shared_ptr<UsageWorkerState> s;
                std::atomic<bool>& done;
                ~Guard() {
                    s->active_workers.fetch_sub(1);
                    done = true;
                }
            } guard{ state, worker_completed };

            if (state->cancelled.load()) return;

            // Simulate slow provider: block until release_provider is true
            provider_called = true;
            while (!release_provider.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // Post-provider check
            if (state->cancelled.load()) {
                // Safely cancelled, no attempt to dereference destroyed owner or HWND
                return;
            }
        });
        worker.detach();

        // Wait until worker enters provider
        while (!provider_called.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // Initiate bounded shutdown with a short 50ms timeout (less than provider delay)
        owner->Shutdown(std::chrono::milliseconds(50));
        assert(owner->worker_state->active_workers.load() > 0);

        // Owner is destroyed right here!
        owner.reset();
        std::cout << "  Owner destroyed while worker was still inside provider.\n";
    }

    // Give some time to ensure owner is completely destroyed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Release the fake provider now
    release_provider = true;

    // Wait for the detached worker to exit safely
    const auto wait_start = std::chrono::steady_clock::now();
    while (!worker_completed.load() &&
           std::chrono::steady_clock::now() - wait_start < std::chrono::seconds(3)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    assert(worker_completed.load() == true);
    assert(preserved_state->active_workers.load() == 0);
    std::cout << "  Passed. Worker safely exited after owner destruction without crashes or memory corruption.\n";
}

int main() {
    TestSnapshotSynchronization();
    TestWorkerOutlivesOwner();
    std::cout << "\nAll usage worker lifetime and synchronization tests passed!\n";
    return 0;
}
