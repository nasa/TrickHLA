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

# Use the TSync CTE card if the path to the driver is set.
ifdef TSYNC_HOME
TRICK_CFLAGS   += -DTSYNC_CTE -I$(TSYNC_HOME)/libtsync/include
TRICK_CXXFLAGS += -DTSYNC_CTE -I$(TSYNC_HOME)/libtsync/include
endif

#=============================================================================
# Construct Build Environment
#=============================================================================

TRICK_CFLAGS   += -I.
TRICK_CXXFLAGS += -I.

