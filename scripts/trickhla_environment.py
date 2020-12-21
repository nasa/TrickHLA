############################################################################
# @file trickhla_environment.py
# @brief Sets up the Python environment for TrickHLA Python scripts.
#
# This is a Python module used to setup the Python environment for the
# Python based scripts that are used to manage the TrickHLA code base and
# repository.
############################################################################
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, April 2019, --, Initial creation.}
# @revs_end
############################################################################
import os
import sys

# Check to see if the $TRICKHLA_HOME environment variable is defined and set.
trickhla_home = os.environ.get( "TRICKHLA_HOME" )
if trickhla_home is None:
   sys.exit( '\033[91m'\
            +'Environment variable TRICKHLA_HOME is not defined!'\
            +'\033[0m\n' )
else:
   if os.path.isdir( trickhla_home ) is False:
      sys.exit( '\033[91m'\
               +'TRICKHLA_HOME not found: '\
               +trickhla_home\
               +'\033[0m\n' )

# Construct the path to the TrickHLA python modules.
trickhla_py_path = trickhla_home + '/scripts'

# Check to see if the python modules directory exists.
if os.path.isdir( trickhla_py_path ) is False:
   sys.exit( '\033[91m'\
            +'ERROR: TrickHLA Python modules path not found: '\
            +trickhla_py_path\
            +'\033[0m\n' )

# Add this to the Python package search path.
sys.path.append( trickhla_py_path )

# Import useful TrickHLA support modules
import trickhla_message
import trickhla_models
