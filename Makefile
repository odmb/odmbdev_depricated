##
#
# This is the ME1/1 development Makefile
#
##

#BUILD_HOME:=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)
PACKAGE_VER_MAJOR=01
PACKAGE_VER_MINOR=00
PACKAGE_VER_PATCH=00

#
# Packages to be built
#
Project = emu
Package = odmbdev
PackageName= emuodmbdev
Description="Emu ME1/1 development tools"
Summary="Emu ME1/1 development tools"
Authors="Dan King, Brant Rumberger, Joe Haley"
Link=""

Sources = \
	utils.cc \
	Manager.cc \
	Action.cc \
	ButtonAction.cc \
	LongTextBoxAction.cc \
        OneTextBoxAction.cc \
	FourTextBoxAction.cc \
	LogAction.cc \
	Buttons.cc \
	SingleTextBoxAction.cc \
	version.cc
TestSources =
TestIncludeDirs =

IncludeDirs = \
	$(XDAQ_ROOT)/include \
	$(BUILD_HOME)/emu/base/include \
	$(BUILD_HOME)/emu/emuDCS/PeripheralCore/include \
	$(BUILD_HOME)/emu/emuDCS/PeripheralApps/include \
	$(BUILD_HOME)/emu/odmbdev/include
LibraryDirs =

UserSourcePath =
UserCFlags = $(ROOTCFLAGS)
#UserCCFlags = -g -Wall -pedantic-errors -Wno-long-long
UserCCFlags = -O0 -g -Wall -Wno-long-long $(ROOTCFLAGS)
UserDynamicLinkFlags = $(ROOTLIBS)
UserStaticLinkFlags =
UserExecutableLinkFlags =

# These libraries can be platform specific and
# potentially need conditional processing
#
Libraries =
ExternalObjects =

#
# Compile the source files and create a shared library
#
DynamicLibrary = emuodmbdev
StaticLibrary =

TestLibraryDirs=
TestLibraries=
TestExecutables=

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
