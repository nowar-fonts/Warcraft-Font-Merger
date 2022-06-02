target("otfcc")
    set_kind("shared")
    add_files("src/otfcc.cpp")
    add_includedirs("include", {public = true})
