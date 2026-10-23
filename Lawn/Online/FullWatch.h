#pragma once
#include <vector>
#include <string>
class Board;
class LawnApp;
namespace Sexy { class Graphics; }

namespace FullWatch {
// @pvz-online: the previous MOD63 implementation used a second process to keep
// global effect systems, run state and the player's own board independent.
bool IsObserver();
bool ObserverPaused();
bool CoversParent();
void AttachWindow(void* window);
void PrepareApp(LawnApp* app);
void TickApp(LawnApp* app);
void TickBoard(Board* board);
void BoardCreated(Board* board);
void BoardEdited(Board* board);
void DrawControls(Sexy::Graphics* graphics);
bool Key(int key);
void Click(int x,int y);
void Close();
void StopChild();
std::string ObserverDataRoot();
bool IsRelayedExit(Board* board);
}
