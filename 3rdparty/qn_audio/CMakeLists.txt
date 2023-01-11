project(qn_audio)
cmake_minimum_required( VERSION 3.10 )


#########################

aux_source_directory(./src DIR_SRCS)

link_directories(${PROJECT_SOURCE_DIR}/lib/x86_64)

include_directories(${PROJECT_SOURCE_DIR}/include )

add_library( ${PROJECT_NAME} SHARED ${DIR_SRCS} )

##############################################

########################

target_link_libraries(${PROJECT_NAME} pthread qn_rtc dl m)


# set the output dir
INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/lib
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/lib
)
