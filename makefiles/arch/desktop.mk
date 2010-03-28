include $(ROOTDIR)/makefiles/arch/unix.mk

ifeq "$(INSTALLDIR)" ""
  INSTALLDIR=/usr
endif
IMAGEDIR = $(INSTALLDIR)/share/pixmaps
APPIMAGEDIR = $(INSTALLDIR)/share/pixmaps/%APPLICATION_NAME%

override AR += rsu

#CC = gcc
#AR = ar rsu
#LD = g++

CFLAGS = -pipe -fno-exceptions -Wall -Wno-ctor-dtor-privacy -W -DLIBICONV_PLUG
LDFLAGS =

ifeq "$(UI_TYPE)" "qt"
  MOC = moc-qt3
  QTINCLUDE = -I /usr/include/qt3
else
  MOC = moc-qt4
  QTINCLUDE = -I /usr/include/qt4
endif

GTKINCLUDE = $(shell pkg-config --cflags gtk+-2.0 libpng xft)

ifeq "$(UI_TYPE)" "qt"
  UILIBS = -lqt-mt
endif

ifeq "$(UI_TYPE)" "qt4"
  UILIBS = -lQtGui
endif

ifeq "$(UI_TYPE)" "gtk"
  UILIBS = $(shell pkg-config --libs gtk+-2.0) -lpng -ljpeg
endif

ifeq "$(UI_TYPE)" "ewl"
  UILIBS = $(shell pkg-config --libs xcb xcb-image xcb-atom xcb-randr evas efreet pango pangoft2 glib-2.0 libpng freetype2 ecore ecore-evas libchoicebox libeoi) -ljpeg -lungif -lrt
  EWLINCLUDE = $(shell pkg-config --cflags xcb xcb-image xcb-randr xcb-atom evas efreet pango pangoft2 glib-2.0 libpng freetype2 ecore ecore-evas libchoicebox libeoi)
  ZLSHARED = no
endif

XML_LIB = -lexpat
ARCHIVER_LIB = -lz -lbz2

RM = rm -rvf
RM_QUIET = rm -rf
