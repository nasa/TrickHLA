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

FOM_HOME?=../../

ifdef FOM_HOME
   TRICK_SFLAGS += -I${FOM_HOME}/S_modules -I${FOM_HOME} -L${FOM_HOME}
#   include ${FOM_HOME}/makefiles/S_hla.mk
else
   $(error S_overrides.mk:ERROR: You must set the FOM_HOME environment variable.)
endif

#=============================================================================
# Construct Build Environment
#=============================================================================

TRICK_CFLAGS   += -I. -I${TRICKHLA_HOME}/include -I${FOM_HOME}/include -I${FOM_HOME}
TRICK_CXXFLAGS += -I. -I${TRICKHLA_HOME}/include -I${FOM_HOME}/include -I${FOM_HOME}

