param([ValidateSet('Dependencies','Native','Apk','All')][string]$Stage = 'All')
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$taskRoot = Split-Path $projectDir -Parent

$env:JAVA_HOME = 'C:\Program Files\JetBrains\CLion 2025.2.1\jbr'
$env:ANDROID_HOME = Join-Path $taskRoot 'android-sdk'
$env:ANDROID_NDK_HOME = Join-Path $env:ANDROID_HOME 'ndk\28.2.13676358'
$env:GRADLE_USER_HOME = Join-Path $taskRoot 'android-build-cache\gradle'
$env:VCPKG_DOWNLOADS = Join-Path $taskRoot 'android-build-cache\downloads'
$env:VCPKG_DEFAULT_BINARY_CACHE = Join-Path $taskRoot 'android-build-cache\binary'
$cmakeExe = Join-Path $env:ANDROID_HOME 'cmake\3.22.1\bin\cmake.exe'
$ninjaExe = Join-Path $env:ANDROID_HOME 'cmake\3.22.1\bin\ninja.exe'
$vcpkgDir = Join-Path $taskRoot 'android-vcpkg'
$depsDir = Join-Path $taskRoot 'android-native-api24-deps'
$env:PATH = "$(Split-Path $cmakeExe);$env:PATH"

function Invoke-BuildTool([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Build tool failed ($LASTEXITCODE): $Executable" }
}
if ($Stage -in @('Dependencies','All')) {
    New-Item -ItemType Directory -Force $env:VCPKG_DEFAULT_BINARY_CACHE | Out-Null
    # Fixed local ports avoid a Git registry fetch on unreliable connections.
    Invoke-BuildTool "$vcpkgDir\vcpkg.exe" @('install','libjpeg-turbo','libopenmpt','libpng','zlib','--classic','--triplet','arm64-android',"--overlay-triplets=$projectDir\tools\android", "--x-install-root=$depsDir", "--x-buildtrees-root=$taskRoot\android-build-cache\buildtrees-api24", "--x-packages-root=$taskRoot\android-build-cache\packages-api24")
}
if ($Stage -in @('Native','All')) {
    $sdlPrefix = "$projectDir\sdl2-android"
    $ndkToolchain = "$env:ANDROID_NDK_HOME\build\cmake\android.toolchain.cmake"
    # Reuse the fixed SDL dependency when only gameplay code changed.
    if (!(Test-Path "$sdlPrefix\lib\libSDL2.so")) {
        Invoke-BuildTool $cmakeExe @('-S',"$projectDir\sdl2-src",'-B',"$projectDir\build-sdl2",'-G','Ninja',"-DCMAKE_MAKE_PROGRAM=$ninjaExe","-DCMAKE_TOOLCHAIN_FILE=$ndkToolchain",'-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-24','-DSDL_SHARED=ON','-DSDL_STATIC=OFF','-DSDL_TEST=OFF',"-DCMAKE_INSTALL_PREFIX=$sdlPrefix",'-DCMAKE_BUILD_TYPE=Release')
        Invoke-BuildTool $cmakeExe @('--build',"$projectDir\build-sdl2",'--parallel','8')
        Invoke-BuildTool $cmakeExe @('--install',"$projectDir\build-sdl2")
    }
    # Classic-mode dependencies are preinstalled; preserve the upstream manifest untouched.
    Invoke-BuildTool $cmakeExe @('-S',$projectDir,'-B',"$projectDir\build-android-api24-arm64-v8a",'-G','Ninja',"-DCMAKE_MAKE_PROGRAM=$ninjaExe","-DCMAKE_TOOLCHAIN_FILE=$vcpkgDir\scripts\buildsystems\vcpkg.cmake",'-DVCPKG_MANIFEST_MODE=OFF',"-DVCPKG_INSTALLED_DIR=$depsDir",'-DVCPKG_TARGET_TRIPLET=arm64-android',"-DVCPKG_OVERLAY_TRIPLETS=$projectDir\tools\android",'-DVCPKG_CMAKE_SYSTEM_VERSION=24',"-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$ndkToolchain",'-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-24',"-DSDL2_DIR=$sdlPrefix\lib\cmake\SDL2",'-DCMAKE_BUILD_TYPE=Release')
    Invoke-BuildTool $cmakeExe @('--build',"$projectDir\build-android-api24-arm64-v8a",'--parallel','8')
    $jniDir = "$projectDir\android\app\src\main\jniLibs\arm64-v8a"
    New-Item -ItemType Directory -Force $jniDir | Out-Null
    Copy-Item -LiteralPath "$projectDir\build-android-api24-arm64-v8a\libmain.so" -Destination $jniDir
    Copy-Item -Path "$sdlPrefix\lib\libSDL2*.so" -Destination $jniDir
    Get-ChildItem "$depsDir\arm64-android\lib" -Filter '*.so' | Copy-Item -Destination $jniDir
    $libcpp = "$env:ANDROID_NDK_HOME\toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\lib\aarch64-linux-android\libc++_shared.so"
    if (Test-Path $libcpp) { Copy-Item -LiteralPath $libcpp -Destination $jniDir }
}
if ($Stage -in @('Apk','All')) {
    if (!(Test-Path "$projectDir\android\app\src\main\jniLibs\arm64-v8a\libmain.so")) { throw 'Native library missing; run Native stage first.' }
    Set-Content "$projectDir\android\local.properties" ('sdk.dir=' + ($env:ANDROID_HOME -replace '\\','/'))
    Invoke-BuildTool "$taskRoot\android-build-tools\gradle-8.12\bin\gradle.bat" @('-p',"$projectDir\android",'assembleRelease','-PusePrebuiltLibs=true','--no-daemon','--console=plain')
    Write-Output "APK: $projectDir\android\app\build\outputs\apk\release\app-release.apk"
}
