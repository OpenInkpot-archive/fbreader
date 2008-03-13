include $(ROOTDIR)/makefiles/arch/unix.mk

ifeq "$(INSTALLDIR)" ""
  INSTALLDIR=/usr
endif
IMAGEDIR = $(INSTALLDIR)/share/pixmaps
APPIMAGEDIR = $(INSTALLDIR)/share/pixmaps/%APPLICATION_NAME%

CC = arm-linux-gcc
AR = arm-linux-ar rsu
LD = arm-linux-g++

CFLAGS = -pipe -fno-exceptions -Wall -Wno-ctor-dtor-privacy -W -DLIBICONV_PLUG -DARM -DXMLCONFIGHOMEDIR=\"/home\"
LDFLAGS =

ifeq "$(UI_TYPE)" "nanox"
  UILIBS = -L$(ROOTDIR)/v3/arm/lib -lfreetype -lrt -lpthread -ljpeg -lpng
  NXINCLUDE = -I$(ROOTDIR)/v3/include/
  EXTERNALINCLUDE = -I$(ROOTDIR)/v3/include/
  ZLSHARED = no
endif

XML_LIB = -L$(ROOTDIR)/v3/arm/lib -lexpat
ARCHIVER_LIB = -L$(ROOTDIR)/v3/arm/lib -lzlib -lbz2

RM = rm -rvf
RM_QUIET = rm -rf
