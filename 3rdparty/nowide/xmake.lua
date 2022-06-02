target("nowide")
    if is_os("windows") then
        set_kind("shared")
        add_files("src/*.cpp")
    else
        set_kind("phony")
    end
    add_includedirs(".", {public = true})
