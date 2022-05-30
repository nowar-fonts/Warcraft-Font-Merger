#!/bin/bash

cmake . -B build/ \
	-DCMAKE_BUILD_TYPE="Release" \
	-DCMAKE_INTERPROCEDURAL_OPTIMIZATION:BOOL="ON" \
	-DCMAKE_C_COMPILER="i686-w64-mingw32-gcc" \
	-DCMAKE_C_FLAGS="-static -s -Wl,--large-address-aware" \
	-DCMAKE_CXX_COMPILER="i686-w64-mingw32-g++" \
	-DCMAKE_CXX_FLAGS="-static -s -Wl,--large-address-aware"
cmake --build build/ -j$(nproc)

source build/config/config.sh
VERSION=$VERSION-windows-32

source package/common.sh

export _platform="windows"
export _archive="7z"
export _url_extension="url"
export _binary_suffix=".exe"

_Dist="字体合并补全工具-压缩字库-$VERSION" \
_dist="WarFontMerger-XS-$VERSION" \
_cjk="WFM-Sans-CJK-XS-Regular" \
_latin="WFM-Sans-LCG-Apache-Regular" \
package_sc
