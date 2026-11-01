#pragma once
#include <string>
class LawnApp;
namespace MachineControl {
bool Parameter(const std::string&,const std::string&);
bool Enabled();
bool LocalWatch();
bool EdgeKill();
const std::string& Instance();
void Configure(LawnApp*);
void Tick(LawnApp*);
void Stop();
}
