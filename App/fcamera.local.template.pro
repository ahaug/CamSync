# Local fcamera project settings - where is fcam-dev?
FCAM_PATH = ../../fcam-dev/trunk

# Shouldn't need to modify the below
INCLUDEPATH += $$FCAM_PATH/include 
LIBS += -L$$FCAM_PATH 
TARGETDEPS += $$FCAM_PATH/libFCam.a
