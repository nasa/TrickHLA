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

# Add the TSync CTE card if the home path to the driver is set for the
# TSYNC_HOME environment variable.
ifdef TSYNC_HOME
TRICK_CFLAGS         += -DNIOS -I$(TSYNC_HOME)/libtsync/include -I$(TSYNC_HOME)/libtsync/src -I$(TSYNC_HOME)/tsync-driver/include
TRICK_CXXFLAGS       += -DNIOS -I$(TSYNC_HOME)/libtsync/include -I$(TSYNC_HOME)/libtsync/src -I$(TSYNC_HOME)/tsync-driver/include
TRICK_ICG_EXCLUDE    += :$(TSYNC_HOME)
TRICK_USER_LINK_LIBS += -L${TSYNC_HOME}/libtsync/src/build -ltsync
endif

TRICK_CFLAGS   += -I.
TRICK_CXXFLAGS += -I.

