
bin_PROGRAMS = mx5000d

mx5000d_SOURCES = mx5000d.c \
	input_translate.c \
	input_translate.h

mx5000d_LDADD = ../libmx5000/libmx5000.la $(MX5000D_LIBS)

AM_CPPFLAGS = -I.. $(MX5000D_CFLAGS)

AM_CFLAGS = -Wall

