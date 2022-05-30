#!/bin/bash

cmake . -B build/ \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	-DCMAKE_BUILD_TYPE="RelWithDebugInfo" \
	-DCMAKE_C_FLAGS="-Wall -Wextra" \
	-DCMAKE_CXX_FLAGS="-Wall -Wextra"
cmake --build build/ -j$(nproc)

ln -sf build/compile_commands.json .
source build/config/config.sh
VERSION=$VERSION-dev

source package/common.sh

export _platform="unix"
export _archive=""
export _url_extension="desktop"
export _script_extension="sh"
export _binary_suffix=""

_Dist="字体合并补全工具-简体中文-$VERSION" \
_cjk="WFM-Sans-CJK-SC-Medium" \
_latin="WFM-Sans-LCG-Medium" \
package_sc
