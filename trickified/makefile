# To generate S_source.hh:
# cd $(TRICKHLA_HOME)/include && find . ! -name 'TSyncCTETimeline.hh' -name "*.hh" -type f | LC_ALL=C sort | sed 's/\.\//#include "/g' | sed 's/\.hh/\.hh"/g' > $(TRICKHLA_HOME)/trickified/S_source.hh
#

ifndef TRICK_HOME
	$(error TRICK_HOME must be set)
endif

ifndef TRICKHLA_HOME
	$(error TRICKHLA_HOME must be set)
endif

TRICKIFY := $(TRICK_HOME)/share/trick/makefiles/trickify.mk

ifeq ($(wildcard $(TRICKIFY)),)
	$(error This makefile requires at least Trick 25.0)
endif

include $(TRICKHLA_HOME)/makefiles/S_hla.mk

export TRICKIFY_OBJECT_NAME := trickified_TrickHLA.o
#export TRICKIFY_CXX_FLAGS := $(TRICK_CXXFLAGS) -I$(TRICK_HOME)/include -DTRICK_VER=25
export TRICKIFY_CXX_FLAGS := -I$(TRICKHLA_HOME)/include -I$(TRICK_HOME)/include -DTRICK_VER=25

#ifdef TRICKIFY_ICG_EXCLUDE
#   TRICKIFY_ICG_EXCLUDE += :${RTI_INCLUDE}
#else
#   TRICKIFY_ICG_EXCLUDE = ${RTI_INCLUDE}
#endif
#ifdef TRICKIFY_SWIG_EXCLUDE
#   TRICKIFY_SWIG_EXCLUDE += :${RTI_INCLUDE}
#else
#   TRICKIFY_SWIG_EXCLUDE = ${RTI_INCLUDE}
#endif

all: S_source.hh
	$(MAKE) -f $(TRICKIFY)

clean:
	@rm -rf build python .trick $(TRICKIFY_OBJECT_NAME) S_source.hh full_file

S_source.hh:
	@cd $(TRICKHLA_HOME)/include && find . -name "*.hh" -type f | LC_ALL=C sort | sed 's/\.\//#include "/g' | sed 's/\.hh/\.hh"/g' > $(TRICKHLA_HOME)/trickified/S_source.hh
