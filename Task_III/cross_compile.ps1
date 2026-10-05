param(
    [Parameter(Mandatory = $true)]
    [string]$Ndk,
    [int]$Api = 24
)

$ErrorActionPreference = "Stop"
$toolchain = Join-Path $Ndk "toolchains\llvm\prebuilt\windows-x86_64"
$sysroot = Join-Path $toolchain "sysroot"
$compiler = Join-Path $toolchain "bin\aarch64-linux-android$Api-clang.cmd"

if (-not (Test-Path $compiler)) {
    $compiler = Join-Path $toolchain "bin\aarch64-linux-android$Api-clang.exe"
}
if (-not (Test-Path $compiler)) {
    throw "NDK compiler not found: $compiler"
}

$common = @(
    "-Iinclude",
    "--sysroot=$sysroot",
    "-O2",
    "-Wall",
    "-Wextra",
    "-std=c11"
)
$libraries = @("-lEGL", "-lGLESv3")

& $compiler @common "egl_probe.c" "-o" "egl_probe" @libraries
if ($LASTEXITCODE -ne 0) {
    throw "egl_probe cross-compilation failed"
}

& $compiler @common "forest_fire.c" "-o" "forest_fire" @libraries
if ($LASTEXITCODE -ne 0) {
    throw "forest_fire cross-compilation failed"
}

Write-Host "Cross-compilation complete: egl_probe and forest_fire"
