set_project("vtk_ball")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_rules("mode.debug", "mode.release")
set_config("vs", "2019")
add_requires("vtk")

target("vtk_ball")
    set_rundir("$(projectdir)")
    add_packages("vtk")
    add_includedirs(".")
    add_files("./main.cpp")

