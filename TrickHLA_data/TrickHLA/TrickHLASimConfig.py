##############################################################################
#
# @file TrickHLASimConfig.py
# @ingroup TrickHLA
# @brief Base TrickHLA simulation configuration data.
# 
# This Python class sets up the default Trick simulation configuration
# parameters.
# 
# @copyright Copyright 2026 United States Government as represented by the
# Administrator of the National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Other Rights Reserved.
# 
# \par<b>Responsible Organization</b>
# Simulation and Graphics Branch, Mail Code ER7\n
# Software, Robotics & Simulation Division\n
# NASA, Johnson Space Center\n
# # 2101 NASA Parkway, Houston, TX  77058
# 
# @python_module{Trick}
#
# @revs_title
# @revs_begin
# @rev_entry{ Dan Dexter, NASA JSC, TrickHLA, May 2026, --, Initial version. }
# @revs_end
#
##############################################################################

import os
import trick

# Import the TrickHLA base Trick Simulation Configuration class.
from TrickHLA_data.Trick.TrickSimConfig import *

class TrickHLASimConfig( TrickSimConfig ):

   # Class constructor.
   def __init__( self,
                 sim_name ):

      # Set the name for the simulation.
      self.sim_name = sim_name

      # Get the variable server port.
      self.variable_server_port = trick.var_server_get_port();

      # Call the TrickDataRecordingGroup constructor.
      TrickSimConfig.__init__( self, sim_name )

      return
