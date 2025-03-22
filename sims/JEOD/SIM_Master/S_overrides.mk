#=============================================================================
# Allow user to specify their own package locations.
#   - File is skipped if not present
#=============================================================================
-include ${HOME}/.trickhla/S_user_env.mk

ifdef JEOD_HOME
   TRICK_SFLAGS += -I${JEOD_HOME}/S_modules
   include $(JEOD_HOME)/bin/jeod/generic_S_overrides.mk
else
   $(error "You must set the JEOD_HOME environment variable.")
endif

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
TRICK_CXXFLAGS += -I.

