#ifndef PVZ_ONLINE_MODTEXT_H
#define PVZ_ONLINE_MODTEXT_H
#include <format>
namespace Sexy { class _Font; }

namespace ModText
{
enum Language { MODLANG_AUTO = 0, MODLANG_CHINESE = 1, MODLANG_ENGLISH = 2 };
// The active language stays fixed until restart, matching the cached image/font resources.
void Initialize();
int GetLanguageSetting();
bool SetLanguageSetting(int language);
bool IsChinese();
bool HasChineseResources();
const char* Tr(const char* chinese, const char* english);
const char* SettingName();
bool ReadSlideCollect();
bool SaveSlideCollect(bool enabled);
Sexy::_Font* FontFor(Sexy::_Font* original, int pixelHeight);
template<typename... Args>
std::string Format(const char* zh, const char* en, Args&&... args)
{
    return std::vformat(Tr(zh, en), std::make_format_args(args...));
}
}

#endif
