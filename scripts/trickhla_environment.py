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

# Append the path to the top level of the top level TrickHLA directory.
# We need this to locate the TrickHLA_data Python data directory.
if trickhla_home not in sys.path :
   sys.path.append( trickhla_home )

# Import useful TrickHLA support modules
from TrickHLA_data.TrickHLAUtils.TrickHLAMessage import *
import trickhla_models
