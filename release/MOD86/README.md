# MOD86 accepted full-package sources

Fixed input tree: `fc931093be088bd44bd46e8dc1bf6bd8ddba129bd37d4a39a4e4bbd13cbb3dec`. SOURCE-MAP.json maps actual tested working-tree bytes, including untracked files. The commit retains the previous remote history.

Players: use the full PC ZIP or full-resource arm64 APK from the `MOD86-full-20261021` Release. Extract all PC files and run 启动游戏.bat. Update Android by installing over the existing app, without uninstalling. Package ID and signing identity are retained.

MOD86 fixes round-end message ordering, includes Quick level 3 in the mid+final boss setting, changes Rolling Blast to 3 stars, refreshes Options and author links, and restores the Chinese huge-wave warning. All native dependencies were rebuilt. Original gameplay, saves and shutdown fixes are retained. No silver sunflower patch or universal crash fix is claimed.

Automatic new-room load allocation is already provided online; existing rooms and reconnects retain their endpoint. gameflow1 deployment is a separate operation scheduled for 04:00 Beijing, even with online players. At package freeze it remained pending. Read the Release server-status section for actual later deployment results. Server status does not change this fixed client snapshot.

PC developers: VS2022 MSVC x86, CMake/Ninja Release. Extract the matching Source-Dependencies.zip beside the checkout so third_party is available, then configure a fresh build. Android: SDK36/min24, NDK28.2.13676358, arm64, CMake3.22.1, Gradle8.12; build SDL/native dependencies from their licensed sources. Game resources and signed APKs are in Releases, not source Git. A Git-derived version suffix may differ on future rebuilds; byte-identical signing reproduction is not promised.

Physical Android install/touch and long mixed-device public multiplayer were not acceptance-tested. Private profiles, original backups, test fixtures, signing keys and caches are excluded.
