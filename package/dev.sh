#!/bin/bash

cmake . -B build/ \
	-DCMAKE_BUILD_TYPE="Release"
cmake --build build/ -j$(nproc)

source build/config/config.sh
VERSION=$VERSION-dev

source package/common.sh

export _platform="unix"
export _archive=""
export _url_extension="desktop"
export _script_extension="sh"
export _binary_suffix=""

_Dist="字体合并补全工具-简体中文大字库-$VERSION" \
_cjk="Nowar-Sans-CJK-SC-Medium" \
_latin="Nowar-Sans-LCG-Medium" \
package_sc
