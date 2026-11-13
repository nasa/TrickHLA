#!/bin/bash
#
# Format Python code and sort imports.
# pipx install black isort
#
# November 13, 2026: Dan Dexter/dan.e.dexter@nasa.gov
#

if [ ! -d $TRICKHLA_HOME ]; then
   echo "ERROR: TRICKHLA_HOME ${TRICKHLA_HOME} does not exist, exiting..."
   exit 1
fi
cd ${TRICKHLA_HOME}

echo "----------------------------------------------------------"
echo "Sorting Python imports..."
isort ${TRICKHLA_HOME}

echo "----------------------------------------------------------"
echo "Formatting Python code..."
black --line-length 90 ${TRICKHLA_HOME}
