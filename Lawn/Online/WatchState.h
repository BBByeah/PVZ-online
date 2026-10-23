#pragma once
#include "WatchWire.h"
class Board;
class LawnApp;
namespace WatchState {
bool Capture(Board*,WatchWire::Bytes& config,WatchWire::Bytes& state);
bool Restore(LawnApp*,const WatchWire::Bytes& complete,const WatchWire::Header&,bool incremental=false);
}
