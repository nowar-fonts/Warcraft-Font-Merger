# 构建和打包环境

## `winx86` — Windows x64

使用 MSYS2 MinGW 64-bit 环境。

操作步骤：
1. 安装 `msys/p7zip`、`msys/cmake`、`mingw-w64-x86_64-toolchain`（包组）。
2. 执行 `package/winx86.sh`。

## `win32` — Windows x86（32 位，已弃用）

* WFM 在 32 位环境下极易因耗尽内存而崩溃。
* Windows ARM64 工具链逐渐成熟，现在已经不再需要 32 位 x86 应用程序来兼容。

使用 MSYS2 MinGW 32-bit 环境。

操作步骤：
1. 安装 `msys/p7zip`、`msys/cmake`、`mingw-w64-i686-toolchain`（包组）。
2. 执行 `package/win32.sh`。

## `winarm` — Windows ARM64

在 Git Bash 环境下使用 [LLVM MinGW](https://github.com/mstorsjo/llvm-mingw) UCRT 工具链。

操作步骤：
1. 安装 7-zip（Windows 原生）并加入 `PATH` 环境变量。
2. 安装 CMake（Windows 原生）并加入 `PATH` 环境变量。
3. 获取 LLVM MinGW 并把 `llvm-mingw/bin` 目录加入 `PATH` 环境变量。（注：该项目发布页 `i686` `x86_64` 等为 host 架构，每个包都支持全部 4 个 target 架构。）
4. 执行 `package/winarm.sh`。

## `mac` - macOS

使用 XCode 12 工具链。

操作步骤：
1. 安装 CMake 并安装命令行工具。
2. 安装 XCode Command Line Tools。
3. 执行 `package/mac.sh`。

## `linux` - Linux x86-64

使用 Gentoo musl 环境。（注：不需要完整安装 Gentoo，下载 stage3 映像、chroot 到 Gentoo musl 环境、配置 portage 即可。）

操作步骤：
1. 安装 `dev-util/cmake`。
2. 执行 `package/linux.sh`。
