if(NOT DEFINED et_root)
  message(ERROR "et_root is not defined, this sources must not be built separately")
endif()

set(et_ext_root ${CMAKE_CURRENT_LIST_DIR})
add_source_folder("scene2d" "${et_ext_root}")

# link freetype
target_include_directories(scene2d PUBLIC "${et_root}/include/external/freetype/include")
link_precompiled(scene2d freetype "")

# link core to make et headers visible
target_link_libraries(scene2d PUBLIC core)
