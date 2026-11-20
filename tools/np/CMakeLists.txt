# ==========================================
# 5. tools/np and test targets
#    # np utility
# ==========================================
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/np.c")
    message(FATAL_ERROR "Missing tools/np/np.c")
endif()

add_executable(np
    np.c
    ${PROJECT_SOURCE_DIR}/src/common/base64.c
)
set_target_properties(np PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools/np"
)
target_include_directories(np PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/common
    ${OPENSSLDIR}/include
)
target_link_directories(np PRIVATE ${OPENSSLDIR}/lib)
target_link_libraries(np PRIVATE crypto)

# In tools/np/CMakeLists.txt
install(TARGETS np RUNTIME DESTINATION tools)