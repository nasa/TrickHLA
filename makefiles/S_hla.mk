#=============================================================================
# Set up HLA and TrickHLA environment.
# Note the developer should check out latest TrickHLA tag to location
# defined below in $TRICKHLA_HOME
#=============================================================================
TRICKHLA_HOME ?= ${MODEL_PACKAGE_HOME}/TrickHLA
RTI_VENDOR    ?= Pitch_HLA_Evolved
RTI_HOME      ?= ${HOME}/rti/pRTI1516e

# Make sure the critical environment variable paths we depend on are valid.
ifeq ("$(wildcard ${TRICKHLA_HOME})","")
   $(error "Must specify a valid TRICKHLA_HOME environment variable, which is currently set to invalid path ${TRICKHLA_HOME}")
endif
ifeq ("$(wildcard ${RTI_HOME})","")
   $(error "Must specify a valid RTI_HOME environment variable, which is currently set to invalid path ${RTI_HOME}")
endif

# These are for TrickHLA
TRICK_SFLAGS   += -I${TRICKHLA_HOME}/S_modules
TRICK_CFLAGS   += -I${TRICKHLA_HOME}/include -I${TRICKHLA_HOME}/models -DIEEE_1516_2010
TRICK_CXXFLAGS += -I${TRICKHLA_HOME}/include -I${TRICKHLA_HOME}/models -DIEEE_1516_2010

# These are for the HLA RTI libraries
ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
   TRICK_CFLAGS   += -I${RTI_HOME}/include
   TRICK_CXXFLAGS += -I${RTI_HOME}/include
else ifeq ($(RTI_VENDOR),Mak)
   TRICK_CFLAGS   += -DRTI_VENDOR=Mak_HLA_Evolved -I${RTI_HOME}/include/HLA1516E
   TRICK_CXXFLAGS += -DRTI_VENDOR=Mak_HLA_Evolved -I${RTI_HOME}/include/HLA1516E
else ifeq ($(RTI_VENDOR),Mak_HLA_Evolved)
   TRICK_CFLAGS   += -DRTI_VENDOR=Mak_HLA_Evolved -I${RTI_HOME}/include/HLA1516E
   TRICK_CXXFLAGS += -DRTI_VENDOR=Mak_HLA_Evolved -I${RTI_HOME}/include/HLA1516E
else
   $(error "Must specify an RTI vendor: Pitch_HLA_Evolved, Mak or Mak_HLA_Evolved.")
endif

ifeq ($(TRICK_HOST_TYPE),Darwin)

   ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info "User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}")
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
         $(error "The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}")
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error "The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}")
      endif

      # Determine what compiler Trick is using to figure out the correct
      # Pitch library to use.
      ifneq (,$(findstring trick-gte, $(shell which trick-gte)))
         CPPC_CMD = $(shell trick-gte TRICK_CXX)
         ifeq (,$(CPPC_CMD))
            CPPC_CMD = $(shell trick-gte TRICK_CPPC)
         endif
      else
         CPPC_CMD = $(shell gte TRICK_CXX)
         ifeq (,$(CPPC_CMD))
            CPPC_CMD = $(shell gte TRICK_CPPC)
         endif
      endif
      ifneq (,$(findstring clang, $(shell $(CPPC_CMD) --version | grep clang)))
         export DYLD_LIBRARY_PATH += :${RTI_HOME}/lib/clang5
         TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -L${RTI_HOME}/lib/clang5 -v -Wl,-rpath,${RTI_HOME}/lib/clang5 -lrti1516e -lfedtime1516e -L${RTI_JAVA_LIB_PATH} -v -Wl,-rpath,${RTI_JAVA_LIB_PATH} -ljvm
      else
         export DYLD_LIBRARY_PATH += :${RTI_HOME}/lib/gcc42
         TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib/gcc42 -lrti1516e -lfedtime1516e
      endif
      # Add the CLASSPATH and DYLD_LIBRARY_PATH environment variables to the 
      # simulation executable.
      export CLASSPATH += ${RTI_HOME}/lib/prti1516e.jar
      export TRICK_GTE_EXT += CLASSPATH DYLD_LIBRARY_PATH
   else
      $(error "Only Pitch has support for Mac OS!")
   endif

else

   ifeq ($(RTI_VENDOR),Pitch_HLA_Evolved)
      # Allow the user to override RTI_JAVA_HOME or RTI_JAVA_LIB_PATH,
      # otherwise we provide defaults.
      ifdef RTI_JAVA_HOME
         $(info "User defined RTI_JAVA_HOME = ${RTI_JAVA_HOME}")
      endif
      RTI_JAVA_HOME ?= ${RTI_HOME}/jre
      ifneq ("$(wildcard ${RTI_JAVA_HOME}/jre/lib/amd64/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/jre/lib/amd64/server
      else ifneq ("$(wildcard ${RTI_JAVA_HOME}/lib/server)","")
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/server
      else
         # Default to JRE that came with the Pitch RTI if needed.
         RTI_JAVA_LIB_PATH ?= ${RTI_JAVA_HOME}/lib/amd64/server
      endif
      # Verify the RTI Java Home and Lib paths.
      ifeq ("$(wildcard ${RTI_JAVA_HOME})","")
         $(error "The path specified by RTI_JAVA_HOME is invalid for ${RTI_JAVA_HOME}")
      endif
      ifeq ("$(wildcard ${RTI_JAVA_LIB_PATH})","")
         $(error "The path specified by RTI_JAVA_LIB_PATH is invalid for ${RTI_JAVA_LIB_PATH}")
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_JAVA_LIB_PATH}/.. -L${RTI_JAVA_LIB_PATH} -ljava -ljvm -lverify -Wl,-rpath,${RTI_JAVA_LIB_PATH}/.. -Wl,-rpath,${RTI_JAVA_LIB_PATH}

      # Add the CLASSPATH environment variable to the simulation executable.
      export CLASSPATH += ${RTI_HOME}/lib/prti1516e.jar
      export TRICK_GTE_EXT += CLASSPATH

      # Determine what compiler Trick is using to figure out the correct
      # Pitch library to use.
      ifneq (,$(findstring trick-gte, $(shell which trick-gte)))
         CPPC_CMD = $(shell trick-gte TRICK_CXX)
         ifeq (,$(CPPC_CMD))
            CPPC_CMD = $(shell trick-gte TRICK_CPPC)
         endif
      else
         CPPC_CMD = $(shell gte TRICK_CXX)
         ifeq (,$(CPPC_CMD))
            CPPC_CMD = $(shell gte TRICK_CPPC)
         endif
      endif

      # Determine what gcc library version to use.
      COMPILER_VERSION = $(shell $(CPPC_CMD) -dumpversion | cut -d . -f 1)
      COMPILER_GTE_7 = $(shell echo $(COMPILER_VERSION)\>=7 | bc )
      COMPILER_GTE_5 = $(shell echo $(COMPILER_VERSION)\>=5 | bc )
      ifeq ($(COMPILER_GTE_7),1)
         RTI_LIB_PATH = ${RTI_HOME}/lib/gcc73_64
      else ifeq ($(COMPILER_GTE_5),1)
         RTI_LIB_PATH = ${RTI_HOME}/lib/gcc52_64
      else
         RTI_LIB_PATH = ${RTI_HOME}/lib/gcc41_64
      endif
      TRICK_USER_LINK_LIBS += -L${RTI_LIB_PATH} -lrti1516e64 -lfedtime1516e64 -Wl,-rpath,${RTI_LIB_PATH}

      # On Ubuntu, the user needs to add the LD_LIBRARY_PATH shown below to
      # their environment.
      ifneq ("$(wildcard /etc/lsb-release)","")
        $(info "Add this to your .bashrc file: export LD_LIBRARY_PATH=${RTI_JAVA_LIB_PATH}/..:${RTI_JAVA_LIB_PATH}:${RTI_LIB_PATH}")
      endif

   else ifeq ($(RTI_VENDOR),Mak)
      TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -lrti1516e64 -lfedtime1516e64
   else ifeq ($(RTI_VENDOR),Mak_HLA_Evolved)
      TRICK_USER_LINK_LIBS += -L${RTI_HOME}/lib -lrti1516e64 -lfedtime1516e64
   else
      $(error "Must specify an RTI vendor: Pitch_HLA_Evolved, Mak, or Mak_HLA_Evolved")
   endif

endif

ifdef TRICK_ICG_EXCLUDE
   TRICK_ICG_EXCLUDE += :${RTI_HOME}/include
else
   TRICK_ICG_EXCLUDE = ${RTI_HOME}/include
endif

ifdef TRICK_SWIG_EXCLUDE
   TRICK_SWIG_EXCLUDE += :${RTI_HOME}/include
else
   TRICK_SWIG_EXCLUDE = ${RTI_HOME}/include
endif
