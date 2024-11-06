# @copyright Copyright 2024 United States Government as represented by the Administrator of the
#            National Aeronautics and Space Administration.  All Rights Reserved. */

MODEL_PACKAGE_HOME  = ${PWD}/../../../
#TODO update on move to trickhla repo:
TRICKHLA_HOME      ?= ${PWD}/../../../../trickhla

# Check for required packages
ifeq ($(wildcard $(TRICKHLA_HOME)),)
    $(error This sim requires you to define TRICKHLA_HOME or have it at the default location!)
endif

# Compile with optimization and debug hooks, point Trick to model code
TRICK_CFLAGS   += -g -O2 -I${MODEL_PACKAGE_HOME}
TRICK_CXXFLAGS += -g -O2 -I${MODEL_PACKAGE_HOME}
TRICK_SFLAGS   += -I${MODEL_PACKAGE_HOME}/S_modules/FDI

#============================================================================
# HLA Build Environment
#============================================================================
# Define the HLA RTI installation folder.
# Default Tricklab environment:
export RTI_HOME ?= /users/hla/prti1516e_5.5.1.0

# Include the HLA RTI.
ifneq ("$(wildcard $(RTI_HOME))","")
    $(info Building the sim with HLA.)
	include ${TRICKHLA_HOME}/makefiles/S_hla.mk
	TRICK_GTE_EXT +=  RTI_HOME
	TRICK_SFLAGS  += -DHLA
else
    $(info Building the sim without HLA.)
endif

# Links to the FOM and Modified_data folders in the sim folder.
all: build_links
clean: clean_links
spotless: clean_links

build_links:
	@ echo "[32mCreating links to FOM and Modified_data folders[00m"
	ln -s -fn ${MODEL_PACKAGE_HOME}/FOMs/FDI FOMs/FDI
	ln -s -fn ${MODEL_PACKAGE_HOME}/Modified_data/FDI Modified_data/FDI
	ln -s -fn ${TRICKHLA_HOME}/Modified_data/TrickHLA Modified_data/TrickHLA

clean_links:
	@if [ -e FOMs/FDI ] ; then \
		echo "[32mCleaning link to FOMs/FDI folder[00m" ;\
		rm -r -f FOMs/FDI ;\
	fi
	@if [ -e Modified_data/FDI ] ; then \
		echo "[32mCleaning link to Modified_data/FDI folder[00m" ;\
		rm -r -f Modified_data/FDI ;\
	fi
	@if [ -e Modified_data/TrickHLA ] ; then \
		echo "[32mCleaning link to Modified_data/TrickHLA folder[00m" ;\
		rm -r -f Modified_data/TrickHLA ;\
	fi
