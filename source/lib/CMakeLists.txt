add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/reg_cmd_list_json.hh
  COMMAND echo "create ${CMAKE_CURRENT_BINARY_DIR}/include/reg_cmd_list_json.hh"
  COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/cpprawstring.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/reg_cmd_list_json.hh ${PROJECT_SOURCE_DIR}/source/lib/misc/reg_cmd_list.json
  )



add_custom_target(RegCmd_custom ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/reg_cmd_list_json.hh)
add_library(headerFileRegCmd INTERFACE)
add_dependencies(headerFileRegCmd RegCmd_custom)
target_include_directories(headerFileRegCmd INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

aux_source_directory(src LIB_SRC)
add_library(datactrl-fe STATIC ${LIB_SRC} )
target_link_libraries(datactrl-fe PUBLIC mycommon headerFileRegCmd Threads::Threads)

target_include_directories(datactrl-fe
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
  )

install(TARGETS datactrl-fe headerFileRegCmd
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
