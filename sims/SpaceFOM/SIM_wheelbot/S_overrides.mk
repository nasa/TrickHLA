#=============================================================================
# Allow user to specify their own package locations.
#   - File is skipped if not present
#=============================================================================
-include ${HOME}/.trickhla/S_user_env.mk

ifdef TRICKHLA_HOME
include ${TRICKHLA_HOME}/makefiles/S_hla.mk
else
$(error "You must set the TRICKHLA_HOME environment variable.")
endif

#=============================================================================
# Construct Build Environment
#=============================================================================

TRICK_CFLAGS    += -I.
TRICK_CFLAGS    += -I${TRICKHLA_HOME}/models/Wheelbot
TRICK_CXXFLAGS  += -I.
TRICK_CXXFLAGS  += -I${TRICKHLA_HOME}/models/Wheelbot
