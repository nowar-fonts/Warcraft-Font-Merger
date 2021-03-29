#!/bin/bash

cmake . -B build/ \
	-DCMAKE_BUILD_TYPE="Release" \
	-DCMAKE_C_COMPILER="i686-w64-mingw32-gcc" \
	-DCMAKE_C_FLAGS="-static -s -Wl,--large-address-aware" \
	-DCMAKE_CXX_COMPILER="i686-w64-mingw32-g++" \
	-DCMAKE_CXX_FLAGS="-static -s -Wl,--large-address-aware"
cmake --build build/ -j$(nproc)

source build/config/config.sh
VERSION=$VERSION-win32

source package/common.sh

export _platform="windows"
export _archive="7z"
export _url_extension="url"
export _binary_suffix=".exe"

_Dist="字体合并补全工具-简体中文压缩字库-$VERSION" \
_dist="WarFontMerger-SC1-$VERSION" \
_cjk="Nowar-Sans-CJK-XS-Regular" \
_latin="Nowar-Sans-LCG-Apache-Regular" \
package_sc

_Dist="字体合并补全工具-简体中文标准字库-$VERSION" \
_dist="WarFontMerger-SC2-$VERSION" \
_cjk="Nowar-Sans-CJK-CN-Medium" \
_latin="Nowar-Sans-LCG-Medium" \
package_sc

_Dist="字体合并补全工具-简体中文大字库-$VERSION" \
_dist="WarFontMerger-SC3-$VERSION" \
_cjk="Nowar-Sans-CJK-SC-Medium" \
_latin="Nowar-Sans-LCG-Medium" \
package_sc
