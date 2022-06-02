target("test-sfnt")
    set_kind("binary")
    add_files("main.cpp")
    add_deps("otfcc")
