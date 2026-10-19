#include "NetLink.h"
// Unsupported targets retain solo gameplay and fail connection requests explicitly.
struct NetLink::Impl { State state = State::IDLE; };
NetLink::NetLink() : mImpl(new Impl()) {}
NetLink::~NetLink() { delete mImpl; }
bool NetLink::Listen(uint16_t) { mImpl->state = State::FAILED; return false; }
bool NetLink::Connect(const char*, uint16_t) { mImpl->state = State::FAILED; return false; }
void NetLink::Close() { mImpl->state = State::IDLE; }
NetLink::State NetLink::GetState() const { return mImpl->state; }
bool NetLink::IsConnected() const { return false; }
int NetLink::GetConnectAttempts() const { return 1; }
bool NetLink::Send(const void*, int) { return false; }
bool NetLink::Poll(Packet&) { return false; }
void NetLink::ClearQueue() {}
const char* NetLink::GetLastError() const { return "Networking is unavailable on this platform"; }
std::string NetLink::GetLocalIPv4Text() { return "Unavailable"; }
