if(NOT DEFINED et_root)
  message(ERROR "et_root is not defined, this sources must not be built separately")
endif()

set(et_ext_root ${CMAKE_CURRENT_LIST_DIR})

function(configure_project project)
  message(STATUS "Configuring ${project}...")
  target_include_directories(${project} PUBLIC "${et_root}/sources/")
  target_include_directories(${project} PUBLIC "${et_ext_root}/sources/")
  target_compile_options(${project} PRIVATE -DNOMINMAX)
endfunction(configure_project)

function(add_source_folder folder)
  set(folder_path "${et_ext_root}/sources/et-ext/${folder}/*.*")
  add_library(${folder})
  file(GLOB_RECURSE folder_sources ${folder_path})
  target_sources(${folder} PRIVATE ${folder_sources})
  configure_project(${folder})
  set_property(TARGET ${folder} PROPERTY FOLDER "et-ext")
endfunction(add_source_folder)

add_source_folder(scene2d)

target_link_libraries(scene2d PUBLIC freetype)