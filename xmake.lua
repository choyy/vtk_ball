set_project("vtkball")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_rules("mode.debug", "mode.release")
set_config("vs", "2019")
set_languages("cxx17")
add_cxxflags("/EHsc") -- boost 需要
add_requires(
    "vtk",
    "boost", {configs = {
                program_options  = true,
             }}
)

target("vtkball")
    set_rundir("$(projectdir)")
    add_packages(
        "vtk",
        "boost"
    )
    add_includedirs(".")
    add_files("./main.cpp")

