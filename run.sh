#!/bin/bash

set -euxo pipefail

xmake b test-sfnt
xmake r test-sfnt $PWD/test-fonts/AND-Regular.otf
xmake r test-sfnt $PWD/test-fonts/AND-Regular.ttf
