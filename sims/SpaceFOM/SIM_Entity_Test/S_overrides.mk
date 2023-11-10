#=============================================================================
# Allow user to specify their own package locations.
#   - File is skipped if not present
#=============================================================================
-include ${HOME}/.trickhla/S_user_env.mk

ifdef TRICKHLA_HOME
TRICK_SFLAGS += -I${TRICKHLA_HOME}/S_modules
include ${TRICKHLA_HOME}/makefiles/S_hla.mk
else
$(error "You must set the TRICKHLA_HOME environment variable.")
endif

#=============================================================================
# Construct Build Environment
#=============================================================================

TRICK_CFLAGS    += -Wno-deprecated-declarations
TRICK_CFLAGS    += -I.
TRICK_CXXFLAGS  += -Wno-deprecated-declarations
TRICK_CXXFLAGS  += -I.

# Use the Trick Stand-Alone Integrators if the SAIntegrator/lib directory exists.
# NOTE: You will also have to build the Trick SAInteg library.
ifneq ($(wildcard ${TRICK_HOME}/trick_source/trick_utils/SAIntegrator/lib/.*),)
TRICK_USER_LINK_LIBS += -L${TRICK_HOME}/trick_source/trick_utils/SAIntegrator/lib -lSAInteg
endif
