# MOD84 accepted full-package sources

Fixed input tree: `530bec2db89c7cb7c1137a739eee22b4d3ad8beb258472dbe9e4815b752b4db7`. SOURCE-MAP.json maps actual tested working-tree bytes, including untracked files. The commit retains the previous remote history.

Players: use the full PC ZIP or full-resource arm64 APK from the `mod84-full-20261020` Release. Extract all PC files and run 启动游戏.bat. Update Android by installing over the existing app, without uninstalling. Package ID and signing identity are retained.

MOD84 removes the PC level-end waiting banner. Completion reporting, waiting for teammates, boss gates and spectating remain intact. Android had no such banner; its build is synchronized. MOD83 gameplay and shutdown fixes are retained. Silver sunflower flicker is not addressed in this batch.

Public servers were still operations1 at acceptance. New host migration, preparation cancellation and fixed grace-expiry flow require gameflow1 deployment; compatibility fallback is retained. The PC branch includes exactly the final v3 server sources, not a downgrade to operations1. Server operation is a separate task.

PC developers: VS2022 MSVC x86, CMake/Ninja Release. Extract the matching Source-Dependencies.zip beside the checkout so third_party is available, then configure a fresh build. Android: SDK36/min24, NDK28.2.13676358, arm64, CMake3.22.1, Gradle8.12; build SDL/native dependencies from their licensed sources. Game resources and signed APKs are in Releases, not source Git. A Git-derived version suffix may differ on future rebuilds; byte-identical signing reproduction is not promised.

Physical Android install/touch and long mixed-device public multiplayer were not acceptance-tested. Private profiles, original backups, test fixtures, signing keys and caches are excluded.
