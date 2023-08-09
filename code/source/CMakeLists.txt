add_subdirectory(http)
add_subdirectory(server)

aux_source_directory(. MSV_SRCS)

add_executable(msv ${MSV_SRCS})

target_link_libraries(msv http server mysqlclient)