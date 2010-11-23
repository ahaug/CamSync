#-----------------------------
# QMake build file for fcamera

TEMPLATE = app
TARGET = fcamera

# If you're compiling fcamera without installing fcam-dev,
# (so either you're using it from the Nokia Qt SDK/MADDE, or you just downloaded the zip into scratchbox)
# rename fcamera.local.template.pro to fcamera.local.pro, and adjust its paths if your organization doesn't match
# the fcam Maemo-garage repository defaults
!include( fcamera.local.pro ) {
  message("!! No fcamera.local.pro found, assuming fcam-dev is installed. If this isn't true, please copy fcamera.local.template.pro to fcamera.local.pro, and adjust the path to point to where your fcam files are, if you're not using the default Maemo garage organization for FCam.")
}

#-----------------------------
# Source and header files

HEADERS += Viewfinder.h ThumbnailView.h ImageItem.h OverlayWidget.h CameraThread.h ScrollArea.h CameraParameters.h SettingsTree.h AdjustmentWidget.h SplashDialog.h ExtendedSettings.h SoundPlayer.h UserDefaults.h VisualizationWidget.h PanicHandler.h LEDBlinker.h \
    src/CamSync.h
SOURCES += Viewfinder.cpp ThumbnailView.cpp ImageItem.cpp OverlayWidget.cpp CameraThread.cpp ScrollArea.cpp CameraParameters.cpp AdjustmentWidget.cpp SettingsTree.cpp SplashDialog.cpp ExtendedSettings.cpp SoundPlayer.cpp UserDefaults.cpp VisualizationWidget.cpp PanicHandler.cpp LEDBlinker.cpp \
    src/CamSync.cpp
SOURCES += FCamera.cpp 

#-----------------------------
# Source and header paths

VPATH += src
DEPENDPATH += .

#-----------------------------
# Libraries

LIBS += -pthread -lFCam -ljpeg -lpulse-simple

#-----------------------------
# Build destination paths
OBJECTS_DIR = build
MOC_DIR     = build
UI_DIR      = build

DESTDIR     = build

#-----------------------------
# QT Configuration

CONFIG += release warn_on
QT += network thread
CONFIG += qt
QMAKE_CXXFLAGS += -mfpu=neon -mfloat-abi=softfp

#-----------------------------
# Packaging setup

INSTALLS    += target
target.path  = /usr/bin/

INSTALLS    += desktop
desktop.path  = /usr/share/applications/hildon
desktop.files  = data/fcamera.desktop

INSTALLS    += service
service.path  = /usr/share/dbus-1/services
service.files  = data/fcamera.service

INSTALLS    += icon64
icon64.path  = /usr/share/pixmaps
icon64.files  = data/icons/64x64/fcamera.png

#
# Targets for debian source and binary package creation

debian-src.commands = dpkg-buildpackage -S -r -us -uc -d -I'\\.svn';
debian-src.commands += mv ../$(QMAKE_TARGET)_*{.dsc,.tar.gz,_source.changes} .
debian-bin.commands = dpkg-buildpackage -b -r -uc -d;
debian-bin.commands += mv ../$(QMAKE_TARGET)_*_armel.{deb,changes} .

debian-all.depends = debian-src debian-bin

#-----------------------------
# Clean all but Makefile

compiler_clean.commands = -$(DEL_FILE) $(TARGET)

#-----------------------------
# MADDE remote commands

go.commands  = mad remote -r N900 send $(TARGET); 
go.commands += mad remote -r N900 run $(QMAKE_TARGET)
go.depends = $(TARGET)

gobug.commands = mad remote -r N900 send $(TARGET); 
gobug.commands += mad remote -r N900 debug $(QMAKE_TARGET)
gobug.depends = $(TARGET)

#-----------------------------
# Add all of the xtra QMAKE targets from above

QMAKE_EXTRA_TARGETS += debian-all debian-src debian-bin compiler_clean go gobug
