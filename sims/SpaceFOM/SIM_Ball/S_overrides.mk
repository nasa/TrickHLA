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

TRICK_DP_FILES = \
 DP_rt_frame.xml\
 DP_rt_timeline_init.xml\
 DP_rt_timeline.xml\
 DP_rt_trickjobs.xml\
 DP_rt_userjobs.xml

# Get a list of the simulation run directories.
RUN_DIRS = $(wildcard RUN*)

# Use the Trick Stand-Alone Integrators if the SAIntegrator/lib directory exists.
# NOTE: You will also have to build the Trick SAInteg library.
ifneq ($(wildcard ${TRICK_HOME}/trick_source/trick_utils/SAIntegrator/lib/.*),)
   TRICK_USER_LINK_LIBS += -L${TRICK_HOME}/trick_source/trick_utils/SAIntegrator/lib -lSAInteg
endif

.PHONY: clean_runs $(RUN_DIRS) DP_Product

spotless: clean_runs clean_DP

clean_DP: DP_Product

DP_Product:
	@echo "Cleaning up the DP_Product directory: $@"
	@cd DP_Product; rm -f $(TRICK_DP_FILES)

clean_runs: $(RUN_DIRS)

$(RUN_DIRS):
	@echo "Cleaning up run directory: $@"
	@cd $@; rm -f _init_log.csv chkpnt_* log_* S_job_execution S_run_summary send_hs varserver_log

