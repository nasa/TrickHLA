#!/bin/bash
#
# Script to make all the TrickHLA simulations.
# January 2, 2025: Dan Dexter/dan.e.dexter@nasa.gov
#

if [ ! -d $TRICKHLA_HOME ]; then
   echo "ERROR: TRICKHLA_HOME ${TRICKHLA_HOME} does not exist, exiting..."
   exit 1
fi
echo "TRICKHLA_HOME: ${TRICKHLA_HOME}"
cd ${TRICKHLA_HOME}

sim_pkgs_list=($(ls ${TRICKHLA_HOME}/sims))
for sim_pkg in "${sim_pkgs_list[@]}"
do
   sim_list=($(ls ${TRICKHLA_HOME}/sims/${sim_pkg}))
   for sim in "${sim_list[@]}"
   do
      echo "----------------------------------------------------------"
      echo "sims/${sim_pkg}/${sim}"
      cd ${TRICKHLA_HOME}/sims/${sim_pkg}/${sim}
      if [ -f makefile ]; then
         make
         status=$?
         if [ $status -ne 0 ]; then
            echo "ERROR"
            exit 1
         fi
      fi
   done
done
