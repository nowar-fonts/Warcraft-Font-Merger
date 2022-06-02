target("merge-otd")
    set_kind("binary")
    add_files("*.cpp")

    add_deps("clipp", "json", "nowide")
