#!/bin/bash
#
# Check Python code using a lint tool called ruff.
# pipx install ruff
#
# November 16, 2026: Dan Dexter/dan.e.dexter@nasa.gov
#

if [ ! -d $TRICKHLA_HOME ]; then
   echo "ERROR: TRICKHLA_HOME ${TRICKHLA_HOME} does not exist, exiting..."
   exit 1
fi

ruff check ${TRICKHLA_HOME}/scripts
ruff check ${TRICKHLA_HOME}/TrickHLA_data
