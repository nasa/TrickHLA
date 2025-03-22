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
TRICK_CXXFLAGS += -I.

# Only for Linux, add the TSync CTE card if the home path to the driver is
# set for the TSYNC_HOME environment variable. TSync Driver:
# https://safran-navigation-timing.com/portal/public-downloads/latest-tsyncpcie-update-files/
ifdef TSYNC_HOME
   ifeq ($(TRICK_HOST_TYPE),Linux)
      TRICK_CFLAGS         += -DNIOS -I$(TSYNC_HOME)/libtsync/include -I$(TSYNC_HOME)/tsync-driver/include
      TRICK_CXXFLAGS       += -DNIOS -I$(TSYNC_HOME)/libtsync/include -I$(TSYNC_HOME)/tsync-driver/include
      TRICK_USER_LINK_LIBS += -L${TSYNC_HOME}/libtsync/src/build -ltsync
      ifdef TRICK_ICG_EXCLUDE
         TRICK_ICG_EXCLUDE += :$(TSYNC_HOME)
      else
         TRICK_ICG_EXCLUDE = $(TSYNC_HOME)
      endif
   endif
endif
