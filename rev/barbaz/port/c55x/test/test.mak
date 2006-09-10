# Generated by Code Composer Studio

# Makefile for E:\work\porus\port\c55x\test\test.pjt
# This makefile is intended to be compatible with any version of make.
#
# USAGE
# Step 1: Set up proper environment variables, by running DosRun.bat
#         (Windows platform) or set them up by yourself.
# Step 2: Change directory to the directory of this makefile, which is stored
#         in the macro MAKEFILE_DIR
# Step 3: In the makefile directory, you can perform one of these operations:
#         To build the configuration <config>, type make <config>
#         To clean the configuration <config>, type make <config>clean
#         To rebuild the configuration <config>, type make <config>rebuild
#         If <config> is not specified, the operations apply to the active
#         configuration, which is stored in the macro ACTIVE_CONFIG.
# For CCS gmake users: In the makefile directory, you can perform one of these operations:
#         To build the configuration <config>, type gmake projectName.mak <config>
#         To clean the configuration <config>, type gmake <config>clean
#         To rebuild the configuration <config>, type gmake <config>rebuild
#         If <config> is not specified, the operations apply to the active
#         configuration, which is stored in the macro ACTIVE_CONFIG.
#
# DETAILS
# Step 1:
#   In order for this makefile to work, you must have the environment variables
# set properly.  Specifically, the paths to build programs and the system-wide
# header files and library files must be set.
#   In Windows, a batch file "DosRun.bat", located at the root directory of
# the Code Composer Studio installation, is provided to set up these variables.
#   In UNIX, you can set up these environment variables yourself.  The "PATH"
# variable must include the paths to the build programs, and the "C_DIR" and
# "A_DIR" variables must include the paths to the system-wide header files and
# library files.
#
# Step 2:
#   System files referenced by this project are specified in full path names,
# while other files in this project are specified in path names relative to the
# directory of this makefile.  These directory names are stored in DIR_x macros.
# You can modify them to reflect the locations of the files on this system.
#   It is important that the command to make this makefile is issued from the
# directory of this makefile, which is stored in the macro MAKEFILE_DIR.
#
# Step 3:
#   There are three operations that can be performed on a build configuration:
# make, clean, and rebuild. A rebuild forces all files in the configuration to
# be rebuilt.
# To build the configuration <config>, type make <config>
# To clean the configuration <config>, type make <config>clean
# To rebuild the configuration <config>, type make <config>rebuild
#   If <config> is not specified, the operations apply to the active
# configuration. The active configuration can be any valid build configuration
# (including "all") and is specified in the macro ACTIVE_CONFIG.
# To build the active configuration, type make
# To clean the active configuration, type make clean
# To rebuild the active configuration, type make rebuild
#
# For CCS gmake users:
#   There are three operations that can be performed on a build configuration:
# make, clean, and rebuild. A rebuild forces all files in the configuration to
# be rebuilt.
# To build the configuration <config>, type gmake projectName.mak <config>
# To clean the configuration <config>, type gmake <config>clean
# To rebuild the configuration <config>, type gmake <config>rebuild
#   If <config> is not specified, the operations apply to the active
# configuration. The active configuration can be any valid build configuration
# (including "all") and is specified in the macro ACTIVE_CONFIG.
# To build the active configuration, type gmake
# To clean the active configuration, type gmake clean
# To rebuild the active configuration, type gmake rebuild

# The MKFLAGS macro takes the place of the MFLAGS or MAKEFLAGS macro. If it
# gives you any trouble, just delete the macro. It is used when recursively
# calling make (i.e. when rebuilding)
MKFLAGS= $(MFLAGS)$(MAKEFLAGS)

MAKEFILE_DIR= E:/work/porus/port/c55x/test

# The active configuration can be one of these valid build configurations:
# all, Debug
ACTIVE_CONFIG= Debug

# These DIR_x macros store the directories of the files used in this project.
# There must be no trailing spaces after these macros.

DIR_1= C:/CCStudio_v3.1/plugins/bios

DIR_2= Debug

DIR_3= C:/CCStudio_v3.1/c5500/bios/include

DIR_4= C:/CCStudio_v3.1/C5500/cgtools/include

DIR_5= C:/CCStudio_v3.1/C5500/cgtools/bin

SOURCE=main.c
SOURCE=test.cdb

active_config: $(ACTIVE_CONFIG)

norecurse: $(ACTIVE_CONFIG)_norecurse

clean: $(ACTIVE_CONFIG)clean

clean_norecurse: $(ACTIVE_CONFIG)clean_norecurse

rebuild: $(ACTIVE_CONFIG)rebuild

force_rebuild: 

all: Debug 

allclean: Debugclean 

allrebuild: Debugrebuild 


Debugclean: Debugclean_norecurse

Debugclean_norecurse: 
	-@rm -f testcfg.cmd
	-@rm -f testcfg.h
	-@rm -f testcfg.h55
	-@rm -f testcfg.s55
	-@rm -f testcfg_c.c
	-@rm -f $(DIR_2)/main.obj
	-@rm -f $(DIR_2)/testcfg.obj
	-@rm -f $(DIR_2)/testcfg_c.obj
	-@rm -f $(DIR_2)/test.out

Debugrebuild: 
	$(MAKE) $(MFLAGS) -f test.mak Debug FRC=force_rebuild

Debug: Debug_norecurse

Debug_norecurse: $(DIR_2)/test.out



testcfg.cmd \
testcfg.h \
testcfg.h55 \
testcfg.s55 \
testcfg_c.c: $(FRC) test.cdb 
	"$(DIR_1)/gconfgen" test.cdb

$(DIR_2)/main.obj: $(FRC) main.c mmb0.h usb.h ads1271.h bfifo.h $(DIR_3)/clk.h $(DIR_4)/stdlib.h $(DIR_3)/c55.h $(DIR_3)/mbx.h 
	"$(DIR_5)/cl55" -g -fr"E:/work/porus/port/c55x/test/Debug" -d"_DEBUG" "main.c" 

$(DIR_2)/testcfg.obj: $(FRC) testcfg.s55 testcfg.h55 $(DIR_3)/confbeg.s55 $(DIR_3)/gbl.h55 $(DIR_3)/mem.h55 $(DIR_3)/obj.h55 $(DIR_3)/buf.h55 $(DIR_3)/clk.h55 $(DIR_3)/prd.h55 $(DIR_3)/rtdx.h55 $(DIR_3)/hst.h55 $(DIR_3)/hwi.h55 $(DIR_3)/swi.h55 $(DIR_3)/tsk.h55 $(DIR_3)/idl.h55 $(DIR_3)/isrc.h55 $(DIR_3)/log.h55 $(DIR_3)/pip.h55 $(DIR_3)/sem.h55 $(DIR_3)/mbx.h55 $(DIR_3)/que.h55 $(DIR_3)/lck.h55 $(DIR_3)/sio.h55 $(DIR_3)/sts.h55 $(DIR_3)/sys.h55 $(DIR_3)/gio.h55 $(DIR_3)/dev.h55 $(DIR_3)/udev.h55 $(DIR_3)/dgn.h55 $(DIR_3)/dhl.h55 $(DIR_3)/dpi.h55 $(DIR_3)/hook.h55 $(DIR_3)/dio.h55 $(DIR_3)/pwrm.h55 $(DIR_3)/confend.s55 
	"$(DIR_5)/cl55" -g -fr"E:/work/porus/port/c55x/test/Debug" -d"_DEBUG" "testcfg.s55" 

$(DIR_2)/testcfg_c.obj: $(FRC) testcfg_c.c testcfg.h $(DIR_3)/std.h $(DIR_3)/swi.h $(DIR_3)/tsk.h $(DIR_3)/log.h 
	"$(DIR_5)/cl55" -g -fr"E:/work/porus/port/c55x/test/Debug" -d"_DEBUG" "testcfg_c.c" 

$(DIR_2)/test.out: $(FRC) testcfg.cmd $(DIR_2)/main.obj $(DIR_2)/testcfg.obj $(DIR_2)/testcfg_c.obj 
	-@echo -z -c -m"./Debug/test.map" -o"./Debug/test.out" -w -x> test.Debug.lkf
	-@echo "testcfg.cmd">> test.Debug.lkf
	-@echo "$(DIR_2)/main.obj">> test.Debug.lkf
	-@echo "$(DIR_2)/testcfg.obj">> test.Debug.lkf
	-@echo "$(DIR_2)/testcfg_c.obj">> test.Debug.lkf
	"$(DIR_5)/cl55" -@"test.Debug.lkf"
	-@rm -f test.Debug.lkf