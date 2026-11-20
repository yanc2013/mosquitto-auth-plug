# ==========================================
# 5. tools/np and test targets
#    pbkdf2 test executable
# ==========================================
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pbkdf2-check-test.c")
    message(FATAL_ERROR "Missing tests/pbkdf2-check-test.c")
endif()

add_executable(pbkdf2-check-test
    pbkdf2-check-test.c
    ${PROJECT_SOURCE_DIR}/src/common/pbkdf2-check.c
    ${PROJECT_SOURCE_DIR}/src/common/base64.c
)
set_target_properties(pbkdf2-check-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
target_include_directories(pbkdf2-check-test PRIVATE
    ${PROJECT_SOURCE_DIR}/src/common
    ${OPENSSLDIR}/include
)
target_link_directories(pbkdf2-check-test PRIVATE ${OPENSSLDIR}/lib)
target_link_libraries(pbkdf2-check-test PRIVATE crypto)

# In tests/CMakeLists.txt
install(TARGETS pbkdf2-check-test RUNTIME DESTINATION tests)

# Register with CTest after confirming the executable supports unattended execution.
