#!/bin/bash
#
# Format Python code and sort imports.
# pipx install black isort ruff
#
# November 13, 2026: Dan Dexter/dan.e.dexter@nasa.gov
#

if [ ! -d $TRICKHLA_HOME ]; then
   echo "ERROR: TRICKHLA_HOME ${TRICKHLA_HOME} does not exist, exiting..."
   exit 1
fi
cd ${TRICKHLA_HOME}

# Either use ruff for sorting imports and formatting code or use the
# combination of isort and black instead.
use_ruff=true

echo "----------------------------------------------------------"
echo "Sorting Python imports..."
if $use_ruff; then
   ruff check --select I --fix ${TRICKHLA_HOME}
else
   isort ${TRICKHLA_HOME}
fi

echo "----------------------------------------------------------"
echo "Formatting Python code..."
if $use_ruff; then
   # Reformat to use 3 spaces for indents.
   ruff format --config "indent-width = 3" --config "line-length = 90" ${TRICKHLA_HOME}
else
   # Black is hard coded to use 4 spaces for indents.
   black --line-length 90 ${TRICKHLA_HOME}
fi
