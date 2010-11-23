### FCam library Makefile

FCAM_VERSION=1.1.1

## Define project directories
SOURCE_DIR=src
INCLUDE_DIR=include
RELEASE_DIR=release
DEBUG_DIR=debug
BINARY_DIR=bin
LAST_MAKE_TRACKER=.last_make

## Define all general FCam source files
SOURCES =  Action.cpp AutoExposure.cpp AutoFocus.cpp AutoWhiteBalance.cpp AsyncFile.cpp 
SOURCES += Base.cpp Device.cpp Event.cpp Flash.cpp Frame.cpp Image.cpp 
SOURCES += Lens.cpp Shot.cpp Sensor.cpp Time.cpp TagValue.cpp 
SOURCES += processing/DNG.cpp processing/TIFF.cpp processing/TIFFTags.cpp
SOURCES += processing/Dump.cpp processing/JPEG.cpp processing/Demosaic.cpp processing/Color.cpp
SOURCES += Dummy/Sensor.cpp Dummy/Frame.cpp Dummy/Shot.cpp Dummy/Daemon.cpp Dummy/Platform.cpp

## Overall build options
CXXFLAGS += -Wall -I$(INCLUDE_DIR)
CXXFLAGS_RELEASE = -O3
CXXFLAGS_DEBUG = -g -O0 -D DEBUG -D FCAM_DEBUG_LEVEL=$(DEBUG_LEVEL)
CXXFLAGS_ARM = -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
CXXFLAGS_X86 = -msse3 -mfpmath=sse
DEBUG_LEVEL=0

## Let's ask compiler about its target processor
MACH := $(shell $(CXX) -dumpmachine)

## Determine target platform - fairly brittle right now
ifdef OE_QMAKE_CXX
  PLATFORM=f2
  PLATFORM_UC=F2
  ARCH=ARM
else ifneq (,$(findstring arm, $(MACH)))
  PLATFORM=n900
  PLATFORM_UC=N900
  ARCH=ARM
else ifneq (,$(findstring apple, $(MACH)))
  PLATFORM=osx
  PLATFORM_UC=OSX
  ARCH=X86
else ifneq (,$(findstring cygwin, $(MACH)))
  PLATFORM=cygwin
  PLATFORM_UC=CYGWIN
  ARCH=X86
else 
  PLATFORM=x86
  PLATFORM_UC=X86
  ARCH=X86
endif

CXXFLAGS += -D FCAM_PLATFORM_$(PLATFORM_UC) -D FCAM_ARCH_$(ARCH)

## Set up object file names

DEBUG_OBJECTS=$(SOURCES:%.cpp=$(DEBUG_DIR).$(PLATFORM)/%.o)
RELEASE_OBJECTS=$(SOURCES:%.cpp=$(RELEASE_DIR).$(PLATFORM)/%.o)

## Set up platform-specific details

ifeq ($(PLATFORM),f2)
 $(info ** Compiling FCam for F2/Angstrom **)
 # The below redirect a bunch of stuff to /usr/local/angstrom-land
 CXX = $(OE_QMAKE_CXX)
 AR = $(OE_QMAKE_AR)
 CXXFLAGS += $(CXXFLAGS_ARM) $(INCPATH)
 LIBS = -lrt
 SOURCES += F2/Platform.cpp F2/Sensor.cpp F2/Shot.cpp F2/Frame.cpp
 SOURCES += F2/Lens.cpp F2/Flash.cpp 
 SOURCES += F2/EF232LensDatabase.cpp F2/PhidgetDevice.cpp F2/ShutterButton.cpp 
 SOURCES += F2/Daemon.cpp F2/V4L2Sensor.cpp  
 SOURCES += processing/Demosaic_ARM.cpp
endif

ifeq ($(PLATFORM),n900)
 $(info ** Compiling FCam for N900/Maemo **)
 # This is all inside madde/scratchbox, so no odd prefixes are required
 CXX = g++
 AR = ar
 CXXFLAGS += $(CXXFLAGS_ARM)
 LIBS = -lrt
 SOURCES += N900/ButtonListener.cpp N900/Sensor.cpp N900/Daemon.cpp N900/V4L2Sensor.cpp 
 SOURCES += N900/Lens.cpp N900/Flash.cpp N900/Platform.cpp N900/Frame.cpp 
 SOURCES += processing/Demosaic_ARM.cpp
endif

ifeq ($(PLATFORM),osx)
 $(info ** Compiling FCam for OS X/macports **)
 # Assumes macports libraries (libjpeg, etc) exist
 CXX = g++
 AR = ar
 CXXFLAGS += -I /opt/local/include $(CXXFLAGS_X86)
 LIBS = -L/opt/local/lib
endif

ifeq ($(PLATFORM),cygwin)
 $(info ** Compiling FCam for x86/Cygwin **)
 # Basic X86 compile
 CXX = g++
 AR = ar
 CXXFLAGS += $(CXXFLAGS_X86)
 LIBS =	-lrt
endif

ifeq ($(PLATFORM),x86)
 $(info ** Compiling FCam for x86/Posix **)
 # Basic X86 compile
 CXX = g++
 AR = ar
 CXXFLAGS += $(CXXFLAGS_X86)
 LIBS =	-lrt
endif

## Main build targets

all: release 

# Optimized build
.PHONY: release
release: CXXFLAGS += $(CXXFLAGS_RELEASE)
release: BUILD_DIR=$(RELEASE_DIR).$(PLATFORM)
release: OBJECTS=$(RELEASE_OBJECTS)
release: make_dirs $(RELEASE_DIR).$(PLATFORM)/libFCam.a
	@echo '** Release mode **'

# Debug build
.PHONY: debug
debug: CXXFLAGS += $(CXXFLAGS_DEBUG)
debug: BUILD_DIR=$(DEBUG_DIR).$(PLATFORM)
debug: OBJECTS=$(DEBUG_OBJECTS)
debug: make_dirs $(DEBUG_DIR).$(PLATFORM)/libFCam.a
	@echo '** Debug mode **'

# Documentation and distribution
.PHONY: docs
docs:
	doxygen

.PHONY: package
package: release
	-rm FCam.zip
	make -C examples
	doxygen
	svn -R list | zip -@ FCam.zip
	find libFCam.a examples/bin/example? doc | zip -@ FCam.zip

.PHONY: srcpackage
srcpackage:
	-rm FCam.zip
	svn -R list | zip -@ FCam.zip

.PHONY: install
install: release
	mkdir -p $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include/FCam/N900
	mkdir -p $(DESTDIR)/usr/include/FCam/F2
	mkdir -p $(DESTDIR)/usr/include/FCam/Dummy
	mkdir -p $(DESTDIR)/usr/include/FCam/processing
	find include/FCam -name '*.h' -exec cp '{}' '$(DESTDIR)/usr/{}' ';'
	cp libFCam.a $(DESTDIR)/usr/lib

# Build release packages for distribution
deb:
	dpkg-buildpackage -rfakeroot -b -uc

deb-src:
	dpkg-buildpackage -rfakeroot -sa -S -I'\.svn'

# Clean it all up
.PHONY: clean
clean: 
	-rm -f libFCam.a 
	-rm -rf $(RELEASE_DIR).*
	-rm -rf $(DEBUG_DIR).*
	-rm -rf $(BINARY_DIR)
	-rm -f $(LAST_MAKE_TRACKER)

## Subsidiary targets

# Create needed directories
.PHONY: make_dirs
make_dirs:
	@-mkdir -p $(BUILD_DIR)/processing
	@-mkdir -p $(BUILD_DIR)/F2
	@-mkdir -p $(BUILD_DIR)/N900
	@-mkdir -p $(BUILD_DIR)/Dummy
	@-mkdir -p $(BINARY_DIR)

# Release library build
$(RELEASE_DIR).$(PLATFORM)/libFCam.a: $(RELEASE_OBJECTS) $(LAST_MAKE_TRACKER) | make_dirs 
	@echo $(BUILD_DIR) > $(LAST_MAKE_TRACKER)
	@rm -f $@
	$(AR) cq $@ $(OBJECTS)
	@cp $@ .

# Debug library build
$(DEBUG_DIR).$(PLATFORM)/libFCam.a: $(DEBUG_OBJECTS) $(LAST_MAKE_TRACKER) | make_dirs 
	@echo $(BUILD_DIR) > $(LAST_MAKE_TRACKER)
	@rm -f $@
	$(AR) cq $@ $(OBJECTS) 
	@cp $@ .

# To deal with initial run when $(LAST_MAKE_TRACKER) doesn't exist
$(LAST_MAKE_TRACKER):

# Real build rule, with dependency generation by GCC
$(DEBUG_DIR).$(PLATFORM)/%.o $(RELEASE_DIR).$(PLATFORM)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(BUILD_DIR)/$*.d -MT $(BUILD_DIR)/$*.o -c $< -o $@	

# Generated dependency inclusion
-include $(RELEASE_OBJECTS:.o=.d)
-include $(DEBUG_OBJECTS:.o=.d)

### Unit test programs 

## Base FCam tests
TESTS = testImage testDemosaic testDNG testTSQueue testTagValue testFlashLatency
## F2-specific tests
ifeq ($(PLATFORM),F2)
TESTS += testF2 testF2Lens
endif

## Test code libraries
CXXTESTFLAGS= $(CXXFLAGS)
TESTLIBS = $(LIBS) -L. -lFCam -lpthread -ljpeg 

## Test code build targets
tests: $(TESTS)

## Base FCam test targets, works for anything named tests/test*.cpp. For a release version test build, append -r to target name.

test%: tests/test%.cpp debug
	$(CXX) -O3 $(CXXTESTFLAGS) $(CXXFLAGS_DEBUG) -o $(DEBUG_DIR).$(PLATFORM)/test$* $< $(TESTLIBS)
	cp $(DEBUG_DIR).$(PLATFORM)/test$* $(BINARY_DIR)

test%-r: tests/test%.cpp release
	$(CXX) -O3 $(CXXTESTFLAGS) -g $(CXXFLAGS_RELEASE) -o $(RELEASE_DIR).$(PLATFORM)/test$* $< $(TESTLIBS)
	cp $(RELEASE_DIR).$(PLATFORM)/test$* $(BINARY_DIR)

### Utility programs 
UTILS = fcamDngUtil

utils: $(UTILS)

FCAM_DNG_UTIL_VERSION=1.0.1
FCAM_DNG_UTIL_PKG_FILES=license.txt utils/README
ifeq ($(PLATFORM),cygwin)
FCAM_DNG_UTIL_PKG_FILES+=/bin/cyggcc_s-1.dll /bin/cygjpeg-7.dll /bin/cygstdc++-6.dll /bin/cygwin1.dll
endif

.PHONY: fcamDngUtil-package
fcamDngUtil-package: fcamDngUtil
	-rm -rf fcamDngUtil.$(PLATFORM).zip
	mkdir -p fcamDngUtil-$(FCAM_DNG_UTIL_VERSION)
	cp $(FCAM_DNG_UTIL_PKG_FILES) fcamDngUtil-$(FCAM_DNG_UTIL_VERSION)
	cp $(RELEASE_DIR).$(PLATFORM)/fcamDngUtil fcamDngUtil-$(FCAM_DNG_UTIL_VERSION)
	zip -r fcamDngUtil.$(PLATFORM).zip fcamDngUtil-$(FCAM_DNG_UTIL_VERSION)
	-rm -rf fcamDngUtil-$(FCAM_DNG_UTIL_VERSION)

fcamDngUtil: utils/fcamDngUtil.cpp release
	$(CXX) -O3 $(CXXFLAGS) -D FCAM_VERSION='"$(FCAM_VERSION)"' -o $(RELEASE_DIR).${PLATFORM}/$@ $<  $(TESTLIBS) 
	cp $(RELEASE_DIR).${PLATFORM}/$@ $(BINARY_DIR)

fcamVignette: utils/fcamVignette.cpp release
	$(CXX) -O3 $(CXXFLAGS) -D FCAM_VERSION='"$(FCAM_VERSION)"' -o $(RELEASE_DIR).${PLATFORM}/$@ $<  $(TESTLIBS) 
	cp $(RELEASE_DIR).${PLATFORM}/$@ $(BINARY_DIR)
