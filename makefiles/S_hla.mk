#=============================================================================
# Set up HLA and TrickHLA environment.
# Note the developer should check out the latest TrickHLA tag to the location
# defined below in $TRICKHLA_HOME
#
# DEPENDENCIES:
# 1) You must set the TRICKHLA_HOME, RTI_HOME, and RTI_VENDOR environment
#    variables because this makefile depends on them.
# 2) The bc, cut, grep, and which system command packages must be installed.
#=============================================================================
# Info and error message text colors.
RED_TXT   =[31m
GREEN_TXT =[32m
RESET_TXT =[00m

# Make sure the bc, cut, grep, and which system commands can be found.
ifeq (,$(findstring which, $(shell which which)))
   $(error ${RED_TXT}S_hla.mk:ERROR: Could not find the which system command. Please ensure your PATH is correct or install the which command package.${RESET_TXT})
endif
ifeq (,$(findstring bc, $(shell which bc)))
   $(error ${RED_TXT}S_hla.mk:ERROR: Could not find the bc system command. Please ensure your PATH is correct or install the bc arithmetic language processor command package.${RESET_TXT})
endif
ifeq (,$(findstring cut, $(shell which cut)))
   $(error ${RED_TXT}S_hla.mk:ERROR: Could not find the cut system command. Please ensure your PATH is correct or install the cut command package.${RESET_TXT})
endif
ifeq (,$(findstring grep, $(shell which grep)))
   $(error ${RED_TXT}S_hla.mk:ERROR: Could not find the grep system command. Please ensure your PATH is correct or install the grep command package.${RESET_TXT})
endif
ifeq (,$(findstring grep, $(shell which readlink)))
   $(error ${RED_TXT}S_hla.mk:ERROR: Could not find the readlink system command. Please ensure your PATH is correct or install the readlink command package.${RESET_TXT})
endif

# Verify the TRICKHLA_HOME environment variables is set and the path is valid.
ifndef TRICKHLA_HOME
   ifdef MODEL_PACKAGE_HOME
      export TRICKHLA_HOME = ${MODEL_PACKAGE_HOME}/TrickHLA
      $(info ${GREEN_TXT}S_hla.mk:INFO: Overriding TRICKHLA_HOME = ${TRICKHLA_HOME}${RESET_TXT})
   else
      $(error ${RED_TXT}S_hla.mk:ERROR: The TRICKHLA_HOME environment variable is not set.${RESET_TXT})
   endif
endif
ifeq ("$(wildcard ${TRICKHLA_HOME})","")
   $(error ${RED_TXT}S_hla.mk:ERROR: Must specify a valid TRICKHLA_HOME environment variable, which is currently set to invalid path ${TRICKHLA_HOME}${RESET_TXT})
endif

# Verify the RTI_HOME environment variables is set and the path is valid.
ifndef RTI_HOME
   $(error ${RED_TXT}S_hla.mk:ERROR: The RTI_HOME environment variable is not set.${RESET_TXT})
endif
ifeq ("$(wildcard ${RTI_HOME})","")
   $(error ${RED_TXT}S_hla.mk:ERROR: Must specify a valid RTI_HOME environment variable, which is currently set to invalid path ${RTI_HOME}${RESET_TXT})
endif

# Verify the RTI_HOME environment variables is set and is valid.
# Also set the IEEE-1516 standard and RTI include paths based on the
# RTI vendor and version specified.
ifndef RTI_VENDOR
   $(error ${RED_TXT}S_hla.mk:ERROR: The RTI_VENDOR environment variable is not set.${RESET_TXT})
endif
IS_PITCH_RTI = 0
ifeq ($(RTI_VENDOR),Pitch_HLA_4)
   IS_PITCH_RTI    = 1
   HLA_STANDARD    = IEEE_1516_2025
   RTI_INCLUDE     = ${RTI_HOME}/api/cpp/HLA_1516-2025
   TRICK_CFLAGS   += -I${RTI_INCLUDE}
   TRICK_CXXFLAGS += -I${RTI_INCLUDE}
else ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
   IS_PITCH_RTI    = 1
   HLA_STANDARD    = IEEE_1516_2010
   RTI_INCLUDE     = ${RTI_HOME}/api/cpp/HLA_1516-2010
   ifeq ("$(wildcard ${RTI_INCLUDE})","")
      RTI_INCLUDE  = ${RTI_HOME}/include
   endif
   TRICK_CFLAGS   += -I${RTI_INCLUDE}
   TRICK_CXXFLAGS += -I${RTI_INCLUDE}
else ifeq ($(RTI_VENDOR),MAK_HLA_Evolved)
   HLA_STANDARD    = IEEE_1516_2010
   RTI_INCLUDE     = ${RTI_HOME}/include/HLA1516E
   TRICK_CFLAGS   += -DRTI_VENDOR=MAK_HLA_Evolved -I${RTI_INCLUDE}
   TRICK_CXXFLAGS += -DRTI_VENDOR=MAK_HLA_Evolved -I${RTI_INCLUDE}
else
   $(error ${RED_TXT}S_hla.mk:ERROR: Unsupported RTI_VENDOR '${RTI_VENDOR}', must specify one of Pitch_HLA_4, Pitch_HLA_Evolved, or MAK_HLA_Evolved.${RESET_TXT})
endif

# Ensure the environment variables set by the Pitch RTI are consistent with
# the RTI home directory specified.
ifeq ($(IS_PITCH_RTI),1)
   ifdef PRTI6_ROOT
      ifneq ($(PRTI6_ROOT),$(RTI_HOME))
         export PRTI6_ROOT = ${RTI_HOME}
         $(info ${GREEN_TXT}S_hla.mk:INFO: Overriding PRTI6_ROOT = ${PRTI6_ROOT}${RESET_TXT})
      endif
   endif
   ifdef PRTI1516E_HOME
      ifneq ($(PRTI1516E_HOME),$(RTI_HOME))
         export PRTI1516E_HOME = ${RTI_HOME}
         $(info ${GREEN_TXT}S_hla.mk:INFO: Overriding PRTI1516E_HOME = ${PRTI1516E_HOME}${RESET_TXT})
      endif
   endif
   ifdef PitchRTI_ROOT
      ifneq ($(PitchRTI_ROOT),$(RTI_HOME))
         export PitchRTI_ROOT = ${RTI_HOME}
         $(info ${GREEN_TXT}S_hla.mk:INFO: Overriding PitchRTI_ROOT = ${PitchRTI_ROOT}${RESET_TXT})
      endif
   endif
endif

# Needed for TrickHLA.
TRICK_SFLAGS   += -I${TRICKHLA_HOME}/S_modules
TRICK_CFLAGS   += -I${TRICKHLA_HOME}/include -I${TRICKHLA_HOME}/models -D${HLA_STANDARD}
TRICK_CXXFLAGS += -I${TRICKHLA_HOME}/include -I${TRICKHLA_HOME}/models -D${HLA_STANDARD}
ifeq ($(HLA_STANDARD),IEEE_1516_2010)
	TRICK_CFLAGS   += -Wno-deprecated-declarations
	TRICK_CXXFLAGS += -Wno-deprecated-declarations
endif

# Configure the ICG and swig excludes.
ifdef TRICK_ICG_EXCLUDE
   TRICK_ICG_EXCLUDE += :${RTI_INCLUDE}
else
   TRICK_ICG_EXCLUDE = ${RTI_INCLUDE}
endif
ifdef TRICK_SWIG_EXCLUDE
   TRICK_SWIG_EXCLUDE += :${RTI_INCLUDE}
else
   TRICK_SWIG_EXCLUDE = ${RTI_INCLUDE}
endif

# Determine what compiler Trick is using.
ifneq (,$(findstring trick-gte, $(shell which trick-gte)))
   CPPC_CMD = $(shell trick-gte TRICK_CXX)
   ifeq (,$(CPPC_CMD))
      CPPC_CMD = $(shell trick-gte TRICK_CPPC)
      ifeq (,$(CPPC_CMD))
         $(error ${RED_TXT}S_hla.mk:ERROR: Could not determine compiler from TRICK_CXX or TRICK_CPPC using trick-gte command!${RESET_TXT})
      endif
   endif
else
   CPPC_CMD = $(shell gte TRICK_CXX)
   ifeq (,$(CPPC_CMD))
      CPPC_CMD = $(shell gte TRICK_CPPC)
      ifeq (,$(CPPC_CMD))
         $(error ${RED_TXT}S_hla.mk:ERROR: Could not determine compiler from TRICK_CXX or TRICK_CPPC using Trick gte command!${RESET_TXT})
      endif
   endif
endif

# Determine the Trick patch version number.
ifneq (,$(findstring trick-version, $(shell which trick-version)))
   TRICK_PATCH_VER = $(shell trick-version -v | cut -d . -f 3 | cut -d - -f 1)
   ifeq (,$(TRICK_PATCH_VER))
      $(error ${RED_TXT}S_hla.mk:ERROR: Could not determine Trick patch version using trick-version command!${RESET_TXT})
   endif
else
   $(error ${RED_TXT}S_hla.mk:ERROR: Could not find the trick-version command!${RESET_TXT})
endif
TRICK_CFLAGS   += -DTRICK_PATCH=${TRICK_PATCH_VER}
TRICK_CXXFLAGS += -DTRICK_PATCH=${TRICK_PATCH_VER}

ifeq ($(TRICK_HOST_TYPE),Darwin)
   # macOS

   ifeq ($(HLA_STANDARD),IEEE_1516_2010)
      # C++17 removed the dynamic exception specification so fallback to
      # C++14 for ICG because the IEEE 1516-2010 APIs use dynamic exception
      # specifications. Otherwise this will result in compile time errors.
      TRICK_ICGFLAGS += --icg-std=c++14

      # Trick now requires at least c++14, and given HLA Evolved is using
      # deprecated c++ APIs, we can only use c++14.
      TRICK_CXXFLAGS += -std=c++14

      $(info ${GREEN_TXT}S_hla.mk:INFO: Using the c++14 standard.${RESET_TXT})
   endif

   ifeq ($(RTI_VENDOR),Pitch_HLA_4)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info ${GREEN_TXT}S_hla.mk:INFO: User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      RTI_JAVA_HOME ?= ${RTI_HOME}/jre
      ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/server
      else
         # Default to JRE that came with the Pitch RTI if needed.
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      endif

      # Verify the RTI Java Home and Lib paths.
      ifeq ("$(wildcard ${RTI_JAVA_HOME})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}${RESET_TXT})
      endif

      # Determine if the compiler is clang or gcc.
      ifneq (,$(findstring clang, $(shell $(CPPC_CMD) --version | grep clang)))

         # Determine the clang version.
         COMPILER_VERSION = $(shell $(CPPC_CMD) --version | grep clang | cut -d' ' -f 4 | cut -d . -f 1)
         ifneq ("$(wildcard ${RTI_HOME}/lib/clang12)","")
            # clang12 library exists and is the clang compiler at least version 12.
            COMPILER_GTE_12 = $(shell echo $(COMPILER_VERSION)\>=12 | bc )
         else
            COMPILER_GTE_12 = 0
         endif
         ifeq ($(COMPILER_GTE_12),1)
            ifdef DYLD_LIBRARY_PATH
               export DYLD_LIBRARY_PATH += :${RTI_HOME}/lib
            else
               export DYLD_LIBRARY_PATH = ${RTI_HOME}/lib
            endif
            TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -v -Wl,-rpath,${RTI_HOME}/lib -lrti1516_2025clang12 -lfedtime1516_2025clang12 -L${RTI_JAVA_LIB_PATH} -v -Wl,-rpath,${RTI_JAVA_LIB_PATH} -ljvm
         else
            $(error ${RED_TXT}S_hla.mk:ERROR: Pitch RTI libraries require at least clang 12 on the Mac.${RESET_TXT})
         endif
      else
         # Using gcc compiler instead of clang.
         $(error ${RED_TXT}S_hla.mk:ERROR: Pitch RTI only supports clang on the Mac.${RESET_TXT})
      endif
      # Add the CLASSPATH and DYLD_LIBRARY_PATH environment variables to the 
      # simulation executable.
      export CLASSPATH     += ${RTI_HOME}/lib/prti1516_hla4.jar
      export TRICK_GTE_EXT += CLASSPATH DYLD_LIBRARY_PATH

   else ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info ${GREEN_TXT}S_hla.mk:INFO: User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      RTI_JAVA_HOME ?= ${RTI_HOME}/jre
      ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/server
      else
         # Default to JRE that came with the Pitch RTI if needed.
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      endif

      # Verify the RTI Java Home and Lib paths.
      ifeq ("$(wildcard ${RTI_JAVA_HOME})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}${RESET_TXT})
      endif

      # Determine if the compiler is clang or gcc.
      ifneq (,$(findstring clang, $(shell $(CPPC_CMD) --version | grep clang)))

         # Determine the clang version.
         COMPILER_VERSION = $(shell $(CPPC_CMD) --version | grep clang | cut -d' ' -f 4 | cut -d . -f 1)
         ifneq ("$(wildcard ${RTI_HOME}/lib/clang12)","")
            # clang12 library exists and is the clang compiler at least version 12.
            COMPILER_GTE_12 = $(shell echo $(COMPILER_VERSION)\>=12 | bc )
         else
            COMPILER_GTE_12 = 0
         endif
         ifeq ($(COMPILER_GTE_12),1)
            ifdef DYLD_LIBRARY_PATH
               export DYLD_LIBRARY_PATH += :${RTI_HOME}/lib/clang12
            else
               export DYLD_LIBRARY_PATH = ${RTI_HOME}/lib/clang12
            endif
            TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -L${RTI_HOME}/lib/clang12 -v -Wl,-rpath,${RTI_HOME}/lib/clang12 -lrti1516e -lfedtime1516e -L${RTI_JAVA_LIB_PATH} -v -Wl,-rpath,${RTI_JAVA_LIB_PATH} -ljvm
         else
            ifdef DYLD_LIBRARY_PATH
               export DYLD_LIBRARY_PATH += :${RTI_HOME}/lib/clang5
            else
               export DYLD_LIBRARY_PATH = ${RTI_HOME}/lib/clang5
            endif
            TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -L${RTI_HOME}/lib/clang5 -v -Wl,-rpath,${RTI_HOME}/lib/clang5 -lrti1516e -lfedtime1516e -L${RTI_JAVA_LIB_PATH} -v -Wl,-rpath,${RTI_JAVA_LIB_PATH} -ljvm
         endif
      else
         # Using gcc compiler instead of clang.
         ifdef DYLD_LIBRARY_PATH
            export DYLD_LIBRARY_PATH += :${RTI_HOME}/lib/gcc42
         else
            export DYLD_LIBRARY_PATH = ${RTI_HOME}/lib/gcc42
         endif
         TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib/gcc42 -lrti1516e -lfedtime1516e
      endif
      # Add the CLASSPATH and DYLD_LIBRARY_PATH environment variables to the 
      # simulation executable.
      export CLASSPATH     += ${RTI_HOME}/lib/prti1516e.jar
      export TRICK_GTE_EXT += CLASSPATH DYLD_LIBRARY_PATH
   else
      $(error ${RED_TXT}S_hla.mk:ERROR: Unsupported RTI_VENDOR '${RTI_VENDOR}', must specify Pitch_HLA_4 or Pitch_HLA_Evolved.${RESET_TXT})
   endif

else
   # Linux

   # Determine the gcc compiler version.
   COMPILER_VERSION = $(shell $(CPPC_CMD) -dumpversion | cut -d . -f 1)

   ifeq ($(HLA_STANDARD),IEEE_1516_2010)
      # ICG code needs to be targeted to either C++14 (gcc versions 6.1 to 10)
      # or C++11 (gcc 4.8.1+) because C++17 (gcc version 11+) removed the dynamic
      # exception specification and the IEEE 1516-2010 APIs use it. Otherwise
      # this will result in compile time errors.
      TRICK_ICGFLAGS += --icg-std=c++14

      # Trick now requires at least c++14, and given HLA Evolved is using
      # deprecated c++ APIs, we can only use c++14.
      TRICK_CXXFLAGS += -std=c++14

      $(info ${GREEN_TXT}S_hla.mk:INFO: Using the c++14 standard.${RESET_TXT})
   endif

   ifeq ($(RTI_VENDOR),Pitch_HLA_4)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info ${GREEN_TXT}S_hla.mk:INFO: User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      RTI_JAVA_HOME ?= ${RTI_HOME}/jre
      ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/amd64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/amd64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/aarch64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/aarch64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/amd64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/amd64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/aarch64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/aarch64/server
      else
         # Default to JRE that came with the Pitch RTI if needed.
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      endif
      # Verify the RTI Java Home and Lib paths.
      ifeq ("$(wildcard ${RTI_JAVA_HOME})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}${RESET_TXT})
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_JAVA_LIB_PATH}/.. -L${RTI_JAVA_LIB_PATH} -Wl,-rpath,${RTI_JAVA_LIB_PATH}/.. -Wl,-rpath,${RTI_JAVA_LIB_PATH} -ljava -ljvm -lverify

      # Add the CLASSPATH environment variable to the simulation executable.
      export CLASSPATH     += ${RTI_HOME}/lib/prti1516_hla4.jar
      export TRICK_GTE_EXT += CLASSPATH

      # Determine which gcc library version to use.
      ifeq ($(shell echo $(COMPILER_VERSION)\>=7 | bc),1)
         RTI_LIB_PATH = $(shell readlink -f ${RTI_HOME}/lib)
      else
         $(error ${RED_TXT}S_hla.mk:ERROR: Pitch RTI libraries require at least gcc 7 for Linux.${RESET_TXT})
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_LIB_PATH} -Wl,-rpath,${RTI_LIB_PATH} -lrti1516_2025gcc7 -lfedtime1516_2025gcc7

      ifdef LD_LIBRARY_PATH
         ifneq (,$(findstring ${RTI_LIB_PATH}, $(LD_LIBRARY_PATH)))
            RTI_LIB_ALREADY_SET_IN_LD_LIB_PATH = 1
         else
            export LD_LIBRARY_PATH += :${RTI_JAVA_LIB_PATH}/..:${RTI_JAVA_LIB_PATH}:${RTI_LIB_PATH}
         endif
      else
         export LD_LIBRARY_PATH = ${RTI_JAVA_LIB_PATH}/..:${RTI_JAVA_LIB_PATH}:${RTI_LIB_PATH}
      endif

      # On Ubuntu, the user needs to add the LD_LIBRARY_PATH shown below to
      # their environment so the HLA simulation can run, and only show the
      # message if LD_LIBRARY_PATH is not already set correctly.
      ifndef RTI_LIB_ALREADY_SET_IN_LD_LIB_PATH
         ifdef LD_LIBRARY_PATH
            ifneq ("$(wildcard /etc/lsb-release)","")
              $(info ${GREEN_TXT}S_hla.mk:INFO: Add this to your .bashrc file: export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)${RESET_TXT})
            endif
         endif
      endif

   else ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info ${GREEN_TXT}S_hla.mk:INFO: User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      RTI_JAVA_HOME ?= ${RTI_HOME}/jre
      ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/amd64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/amd64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/aarch64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/aarch64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/amd64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/amd64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/aarch64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/aarch64/server
      else
         # Default to JRE that came with the Pitch RTI if needed.
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      endif
      # Verify the RTI Java Home and Lib paths.
      ifeq ("$(wildcard ${RTI_JAVA_HOME})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}${RESET_TXT})
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_JAVA_LIB_PATH}/.. -L${RTI_JAVA_LIB_PATH} -Wl,-rpath,${RTI_JAVA_LIB_PATH}/.. -Wl,-rpath,${RTI_JAVA_LIB_PATH} -ljava -ljvm -lverify

      # Add the CLASSPATH environment variable to the simulation executable.
      export CLASSPATH     += ${RTI_HOME}/lib/prti1516e.jar
      export TRICK_GTE_EXT += CLASSPATH

      # Determine which gcc library version to use.
      ifeq ($(shell echo $(COMPILER_VERSION)\>=7 | bc),1)
         RTI_LIB_PATH = $(shell readlink -f ${RTI_HOME}/lib/gcc73_64)
      else ifeq ($(shell echo $(COMPILER_VERSION)\>=5 | bc),1)
         RTI_LIB_PATH = $(shell readlink -f ${RTI_HOME}/lib/gcc52_64)
      else ifeq ($(shell echo $(COMPILER_VERSION)\>=4 | bc),1)
         RTI_LIB_PATH = $(shell readlink -f ${RTI_HOME}/lib/gcc41_64)
      else
         RTI_LIB_PATH = $(shell readlink -f ${RTI_HOME}/lib/gcc34_64)
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_LIB_PATH} -Wl,-rpath,${RTI_LIB_PATH} -lrti1516e64 -lfedtime1516e64

      ifdef LD_LIBRARY_PATH
         ifneq (,$(findstring ${RTI_LIB_PATH}, $(LD_LIBRARY_PATH)))
            RTI_LIB_ALREADY_SET_IN_LD_LIB_PATH = 1
         else
            export LD_LIBRARY_PATH += :${RTI_JAVA_LIB_PATH}/..:${RTI_JAVA_LIB_PATH}:${RTI_LIB_PATH}
         endif
      else
         export LD_LIBRARY_PATH = ${RTI_JAVA_LIB_PATH}/..:${RTI_JAVA_LIB_PATH}:${RTI_LIB_PATH}
      endif

      # On Ubuntu, the user needs to add the LD_LIBRARY_PATH shown below to
      # their environment so the HLA simulation can run, and only show the
      # message if LD_LIBRARY_PATH is not already set correctly.
      ifndef RTI_LIB_ALREADY_SET_IN_LD_LIB_PATH
         ifdef LD_LIBRARY_PATH
            ifneq ("$(wildcard /etc/lsb-release)","")
              $(info ${GREEN_TXT}S_hla.mk:INFO: Add this to your .bashrc file: export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)${RESET_TXT})
            endif
         endif
      endif

   else ifeq ($(RTI_VENDOR),MAK_HLA_Evolved)
      TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -lrti1516e64 -lfedtime1516e64
   else
      $(error ${RED_TXT}S_hla.mk:ERROR: Unsupported RTI_VENDOR '${RTI_VENDOR}', must specify one of Pitch_HLA_4, Pitch_HLA_Evolved, or MAK_HLA_Evolved.${RESET_TXT})
   endif

endif
