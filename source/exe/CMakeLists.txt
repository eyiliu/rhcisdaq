find_package(ROOT REQUIRED COMPONENTS RIO Tree)


add_executable(rhcisctrl regctrl_main.cc)
target_link_libraries(rhcisctrl PRIVATE datactrl-fe mycommon stdc++fs ROOT::RIO ROOT::Tree)

add_executable(rhcisdump dmarx_dump.cc)
target_link_libraries(rhcisdump PRIVATE datactrl-fe mycommon stdc++fs)


add_executable(rhcisdump_test dmarx_test.cc)
target_link_libraries(rhcisdump_test PRIVATE datactrl-fe mycommon stdc++fs)


# add_executable(dmarx_dump dmarx_dump.cc)

# target_include_directories(dmarx_dump
#   PRIVATE
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
#   )
# target_link_libraries(dmarx_dump PRIVATE mycommon datactrl-fe stdc++fs Threads::Threads)


install(TARGETS rhcisctrl rhcisdump rhcisdump_test
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )

