#=============================================================================
# Allow user to specify their own package locations.
#   - File is skipped if not present
#=============================================================================
-include ${HOME}/.trickhla/S_user_env.mk

ifdef TRICKHLA_HOME
   TRICK_SFLAGS += -I${TRICKHLA_HOME}/S_modules
   include ${TRICKHLA_HOME}/makefiles/S_hla.mk
else
   $(error S_overrides.mk:ERROR: You must set the TRICKHLA_HOME environment variable.)
endif

#=============================================================================
# Construct Build Environment
#=============================================================================

TRICK_CFLAGS   += -I.
TRICK_CFLAGS   += -DTHLA_CHECK_SEND_AND_RECEIVE_COUNTS
TRICK_CFLAGS   += -DTHLA_CYCLIC_READ_TIME_STATS
TRICK_CFLAGS   += -O3
TRICK_CXXFLAGS += -I.
TRICK_CXXFLAGS += -DTHLA_CHECK_SEND_AND_RECEIVE_COUNTS
TRICK_CXXFLAGS += -DTHLA_CYCLIC_READ_TIME_STATS
TRICK_CXXFLAGS += -O3

