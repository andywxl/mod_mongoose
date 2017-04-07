include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_mongoose

mod_LTLIBRARIES = mod_mongoose.la
mod_mongoose_la_SOURCES  = mod_mongoose.c mongoose.c
mod_mongoose_la_CFLAGS   = $(AM_CFLAGS)
mod_mongoose_la_CPPFLAGS   = $(CURL_CFLAGS) $(AM_CPPFLAGS)
mod_mongoose_la_LIBADD   = $(switch_builddir)/libippbx.la
mod_mongoose_la_LDFLAGS  = $(CURL_LIBS) -avoid-version -module -no-undefined -shared
