# 构建和打包环境

## `windows-x64` — Windows x64

使用 MSYS2 MinGW x64 环境。

操作步骤：
1. 安装 `msys/cmake`、`msys/make`、`mingw-w64-x86_64-toolchain`（包组）、`msys/p7zip`。
1. 执行 `package/windows-x64.sh`。

## `windows-arm64` — Windows ARM64

使用 MSYS2 MSYS 环境（交叉编译）。

操作步骤：
1. 安装 `msys/cmake`、`msys/make`、`mingw-w64-cross-clang-toolchain`（包组）、`msys/lld`、`msys/p7zip`。
1. 执行 `package/windows-arm64.sh`。

## `windows-32` — Windows x86（32 位，已弃用）

* WFM 在 32 位环境下极易因耗尽内存而崩溃。
* Windows ARM64 工具链逐渐成熟，现在已经不再需要 32 位 x86 应用程序来兼容。

使用 MSYS2 MinGW x86 环境。

操作步骤：
1. 安装 `msys/cmake`、`msys/make`、`mingw-w64-i686-toolchain`（包组）、`msys/p7zip`。
1. 执行 `package/windows-32.sh`。

## `mac` - macOS

使用 XCode 12 工具链。

操作步骤：
1. 安装 CMake 并安装命令行工具。
1. 安装 XCode Command Line Tools。
1. 执行 `package/mac.sh`。

## `linux-amd64` - Linux x86-64

使用 Alpine musl 环境。

操作步骤：
1. 安装 `bash`、`cmake`、`make`、`gcc`、`g++`、`xz`。
1. 执行 `package/linux-amd64.sh`。

注：推荐通过 Docker 启动 Alpine musl 环境。

```bash
# 启动 Alpine musl 环境
docker run -it --rm -v "/path/to/wfm:/wfm" alpine ash

# 如果需要 clean build
rm -rf /wfm/build /wfm/release

# 如果需要切换镜像站
sed -i "s|dl-cdn.alpinelinux.org|mirrors.ustc.edu.cn|" /etc/apk/repositories

# 安装软件包
apk add bash cmake make gcc g++ xz

# 创建用户（并指定和主机系统相同的 UID 以方便访问）
adduser -u 1000 -D builduser

# 编译并打包
su builduser -c "cd /wfm; package/linux-amd64.sh"
```
