#!/bin/bash
#
# Script to update the Pitch Environment variables.
# February 26, 2026: Dan Dexter/dan.e.dexter@nasa.gov
#
# Add the following to your .bashrc or .zshrc file:
# if [ -e ${TRICKHLA_HOME}/scripts/update_Pitch_env.sh ]; then
#    source ${TRICKHLA_HOME}/scripts/update_Pitch_env.sh
# fi
#

if [ ! -d $RTI_HOME ]; then 
   if [[ -t 0 ]]; then
      echo "WARNING: RTI_HOME ${RTI_HOME} does not exist."
   fi
else
   # Pitch sets system environment variables, so update them to the RTI being used.
   if [ -n "${PRTI6_ROOT}" ]; then
      if [ "$PRTI6_ROOT" != "$RTI_HOME" ]; then
         export PRTI6_ROOT="${RTI_HOME}"
      fi
   fi
   if [ -n "${PitchRTI_ROOT}" ]; then
      if [ "$PitchRTI_ROOT" != "$RTI_HOME" ]; then
         export PitchRTI_ROOT="${RTI_HOME}"
      fi
   fi
   if [ -n "${PRTI1516E_HOME}" ]; then
      if [ "$PRTI1516E_HOME" != "$RTI_HOME" ]; then
         export PRTI1516E_HOME="${RTI_HOME}"
      fi
   fi
fi
