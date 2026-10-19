# MOD82 accepted source snapshot

This branch publishes the actual source inputs of the tested MOD82 full packages dated 2026-10-20.
Snapshot input digest: `8a1ab2217c80a1cf57eaf3e38524687f6b75d0035f52ee43d102c0204a40b320`.
Frozen Git baseline: `678b7190fe32401cc409a720240abc8345600b5a`. The accepted working tree also contained
uncommitted/untracked files; the baseline alone is not the built source. SOURCE-MAP.json identifies
every published product input by path and SHA256. Publication metadata in this directory was added
after acceptance and does not change game code. Parent history is retained without force pushing.

## Players

Use the complete PC ZIP or full-resource Android APK from the `mod82-full-20261020` GitHub Release.
PC: extract the whole archive and run 启动游戏.bat. Android: install the full APK over the existing
installation; save and exit first, and do not uninstall if you want to retain saves. All online players
must use MOD82. Packages include resources and dependencies. No player profiles or signing keys
are published. Android device/touch and long public multiplayer sessions were not acceptance-tested.

## Developers

PC: use the `pvz-online` branch with VS2022 MSVC x86, Windows SDK, CMake and Ninja. Download the
matching `PvZ-Online-MOD82-Source-Dependencies.zip` Release asset and extract its `third_party/`
beside the source checkout, as required by SexyAppFramework/imagelib/CMakeLists.txt. In an x86
VS developer prompt configure `cmake -S <source> -B <fresh-build> -G Ninja -DCMAKE_BUILD_TYPE=Release`,
then `cmake --build <fresh-build> --parallel 6`. The target is SexyAppFramework.exe. Game resources,
Chinese overlay and VC runtime are provided in the complete player package, not in source Git.

Android: use the `android-port` branch, Android API 24 minimum, arm64-v8a, SDK/target 36,
NDK 28.2.13676358, CMake 3.22.1 and Gradle 8.12. SDL source/Java files and SDL-Mixer-X source
are included with their upstream licences; regenerate the SDL native prefix and native dependencies
for arm64/API24 instead of reusing build caches. BUILD-DEPENDENCIES.json lists the exact accepted
native input files by hash; installed dependency prefixes are not vendored as source. Configure
the native build with `-DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DCMAKE_BUILD_TYPE=Release`
and explicit SDL2/native dependency search paths. Gradle can package accepted native outputs using
`-PusePrebuiltLibs=true -PbundledGameAssets=<prepared-assets>`; resource preparation is provided by
`tools/android/prepare-bundled-resources.ps1`. Use your own developer signing identity; the released
APK retains package `io.github.pvzonline.android` and the existing public signing certificate fingerprint.

The published APK version is `0.1-online82.c01ab12b-full`, versionCode 82. New publication commits
and tags change Git-derived version text for future rebuilds. These source manifests do not promise
byte-identical reproduction without the original build environment and signing identity.

The source snapshot contains the historical MOD82 relay. It does not include the later operations1
server deployment or developing MOD83 client features. Root historical docs may retain older resource
publication wording; the user explicitly authorised public full-resource player packages on 2026-10-20.
Upstream code/dependency licences remain applicable. The original authors and help links are retained.
