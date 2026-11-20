
option(BUILD_NP_TOOL "Build np utility" ON)

if(BUILD_NP_TOOL)
    add_subdirectory(np)
endif()

# Future tools:
# option(BUILD_XX_TOOL "Build xx utility" ON)
# if(BUILD_XX_TOOL)
#     add_subdirectory(xx)
# endif()
