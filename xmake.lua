includes("lib/commonlibf4")

set_project("MagPoop")
set_version("1.0.0")
set_license("MIT")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")
add_defines("COMMONLIB_RUNTIMECOUNT=3")

target("MagPoop")
    add_rules("commonlibf4.plugin", {
        name = "MagPoop",
        author = "jarari",
        description = "Animated detachable magazine debris",
        plugin_template = path.join(os.projectdir(), "res/commonlibf4-plugin.cpp.in"),
    })
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")
