#include "ModText.h"
#include "Run/RunCheckpointIO.h"
#include "Common.h"
#include "paklib/PakInterface.h"
#include "../PvzpLib/PvzpDebug.h"
#include "graphics/CjkFont.h"
#include <SDL.h>
#include <fstream>

namespace
{
int setting = ModText::MODLANG_AUTO;
bool chinese = false;
bool resourcesAvailable = false;

bool SystemIsChinese()
{
    SDL_Locale* locales = SDL_GetPreferredLocales();
    const bool result = locales && locales[0].language && std::string_view(locales[0].language) == "zh";
    SDL_free(locales);
    return result;
}

std::string PreferencePath() { return Sexy::GetAppDataPath("userdata/android-language.txt"); }
}

void ModText::Initialize()
{
    std::ifstream file(Sexy::PathFromU8(PreferencePath()));
    int value;
    if (file >> value && value >= MODLANG_AUTO && value <= MODLANG_ENGLISH) setting = value;
    const auto overlay = Sexy::GetResourcePath("pvz_zh.pak");
    resourcesAvailable = Sexy::FileExists(overlay);
    const bool requested = setting == MODLANG_CHINESE || (setting == MODLANG_AUTO && SystemIsChinese());
    if (requested && resourcesAvailable) chinese = gPakInterface->AddPakFile(overlay);
    if (requested && !chinese) PvzpLogLn("[language] Chinese overlay unavailable; using English");
    PvzpLogLn("[language] setting={} active={}", setting, chinese ? "zh" : "en");
}

int ModText::GetLanguageSetting() { return setting; }

bool ModText::SetLanguageSetting(int language)
{
    if (language < MODLANG_AUTO || language > MODLANG_ENGLISH) return false;
    std::ofstream file(Sexy::PathFromU8(PreferencePath()), std::ios::trunc);
    file << language << '\n';
    file.flush();
    if (!file) return false;
    setting = language;
    return true;
}

bool ModText::IsChinese() { return chinese; }
bool ModText::HasChineseResources() { return resourcesAvailable; }
const char* ModText::Tr(const char* zh, const char* en) { return chinese ? zh : en; }
Sexy::_Font* ModText::FontFor(Sexy::_Font* original, int height)
{
    if (chinese) if (auto* font = Sexy::GetCjkFont(height)) return font;
    return original;
}
const char* ModText::SettingName()
{
    if (setting == MODLANG_CHINESE) return Tr("中文", "Chinese");
    if (setting == MODLANG_ENGLISH) return "English";
    return Tr("自动", "Auto");
}

bool ModText::ReadSlideCollect()
{
    std::ifstream file(Sexy::PathFromU8(Sexy::GetAppDataPath("userdata/android-slide-collect.txt")));
    int enabled = 1;
    return !(file >> enabled) || enabled != 0;
}

bool ModText::SaveSlideCollect(bool enabled)
{
    return RunCheckpointIO::AtomicWrite(Sexy::GetAppDataPath("userdata/android-slide-collect.txt"),
        {static_cast<unsigned char>(enabled ? '1' : '0'), '\n'});
}
