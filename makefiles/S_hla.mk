#=============================================================================
# Set up HLA and TrickHLA environment.
# Note the developer should check out the latest TrickHLA tag to the location
# defined below in $TRICKHLA_HOME
#=============================================================================
TRICKHLA_HOME ?= ${MODEL_PACKAGE_HOME}/TrickHLA
RTI_VENDOR    ?= Pitch_HLA_Evolved
RTI_HOME      ?= ${HOME}/rti/pRTI1516e

# Info and error message text colors.
RED_TXT   =[31m
GREEN_TXT =[32m
RESET_TXT =[00m

# Make sure the critical environment variable paths we depend on are valid.
ifeq ("$(wildcard ${TRICKHLA_HOME})","")
   $(error ${RED_TXT}S_hla.mk:ERROR: Must specify a valid TRICKHLA_HOME environment variable, which is currently set to invalid path ${TRICKHLA_HOME}${RESET_TXT})
endif
ifeq ("$(wildcard ${RTI_HOME})","")
   $(error ${RED_TXT}S_hla.mk:ERROR: Must specify a valid RTI_HOME environment variable, which is currently set to invalid path ${RTI_HOME}${RESET_TXT})
endif

# Either IEEE_1516_2010 for HLA Evolved or IEEE_1516_202X for HLA 4.
ifeq ($(RTI_VENDOR),Pitch_HLA_4)
   HLA_STANDARD = IEEE_1516_202X
else ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
   HLA_STANDARD = IEEE_1516_2010
else ifeq ($(RTI_VENDOR),MAK_HLA_Evolved)
   HLA_STANDARD = IEEE_1516_2010
# ERAU: Adding support for OpenRTI
else ifeq ($(RTI_VENDOR),OpenRTI_HLA)
   HLA_STANDARD = IEEE_1516_2010
else
   $(error ${RED_TXT}S_hla.mk:ERROR: Unsupported RTI_VENDOR '${RTI_VENDOR}', must specify one of Pitch_HLA_4, Pitch_HLA_Evolved, or MAK_HLA_Evolved.${RESET_TXT})
endif

# Needed for TrickHLA.
TRICK_SFLAGS   += -I${TRICKHLA_HOME}/S_modules
TRICK_CFLAGS   += -I${TRICKHLA_HOME}/include -I${TRICKHLA_HOME}/models -D${HLA_STANDARD} -Wno-deprecated-declarations
TRICK_CXXFLAGS += -I${TRICKHLA_HOME}/include -I${TRICKHLA_HOME}/models -D${HLA_STANDARD} -Wno-deprecated-declarations

# Needed for the HLA IEEE 1516 header files.
ifeq ($(RTI_VENDOR),Pitch_HLA_4)
   # Determine the Pitch RTI include path based on the HLA Standard specififed.
   RTI_INCLUDE    =  ${RTI_HOME}/api/cpp/HLA_1516-202X
   TRICK_CFLAGS   += -I${RTI_INCLUDE}
   TRICK_CXXFLAGS += -I${RTI_INCLUDE}
else ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
   # Determine the Pitch RTI include path based on the HLA Standard specififed.
   RTI_INCLUDE = ${RTI_HOME}/api/cpp/HLA_1516-2010
   ifeq ("$(wildcard ${RTI_INCLUDE})","")
      RTI_INCLUDE = ${RTI_HOME}/include
   endif
   TRICK_CFLAGS   += -I${RTI_INCLUDE}
   TRICK_CXXFLAGS += -I${RTI_INCLUDE}
else ifeq ($(RTI_VENDOR),MAK_HLA_Evolved)
   RTI_INCLUDE    =  ${RTI_HOME}/include/HLA1516E
   TRICK_CFLAGS   += -DRTI_VENDOR=MAK_HLA_Evolved -I${RTI_INCLUDE}
   TRICK_CXXFLAGS += -DRTI_VENDOR=MAK_HLA_Evolved -I${RTI_INCLUDE}
# ERAU: Adding support for OpenRTI
else ifeq ($(RTI_VENDOR),OpenRTI_HLA)
   RTI_INCLUDE    =  ${RTI_HOME}/include/rti1516e
   TRICK_CFLAGS   += -DRTI_VENDOR=OpenRTI_HLA -I${RTI_INCLUDE}
   TRICK_CXXFLAGS += -DRTI_VENDOR=OpenRTI_HLA -I${RTI_INCLUDE}
else
   $(error ${RED_TXT}S_hla.mk:ERROR: Unsupported RTI_VENDOR '${RTI_VENDOR}', must specify one of Pitch_HLA_4, Pitch_HLA_Evolved, or MAK_HLA_Evolved.${RESET_TXT})
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

ifeq ($(TRICK_HOST_TYPE),Darwin)
   # macOS

   # C++17 removed the dynamic exception specification so fallback to C++14 for
   # ICG because the IEEE 1516-2010 APIs use dynamic exception specifications.
   # Otherwise this will result in compile time errors.
   TRICK_ICGFLAGS += --icg-std=c++14
   $(info ${GREEN_TXT}S_hla.mk:INFO: Using C++14 for Trick ICG code.${RESET_TXT})

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
            TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -v -Wl,-rpath,${RTI_HOME}/lib -lrti1516_202Xclang12 -lfedtime1516_202Xclang12 -L${RTI_JAVA_LIB_PATH} -v -Wl,-rpath,${RTI_JAVA_LIB_PATH} -ljvm
         else
            $(error ${RED_TXT}S_hla.mk:ERROR: Pitch RTI libraries require at least clang 12 on the Mac.${RESET_TXT})
         endif
      else
         # Using gcc compiler instead of clang.
         $(error ${RED_TXT}S_hla.mk:ERROR: Pitch RTI only supports clang on the Mac.${RESET_TXT})
      endif
      # Add the CLASSPATH and DYLD_LIBRARY_PATH environment variables to the 
      # simulation executable.
      export CLASSPATH     += ${RTI_HOME}/lib/prti1516_202X.jar
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
   # ERAU: Adding support for OpenRTI
   else ifeq ($(RTI_VENDOR),OpenRTI_HLA)
      $(info ${GREEN_TXT}S_hla.mk:INFO: Using OpenRTI)
      TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -lOpenRTI -lrti1516e -lfedtime1516e -Wl,-rpath,${RTI_HOME}/lib

   else
      $(error ${RED_TXT}S_hla.mk:ERROR: Unsupported RTI_VENDOR '${RTI_VENDOR}', must specify Pitch_HLA_4 or Pitch_HLA_Evolved.${RESET_TXT})
   endif

else
   # Linux

   # Determine the gcc compiler version.
   COMPILER_VERSION = $(shell $(CPPC_CMD) -dumpversion | cut -d . -f 1)

   # The gcc version 11 compiler defaults to C++17 which removed the
   # dynamic exception specification. Instead fallback to C++14 because
   # the IEEE 1516-2010 APIs use dynamic exception specifications.
   # Otherwise this will result in compile time errors for C++17.
   ifeq ($(shell echo $(COMPILER_VERSION)\>=11 | bc),1)
      TRICK_CFLAGS   += -std=c++14
      TRICK_CXXFLAGS += -std=c++14
      $(info ${GREEN_TXT}S_hla.mk:INFO: Falling back to C++14 to compile Trick simulation.${RESET_TXT})
   endif

   # ICG code needs to be targeted to either C++14 (gcc versions 6.1 to 10)
   # or C++11 (gcc 4.8.1+) because C++17 (gcc version 11+) removed the dynamic
   # exception specification and the IEEE 1516-2010 APIs use it. Otherwise
   # this will result in compile time errors.
   ifeq ($(shell echo $(COMPILER_VERSION)\>=6 | bc),1)
      TRICK_ICGFLAGS += --icg-std=c++14
      $(info ${GREEN_TXT}S_hla.mk:INFO: Using C++14 for Trick ICG code.${RESET_TXT})
   endif

   ifeq ($(RTI_VENDOR),Pitch_HLA_4)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info ${GREEN_TXT}S_hla.mk:INFO: User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      RTI_JAVA_HOME ?= ${RTI_HOME}/jre
      ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/amd64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/amd64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/aarch64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/aarch64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      else
         # Default to JRE that came with the Pitch RTI if needed.
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/amd64/server
      endif
      # Verify the RTI Java Home and Lib paths.
      ifeq ("$(wildcard ${RTI_JAVA_HOME})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}${RESET_TXT})
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_JAVA_LIB_PATH}/.. -L${RTI_JAVA_LIB_PATH} -ljava -ljvm -lverify -Wl,-rpath,${RTI_JAVA_LIB_PATH}/.. -Wl,-rpath,${RTI_JAVA_LIB_PATH}

      # Add the CLASSPATH environment variable to the simulation executable.
      export CLASSPATH     += ${RTI_HOME}/lib/prti1516_202X.jar
      export TRICK_GTE_EXT += CLASSPATH

      # Determine which gcc library version to use.
      ifeq ($(shell echo $(COMPILER_VERSION)\>=7 | bc),1)
         RTI_LIB_PATH = ${RTI_HOME}/lib
      else
         $(error ${RED_TXT}S_hla.mk:ERROR: Pitch RTI libraries require at least gcc 7 for Linux.${RESET_TXT})
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_LIB_PATH} -lrti1516_202Xgcc7 -lfedtime1516_202Xgcc7 -Wl,-rpath,${RTI_LIB_PATH}

      # On Ubuntu, the user needs to add the LD_LIBRARY_PATH shown below to
      # their environment.
      ifneq ("$(wildcard /etc/lsb-release)","")
        $(info ${GREEN_TXT}S_hla.mk:INFO: Add this to your .bashrc file: export LD_LIBRARY_PATH=${RTI_JAVA_LIB_PATH}/..:${RTI_JAVA_LIB_PATH}:${RTI_LIB_PATH}${RESET_TXT})
      endif

   else ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info ${GREEN_TXT}S_hla.mk:INFO: User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      RTI_JAVA_HOME ?= ${RTI_HOME}/jre
      ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/amd64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/amd64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/aarch64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/aarch64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      else
         # Default to JRE that came with the Pitch RTI if needed.
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/amd64/server
      endif
      # Verify the RTI Java Home and Lib paths.
      ifeq ("$(wildcard ${RTI_JAVA_HOME})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}${RESET_TXT})
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error ${RED_TXT}S_hla.mk:ERROR: The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}${RESET_TXT})
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_JAVA_LIB_PATH}/.. -L${RTI_JAVA_LIB_PATH} -ljava -ljvm -lverify -Wl,-rpath,${RTI_JAVA_LIB_PATH}/.. -Wl,-rpath,${RTI_JAVA_LIB_PATH}

      # Add the CLASSPATH environment variable to the simulation executable.
      export CLASSPATH     += ${RTI_HOME}/lib/prti1516e.jar
      export TRICK_GTE_EXT += CLASSPATH

      # Determine which gcc library version to use.
      ifeq ($(shell echo $(COMPILER_VERSION)\>=7 | bc),1)
         RTI_LIB_PATH = ${RTI_HOME}/lib/gcc73_64
      else ifeq ($(shell echo $(COMPILER_VERSION)\>=5 | bc),1)
         RTI_LIB_PATH = ${RTI_HOME}/lib/gcc52_64
      else ifeq ($(shell echo $(COMPILER_VERSION)\>=4 | bc),1)
         RTI_LIB_PATH = ${RTI_HOME}/lib/gcc41_64
      else
         RTI_LIB_PATH = ${RTI_HOME}/lib/gcc34_64
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_LIB_PATH} -lrti1516e64 -lfedtime1516e64 -Wl,-rpath,${RTI_LIB_PATH}

      # On Ubuntu, the user needs to add the LD_LIBRARY_PATH shown below to
      # their environment.
      ifneq ("$(wildcard /etc/lsb-release)","")
        $(info ${GREEN_TXT}S_hla.mk:INFO: Add this to your .bashrc file: export LD_LIBRARY_PATH=${RTI_JAVA_LIB_PATH}/..:${RTI_JAVA_LIB_PATH}:${RTI_LIB_PATH}${RESET_TXT})
      endif

   else ifeq ($(RTI_VENDOR),MAK_HLA_Evolved)
      TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -lrti1516e64 -lfedtime1516e64
   # ERAU: Adding support for OpenRTI
   else ifeq ($(RTI_VENDOR),OpenRTI_HLA)
      TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -lOpenRTI -lrti1516e -lfedtime1516e -v -Wl,-rpath,${RTI_HOME}/lib
   else
      $(error ${RED_TXT}S_hla.mk:ERROR: Unsupported RTI_VENDOR '${RTI_VENDOR}', must specify one of Pitch_HLA_4, Pitch_HLA_Evolved, or MAK_HLA_Evolved.${RESET_TXT})
   endif

endif
