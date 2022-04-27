workspace "custos"
    configurations { "debug", "release" }
    kind "ConsoleApp"
    language "C"
    cdialect "C11"
    defines {
        "_POSIX_C_SOURCE=200809L",
        "_XOPEN_SOURCE",
    }
    warnings "Extra"

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "On"

    filter "toolset:clang or gcc"
        enablewarnings { "pedantic", "conversion" }

    filter { "configurations:debug", "toolset:clang or gcc" }
        buildoptions { "-fsanitize=address,undefined", "-fstack-protector" }
        linkoptions { "-fsanitize=address,undefined", "-fstack-protector" }

    include "src"
