#include "pch.h"
#include "telemetry/process_monitor.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

using namespace tmp;

void TestAuditCounterexample() {
    std::cout << "[Test 1] Audit Finding 7 counterexample\n";
    // Counterexample from review:
    // A=(1.00%, 300), B=(1.04%, 200), C=(1.08%, 100)
    ProcessInfo a{ 1, L"proc_a", 1.00, 0, 300 };
    ProcessInfo b{ 2, L"proc_b", 1.04, 0, 200 };
    ProcessInfo c{ 3, L"proc_c", 1.08, 0, 100 };

    // With exact lexicographic ordering (descending CPU):
    // C has highest CPU (1.08%) -> C before B
    // B has higher CPU (1.04% vs 1.00%) -> B before A
    // Therefore C before A (transitive!)
    assert(CompareProcessCpuUsage(c, b) == true);
    assert(CompareProcessCpuUsage(b, a) == true);
    assert(CompareProcessCpuUsage(c, a) == true);

    // No reverse relations (asymmetric)
    assert(CompareProcessCpuUsage(b, c) == false);
    assert(CompareProcessCpuUsage(a, b) == false);
    assert(CompareProcessCpuUsage(a, c) == false);

    // In all 6 permutations, std::sort must produce { C, B, A }
    std::vector<ProcessInfo> procs = { a, b, c };
    std::vector<int> p = { 0, 1, 2 };
    int perm_count = 0;
    do {
        std::vector<ProcessInfo> test_list = { procs[p[0]], procs[p[1]], procs[p[2]] };
        std::sort(test_list.begin(), test_list.end(), CompareProcessCpuUsage);
        assert(test_list[0].pid == 3); // C
        assert(test_list[1].pid == 2); // B
        assert(test_list[2].pid == 1); // A
        perm_count++;
    } while (std::next_permutation(p.begin(), p.end()));
    assert(perm_count == 6);
    std::cout << "  Passed. 6 permutations sorted deterministically into {C, B, A}.\n";
}

void TestStrictWeakOrderingProperties() {
    std::cout << "[Test 2] Strict weak ordering axioms\n";
    std::vector<ProcessInfo> sample = {
        { 10, L"p1", 5.0, 0, 1024 },
        { 20, L"p2", 5.0, 0, 2048 },
        { 30, L"p3", 5.0, 0, 1024 },
        { 40, L"p4", 2.5, 0, 4096 },
        { 50, L"p5", 8.0, 0, 512  },
        { 60, L"p6", 0.0, 0, 0    }
    };

    // 1. Irreflexivity: comp(x, x) == false
    for (const auto& x : sample) {
        assert(!CompareProcessCpuUsage(x, x));
    }

    // 2. Asymmetry: comp(x, y) == true => comp(y, x) == false
    for (const auto& x : sample) {
        for (const auto& y : sample) {
            if (CompareProcessCpuUsage(x, y)) {
                assert(!CompareProcessCpuUsage(y, x));
            }
        }
    }

    // 3. Transitivity: comp(x, y) && comp(y, z) => comp(x, z)
    for (const auto& x : sample) {
        for (const auto& y : sample) {
            for (const auto& z : sample) {
                if (CompareProcessCpuUsage(x, y) && CompareProcessCpuUsage(y, z)) {
                    assert(CompareProcessCpuUsage(x, z));
                }
            }
        }
    }
    std::cout << "  Passed. Irreflexivity, asymmetry, and transitivity verified.\n";
}

void TestTieBreaking() {
    std::cout << "[Test 3] Tie breaking: CPU equal -> Memory; Memory equal -> PID\n";
    // Equal CPU, different memory
    ProcessInfo m1{ 101, L"m1", 4.0, 0, 5000 };
    ProcessInfo m2{ 102, L"m2", 4.0, 0, 3000 };
    assert(CompareProcessCpuUsage(m1, m2) == true);
    assert(CompareProcessCpuUsage(m2, m1) == false);

    // Equal CPU, equal memory, different PID
    ProcessInfo id1{ 50, L"id1", 4.0, 0, 3000 };
    ProcessInfo id2{ 80, L"id2", 4.0, 0, 3000 };
    assert(CompareProcessCpuUsage(id1, id2) == true);
    assert(CompareProcessCpuUsage(id2, id1) == false);
    std::cout << "  Passed. Deterministic tie breaking verified.\n";
}

void TestLargeCollectionSort() {
    std::cout << "[Test 4] Stress test std::sort on 1,000 processes\n";
    std::mt19937 rng(1337);
    std::uniform_real_distribution<double> cpu_dist(0.0, 100.0);
    std::uniform_int_distribution<uint64_t> mem_dist(1024, 1024 * 1024 * 1024);

    std::vector<ProcessInfo> procs;
    procs.reserve(1000);
    for (DWORD i = 1; i <= 1000; ++i) {
        procs.push_back({ i, L"proc", cpu_dist(rng), 0, mem_dist(rng) });
    }

    // Must sort without crashing, infinite looping or violating assertions
    std::sort(procs.begin(), procs.end(), CompareProcessCpuUsage);

    // Verify sorted descending
    for (size_t i = 1; i < procs.size(); ++i) {
        assert(!CompareProcessCpuUsage(procs[i], procs[i - 1]));
    }
    std::cout << "  Passed. 1,000 elements sorted strictly.\n";
}

int main() {
    // Hold a scan in flight to verify UI reads never wait or publish partial data.
    std::promise<void> release, started;
    auto gate=release.get_future().share();
    auto began=started.get_future();
    const DWORD ui_thread=GetCurrentThreadId();
    {
        ProcessMonitor monitor([&](){
            assert(GetCurrentThreadId()!=ui_thread);
            started.set_value();
            gate.wait();
            return std::vector<ProcessInfo>{{42,L"worker",12.0,0,1234}};
        });
        monitor.Poll();
        assert(began.wait_for(std::chrono::seconds(5))==std::future_status::ready);
        monitor.Poll();
        assert(monitor.GetProcesses().empty());
        release.set_value();
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(1);
        while(monitor.GetProcesses().empty()&&std::chrono::steady_clock::now()<deadline){
            monitor.Poll();Sleep(1);
        }
        assert(monitor.GetProcesses().size()==1);
        assert(monitor.GetProcesses()[0].pid==42);
        assert(monitor.GetTopCpuProcesses()[0].memory_bytes==1234);
    }
    TestAuditCounterexample();
    TestStrictWeakOrderingProperties();
    TestTieBreaking();
    TestLargeCollectionSort();
    std::cout << "\nAll process sorting tests passed successfully!\n";
    return 0;
}
