if(NOT DEFINED et_root)
  message(ERROR "et_root is not defined, this sources must not be built separately")
endif()

set(et_ext_root ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE headers "${et_ext_root}/include/et-ext/scene2d/*.*")
file(GLOB_RECURSE sources "${et_ext_root}/sources/et-ext/scene2d/*.*")

add_library(scene2d ${headers} ${sources})
target_compile_options(scene2d PRIVATE -DNOMINMAX)
target_compile_options(scene2d PRIVATE "/Zl")

set_property(TARGET scene2d PROPERTY STATIC_LIBRARY_FLAGS "/NODEFAULTLIB")

target_include_directories(scene2d PUBLIC "${et_root}/include/")
target_include_directories(scene2d PUBLIC "${et_root}/include/external/freetype/include")
target_include_directories(scene2d PUBLIC "${et_ext_root}/include/")

set_property(TARGET scene2d PROPERTY FOLDER "et-ext")

link_precompiled(scene2d freetype "")
