#!/bin/bash

cmake . -B build/ -G "MinGW Makefiles" \
	-DCMAKE_BUILD_TYPE="Release" \
	-DCMAKE_INTERPROCEDURAL_OPTIMIZATION:BOOL="ON" \
	-DCMAKE_C_COMPILER="aarch64-w64-mingw32-clang" \
	-DCMAKE_C_FLAGS="-static -s" \
	-DCMAKE_CXX_COMPILER="aarch64-w64-mingw32-clang++" \
	-DCMAKE_CXX_FLAGS="-static -s"
cmake --build build/ -j$(nproc)

source build/config/config.sh
VERSION=$VERSION-winarm

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

_Dist="字型合併補全工具-繁體中文大字庫-$VERSION" \
_dist="WarFontMerger-TC-$VERSION" \
_cjk="Nowar-Sans-CJK-TC-Medium" \
_latin="Nowar-Sans-LCG-Medium" \
package_tc

_Dist="字型合併補全工具-傳統字形大字庫-$VERSION" \
_dist="WarFontMerger-Classic-$VERSION" \
_cjk="Nowar-Sans-CJK-CL-Medium" \
_latin="Nowar-Sans-LCG-Medium" \
package_tc
