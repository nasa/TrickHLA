# @copyright Copyright 2024 United States Government as represented by the Administrator of the
#            National Aeronautics and Space Administration.  All Rights Reserved. */

MODEL_PACKAGE_HOME  = ${PWD}/../../../
TRICKHLA_HOME      ?= ${PWD}/../../../../TrickHLA

# Info and error message text colors.
RED_TXT   =[31m
GREEN_TXT =[32m
RESET_TXT =[00m

# Check for required packages.
ifeq ("$(wildcard ${TRICKHLA_HOME})","")
   $(error ${RED_TXT}This sim requires you to define TRICKHLA_HOME or have it installed at the default location!${RESET_TXT})
endif

# Compile with optimization and debug hooks, point Trick to model code.
TRICK_CFLAGS   += -g -O2 -I${MODEL_PACKAGE_HOME}
TRICK_CXXFLAGS += -g -O2 -I${MODEL_PACKAGE_HOME}
TRICK_SFLAGS   += -I${MODEL_PACKAGE_HOME}/S_modules/DistIf

#============================================================================
# HLA Build Environment
#============================================================================
# Define the HLA RTI installation folder.
# Default environment:
export RTI_HOME ?= /users/hla/prti1516e_5.5.11.0

# Include the HLA RTI.
ifneq ("$(wildcard $(RTI_HOME))","")
   $(info ${GREEN_TXT}Building the sim with HLA.${RESET_TXT})
   include ${TRICKHLA_HOME}/makefiles/S_hla.mk
   TRICK_GTE_EXT += RTI_HOME
   TRICK_SFLAGS  += -DHLA
else
   $(info ${GREEN_TXT}Building the sim without HLA.${RESET_TXT})
endif

# Links to the FOM and Modified_data folders in the sim folder.
all: build_links
clean: clean_links
spotless: clean_links

build_links:
	@ echo "${GREEN_TXT}Creating links to FOM and Modified_data folders.${RESET_TXT}"
	ln -s -fn ${MODEL_PACKAGE_HOME}/FOMs/DistIf FOMs/DistIf
	ln -s -fn ${MODEL_PACKAGE_HOME}/Modified_data/DistIf Modified_data/DistIf
	ln -s -fn ${TRICKHLA_HOME}/Modified_data/TrickHLA Modified_data/TrickHLA

clean_links:
	@if [ -e FOMs/DistIf ] ; then \
		echo "${GREEN_TXT}Cleaning link to FOMs/DistIf folder.${RESET_TXT}" ;\
		rm -r -f FOMs/DistIf ;\
	fi
	@if [ -e Modified_data/DistIf ] ; then \
		echo "${GREEN_TXT}Cleaning link to Modified_data/DistIf folder.${RESET_TXT}" ;\
		rm -r -f Modified_data/DistIf ;\
	fi
	@if [ -e Modified_data/TrickHLA ] ; then \
		echo "${GREEN_TXT}Cleaning link to Modified_data/TrickHLA folder.${RESET_TXT}" ;\
		rm -r -f Modified_data/TrickHLA ;\
	fi
