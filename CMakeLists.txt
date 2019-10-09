if(NOT DEFINED et_root)
  message(ERROR "et_root is not defined, this sources must not be built separately")
endif()

set(et_ext_root ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE folder_sources "${et_ext_root}/sources/et-ext/scene2d/*.*")

add_library(scene2d ${folder_sources})
target_include_directories(scene2d PUBLIC "${et_root}/sources/")
target_include_directories(scene2d PUBLIC "${et_ext_root}/sources/")
target_include_directories(scene2d PUBLIC "${et_root}/sources/external/freetype/include")
target_compile_options(scene2d PRIVATE -DNOMINMAX)
set_property(TARGET scene2d PROPERTY FOLDER "et-ext")

link_precompiled(scene2d freetype "d")
