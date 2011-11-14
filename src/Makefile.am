noinst_PROGRAMS = sqlite3 sha2-test spawn-fcgi
bin_PROGRAMS = nncms

# -Wall -pedantic
AM_CPPFLAGS = -std=gnu99 -I/usr/include/libxml2/ -I/usr/include/fastcgi/

nncms_SOURCES = log.c cfg.c mime.c view.c file.c acl.c database.c filter.c
nncms_SOURCES += main.c sha2.c user.c template.c post.c cache.c smart.c
nncms_SOURCES += strlcat.c strlcpy.c
nncms_SOURCES += gettok.c strnstr.c recursive_mkdir.c asprintf.c dirname_r.c
nncms_SOURCES += memory.c tree.c strdup.c ban.c

#nncms_SOURCES += html_parser.c html_tree.c html_utils.c

nncms_LDADD = -L/usr/local/lib $(MAGICKWAND_LIBS) $(SQLITE3_LIBS) $(LUA_LIBS) -lrt

nncms_SOURCES += log.h cfg.h mime.h view.h file.h acl.h database.h filter.h
nncms_SOURCES += main.h sha2.h template.h user.h post.h cache.h smart.h
nncms_SOURCES += strlcat.h strlcpy.h
nncms_SOURCES += gettok.h strnstr.h recursive_mkdir.h asprintf.h dirname_r.h
nncms_SOURCES += memory.h tree.h strdup.h ban.h

#nncms_SOURCES += html_tree.h html_utils.h

sqlite3_SOURCES = sqlient.c
sqlite3_LDADD = $(SQLITE3_LIBS)

sha2_test_SOURCES = sha2_test.c sha2.c
sha2_test_SOURCES += sha2.h

spawn_fcgi_SOURCES = spawn-fcgi.c
