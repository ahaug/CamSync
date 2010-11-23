TEMPLATE = app
TARGET = example6
INCLUDEPATH += ../../include
HEADERS += SoundPlayer.h
SOURCES += example6.cpp SoundPlayer.cpp
LIBS += -lpthread -ljpeg -lpulse-simple -L../.. -lFCam
