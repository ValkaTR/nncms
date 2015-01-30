
add_executable(${PROJECT_NAME}
    log.c cfg.c mime.c view.c file.c acl.c database.c filter.c
    main.c user.c group.c template.c form.c field.c table.c post.c cache.c smart.c node.c i18n.c
    strlcat.c strlcpy.c
    gettok.c strnstr.c recursive_mkdir.c asprintf.c dirname_r.c
    memory.c strdup.c ban.c sha2.c
    canonicalize.c xxx.c
)


find_package(LibM REQUIRED)
find_package(Glib REQUIRED)
find_package(Sqlite3 REQUIRED)
find_package(Lua REQUIRED)
find_package(PTHREAD REQUIRED)
find_package(FastCGI REQUIRED)

pkg_check_modules(GMIME gmime-2.6)
add_definitions(${GMIME_CFLAGS_OTHER})

pkg_check_modules(IMAGEMAGICK MagickWand)
add_definitions(${IMAGEMAGICK_CFLAGS_OTHER})

include_directories(
    ${LibM_INCLUDE_DIRS}
    ${Glib_INCLUDE_DIRS}
    ${GMIME_INCLUDE_DIRS}
    ${IMAGEMAGICK_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIR}
    ${LUA_INCLUDE_DIRS}
    ${PTHREAD_INCLUDE_DIR}
    ${FCGI_INCLUDE_DIR}
)

target_link_libraries(
    ${PROJECT_NAME}
    ${LibM_LIBRARIES}
    ${Glib_LIBRARIES}
    ${GMIME_LIBRARIES}
    ${IMAGEMAGICK_LIBRARIES}
    ${SQLITE3_LIBRARY}
    ${LUA_LIBRARIES}
    ${PTHREAD_LIBRARIES}
    ${FCGI_LIBRARY}
)

CONFIGURE_FILE(config.h.cmake config.h)
