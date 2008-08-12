include $(ROOTDIR)/makefiles/arch/unix.mk

ifeq "$(INSTALLDIR)" ""
  INSTALLDIR=/usr
endif
IMAGEDIR = $(INSTALLDIR)/share/pixmaps
APPIMAGEDIR = $(INSTALLDIR)/share/pixmaps/%APPLICATION_NAME%

#CC = arm-linux-gnueabi-gcc
#AR = arm-linux-gnueabi-ar rsu
#LD = arm-linux-gnueabi-g++

override AR += rsu
override LD = arm-linux-gnueabi-g++

CFLAGS = -pipe -fno-exceptions -Wall -Wno-ctor-dtor-privacy -W -DLIBICONV_PLUG -DARM -DXMLCONFIGHOMEDIR=\"/home\"
LDFLAGS =

ifeq "$(UI_TYPE)" "nanox"
  UILIBS = -lrt -lpthread -ljpeg -lpng -lxcb -lxcb-image `arm-linux-gnueabi-pkg-config --libs evas ewl pango pangoft2'
  NXINCLUDE = `arm-linux-gnueabi-pkg-config --cflags evas ewl pango pangoft2`
  EXTERNALINCLUDE =
  ZLSHARED = no
endif

XML_LIB = -lexpat
ARCHIVER_LIB = -lz -lbz2

RM = rm -rvf
RM_QUIET = rm -rf
