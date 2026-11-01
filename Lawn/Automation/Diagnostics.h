#pragma once
#include <atomic>
#include <cstdint>
#include <string>
namespace TestDiagnostics {
struct Counters {
 std::atomic<uint64_t> raw{0},packed{0},captures{0},captureFailed{0},queued{0};
 std::atomic<uint64_t> udpTx{0},udpRx{0},udpWatchTx{0},udpWatchRx{0},tcpTx{0},tcpRx{0},tcpObserverTx{0},tcpLocalTx{0},tcpLocalRx{0};
 std::atomic<uint64_t> complete{0},displayed{0},dropped{0},invalid{0},crcFailed{0};
};
inline Counters counters;
inline std::atomic<bool> blockTestUdp{false},holdTestWatchFrames{false};
void Initialize(const std::string& instance);
void Line(const std::string& event,const std::string& message);
void Close();
}
