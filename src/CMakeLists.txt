# ==========================================
# 1. Paths and configuration options (from config.mk)
# ==========================================
set(MOSQUITTO_SRC "${PROJECT_SOURCE_DIR}/contrib/mosquitto-2.0.11"
    CACHE PATH "Path to Mosquitto source/header directory")
set(OPENSSLDIR "/usr"
    CACHE PATH "OpenSSL installation directory")

option(BACKEND_CDB "Enable CDB backend" ON)
option(BACKEND_HTTP "Enable HTTP backend" ON)
option(BACKEND_JWT "Enable JWT backend" ON)
option(BACKEND_FILES "Enable Files backend" ON)

option(BACKEND_MYSQL "Enable MySQL/MariaDB backend" OFF)
option(BACKEND_SQLITE "Enable SQLite backend" OFF)
option(BACKEND_REDIS "Enable Redis backend" OFF)
option(BACKEND_POSTGRES "Enable PostgreSQL backend" OFF)
option(BACKEND_LDAP "Enable LDAP backend" OFF)
option(BACKEND_MONGO "Enable MongoDB backend" OFF)
option(BACKEND_MEMCACHED "Enable Memcached backend" OFF)

option(SUPPORT_DJANGO_HASHERS "Support Django hashers algorithm names" OFF)


# ==========================================
# 2. Core sources and include/link paths
# ==========================================
set(AUTH_PLUG_SOURCES
    ${PROJECT_SOURCE_DIR}/src/auth-plug.c
    ${PROJECT_SOURCE_DIR}/src/common/base64.c
    ${PROJECT_SOURCE_DIR}/src/common/pbkdf2-check.c
    ${PROJECT_SOURCE_DIR}/src/common/log.c
    ${PROJECT_SOURCE_DIR}/src/common/envs.c
    ${PROJECT_SOURCE_DIR}/src/common/hash.c
    ${PROJECT_SOURCE_DIR}/src/backend/be-psk.c
    ${PROJECT_SOURCE_DIR}/src/backend/backends.c
    ${PROJECT_SOURCE_DIR}/src/cache/cache.c
    ${PROJECT_SOURCE_DIR}/src/gc/client-gc.c
)

set(AUTH_PLUG_INCLUDE_DIRS
    ${MOSQUITTO_SRC}
    ${MOSQUITTO_SRC}/include
    ${MOSQUITTO_SRC}/src
    ${MOSQUITTO_SRC}/lib
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/backend
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/cache
    ${PROJECT_SOURCE_DIR}/src/gc
    ${PROJECT_SOURCE_DIR}/tools/np
    ${OPENSSLDIR}/include
)

set(AUTH_PLUG_DEFINES DEBUG=1)
set(AUTH_PLUG_LIBS "")
set(AUTH_PLUG_LINK_DIRS
    ${MOSQUITTO_SRC}/lib
    ${OPENSSLDIR}/lib
)
set(SELECTED_BACKENDS "")

if(SUPPORT_DJANGO_HASHERS)
    list(APPEND AUTH_PLUG_DEFINES SUPPORT_DJANGO_HASHERS)
endif()

# ==========================================
# 3. Backend selection and dependencies
# ==========================================
if(BACKEND_CDB)
    list(APPEND SELECTED_BACKENDS "CDB")
    list(APPEND AUTH_PLUG_DEFINES BE_CDB)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-cdb.c)

    set(CDB_DIR "${PROJECT_SOURCE_DIR}/contrib/tinycdb-0.78")
    set(CDB_OUT_DIR "${CMAKE_BINARY_DIR}/tinycdb")

    file(GLOB TINYCDB_SRCS "${CDB_DIR}/cdb_*.c")
    list(REMOVE_ITEM TINYCDB_SRCS "${CDB_DIR}/cdb.c")

    add_library(tinycdb STATIC ${TINYCDB_SRCS})
    set_target_properties(tinycdb PROPERTIES
        OUTPUT_NAME "cdb"
        POSITION_INDEPENDENT_CODE ON
        ARCHIVE_OUTPUT_DIRECTORY "${CDB_OUT_DIR}"
    )

    add_executable(cdb "${CDB_DIR}/cdb.c")
    set_target_properties(cdb PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CDB_OUT_DIR}"
    )
    target_link_libraries(cdb PRIVATE tinycdb)

    list(APPEND AUTH_PLUG_INCLUDE_DIRS "${CDB_DIR}")
    list(APPEND AUTH_PLUG_LIBS tinycdb)
endif()

if(BACKEND_FILES)
    list(APPEND SELECTED_BACKENDS "Files")
    list(APPEND AUTH_PLUG_DEFINES BE_FILES)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-files.c)
endif()

if(BACKEND_HTTP)
    list(APPEND SELECTED_BACKENDS "HTTP")
    list(APPEND AUTH_PLUG_DEFINES BE_HTTP)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-http.c)
    list(APPEND AUTH_PLUG_LIBS curl)
endif()

if(BACKEND_JWT)
    list(APPEND SELECTED_BACKENDS "JWT")
    list(APPEND AUTH_PLUG_DEFINES BE_JWT)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-jwt.c)
    list(APPEND AUTH_PLUG_LIBS curl)
endif()

if(BACKEND_MYSQL)
    list(APPEND SELECTED_BACKENDS "MySQL")
    list(APPEND AUTH_PLUG_DEFINES BE_MYSQL)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-mysql.c)

    find_program(MYSQL_CONFIG_EXECUTABLE mysql_config)
    if(MYSQL_CONFIG_EXECUTABLE)
        execute_process(
            COMMAND "${MYSQL_CONFIG_EXECUTABLE}" --cflags
            OUTPUT_VARIABLE MYSQL_CFLAGS_RAW
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND "${MYSQL_CONFIG_EXECUTABLE}" --libs
            OUTPUT_VARIABLE MYSQL_LIBS_RAW
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        separate_arguments(MYSQL_CFLAGS_LIST UNIX_COMMAND "${MYSQL_CFLAGS_RAW}")
        separate_arguments(MYSQL_LIBS_LIST UNIX_COMMAND "${MYSQL_LIBS_RAW}")
        foreach(flag IN LISTS MYSQL_CFLAGS_LIST)
            if(flag MATCHES "^-I(.+)")
                list(APPEND AUTH_PLUG_INCLUDE_DIRS "${CMAKE_MATCH_1}")
            endif()
        endforeach()
        list(APPEND AUTH_PLUG_LIBS ${MYSQL_LIBS_LIST})
    else()
        list(APPEND AUTH_PLUG_LIBS mariadb)
    endif()
endif()

if(BACKEND_SQLITE)
    list(APPEND SELECTED_BACKENDS "SQLite")
    list(APPEND AUTH_PLUG_DEFINES BE_SQLITE)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-sqlite.c)
    list(APPEND AUTH_PLUG_LIBS sqlite3)
endif()

if(BACKEND_REDIS)
    list(APPEND SELECTED_BACKENDS "Redis")
    list(APPEND AUTH_PLUG_DEFINES BE_REDIS)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-redis.c)
    list(APPEND AUTH_PLUG_INCLUDE_DIRS /usr/local/include/hiredis)
    list(APPEND AUTH_PLUG_LINK_DIRS /usr/local/lib)
    list(APPEND AUTH_PLUG_LIBS hiredis)
endif()

if(BACKEND_MEMCACHED)
    list(APPEND SELECTED_BACKENDS "Memcached")
    list(APPEND AUTH_PLUG_DEFINES BE_MEMCACHED)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-memcached.c)
    list(APPEND AUTH_PLUG_INCLUDE_DIRS /usr/local/include/libmemcached)
    list(APPEND AUTH_PLUG_LINK_DIRS /usr/local/lib)
    list(APPEND AUTH_PLUG_LIBS memcached)
endif()

if(BACKEND_POSTGRES)
    list(APPEND SELECTED_BACKENDS "PostgreSQL")
    list(APPEND AUTH_PLUG_DEFINES BE_POSTGRES)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-postgres.c)

    find_program(PG_CONFIG_EXECUTABLE pg_config)
    if(PG_CONFIG_EXECUTABLE)
        execute_process(
            COMMAND "${PG_CONFIG_EXECUTABLE}" --includedir
            OUTPUT_VARIABLE PG_INC_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND "${PG_CONFIG_EXECUTABLE}" --libdir
            OUTPUT_VARIABLE PG_LIB_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        list(APPEND AUTH_PLUG_INCLUDE_DIRS "${PG_INC_DIR}")
        list(APPEND AUTH_PLUG_LINK_DIRS "${PG_LIB_DIR}")
    endif()
    list(APPEND AUTH_PLUG_LIBS pq)
endif()

if(BACKEND_LDAP)
    list(APPEND SELECTED_BACKENDS "LDAP")
    list(APPEND AUTH_PLUG_DEFINES BE_LDAP)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-ldap.c)
    list(APPEND AUTH_PLUG_LIBS ldap lber)
endif()

if(BACKEND_MONGO)
    list(APPEND SELECTED_BACKENDS "MongoDB")
    list(APPEND AUTH_PLUG_DEFINES BE_MONGO)
    list(APPEND AUTH_PLUG_SOURCES ${PROJECT_SOURCE_DIR}/src/backend/be-mongo.c)

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(MONGOC QUIET libmongoc-1.0 libbson-1.0)
        if(MONGOC_FOUND)
            list(APPEND AUTH_PLUG_INCLUDE_DIRS ${MONGOC_INCLUDE_DIRS})
            list(APPEND AUTH_PLUG_LINK_DIRS ${MONGOC_LIBRARY_DIRS})
        endif()
    endif()
    list(APPEND AUTH_PLUG_INCLUDE_DIRS /usr/local/include)
    list(APPEND AUTH_PLUG_LINK_DIRS /usr/local/lib)
    list(APPEND AUTH_PLUG_LIBS mongoc-1.0 bson-1.0)
endif()

list(REMOVE_DUPLICATES AUTH_PLUG_INCLUDE_DIRS)
list(REMOVE_DUPLICATES AUTH_PLUG_LINK_DIRS)
list(REMOVE_DUPLICATES AUTH_PLUG_LIBS)

# ==========================================
# 4. Shared library build (build/auth-plug.so)
# ==========================================
add_library(auth-plug SHARED ${AUTH_PLUG_SOURCES})

set_target_properties(auth-plug PROPERTIES
    PREFIX ""
    OUTPUT_NAME "auth-plug"
    SUFFIX ".so"
)

target_include_directories(auth-plug PRIVATE ${AUTH_PLUG_INCLUDE_DIRS})
target_compile_definitions(auth-plug PRIVATE ${AUTH_PLUG_DEFINES})
target_link_directories(auth-plug PRIVATE ${AUTH_PLUG_LINK_DIRS})

find_package(Threads REQUIRED)
target_link_libraries(auth-plug PRIVATE
    ${AUTH_PLUG_LIBS}
    crypto
    mosquitto
    Threads::Threads
)

if(UNIX AND NOT APPLE)
    target_compile_options(auth-plug PRIVATE -fPIC -Wall -Werror)
elseif(APPLE)
    target_compile_options(auth-plug PRIVATE -fPIC -Wall)
    target_link_options(auth-plug PRIVATE -undefined dynamic_lookup)
endif()

# In src/CMakeLists.txt
install(TARGETS auth-plug LIBRARY DESTINATION auth-plug)

# ==========================================
# 7. Print configuration summary
# ==========================================
string(JOIN " " SELECTED_BACKENDS_STR ${SELECTED_BACKENDS})

set(_PREVIEW_CFLAGS "")
foreach(d IN LISTS AUTH_PLUG_DEFINES)
    list(APPEND _PREVIEW_CFLAGS "-D${d}")
endforeach()
foreach(inc IN LISTS AUTH_PLUG_INCLUDE_DIRS)
    list(APPEND _PREVIEW_CFLAGS "-I${inc}")
endforeach()
if(UNIX AND NOT APPLE)
    list(APPEND _PREVIEW_CFLAGS "-fPIC" "-Wall" "-Werror")
elseif(APPLE)
    list(APPEND _PREVIEW_CFLAGS "-fPIC" "-Wall")
endif()
string(JOIN " " _PREVIEW_CFLAGS_STR ${_PREVIEW_CFLAGS})

set(_PREVIEW_LDFLAGS "")
foreach(ldir IN LISTS AUTH_PLUG_LINK_DIRS)
    list(APPEND _PREVIEW_LDFLAGS "-L${ldir}")
endforeach()
if(APPLE)
    list(APPEND _PREVIEW_LDFLAGS "-undefined" "dynamic_lookup")
endif()
string(JOIN " " _PREVIEW_LDFLAGS_STR ${_PREVIEW_LDFLAGS})

set(_PREVIEW_LDADD "")
foreach(lib IN LISTS AUTH_PLUG_LIBS ${CMAKE_THREAD_LIBS_INIT} crypto mosquitto)
    if("${lib}" STREQUAL "tinycdb" AND DEFINED CDB_OUT_DIR)
        list(APPEND _PREVIEW_LDADD "${CDB_OUT_DIR}/libcdb.a")
    elseif(TARGET "${lib}")
        list(APPEND _PREVIEW_LDADD "${lib}")
    elseif(lib MATCHES "^-l")
        list(APPEND _PREVIEW_LDADD "${lib}")
    else()
        list(APPEND _PREVIEW_LDADD "-l${lib}")
    endif()
endforeach()
string(JOIN " " _PREVIEW_LDADD_STR ${_PREVIEW_LDADD})

message(STATUS "Selected backends:          ${SELECTED_BACKENDS_STR}")
message(STATUS "Using mosquitto source dir: ${MOSQUITTO_SRC}")
message(STATUS "OpenSSL install dir:        ${OPENSSLDIR}")
message(STATUS "Build output dir:           ${CMAKE_BINARY_DIR}")
message(STATUS "")
message(STATUS "If you changed the backend selection, you might need to 'rm -rf ${CMAKE_BINARY_DIR}' first")
message(STATUS "")
message(STATUS "CFLAGS:  ${_PREVIEW_CFLAGS_STR}")
message(STATUS "LDFLAGS: ${_PREVIEW_LDFLAGS_STR}")
message(STATUS "LDADD:   ${_PREVIEW_LDADD_STR}")
message(STATUS "")
