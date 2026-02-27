##############################################################################
#
# @file BallSimConfig.py
# @ingroup TrickHLA
# @brief Simulation configuration data for the Ball sim.
# 
# This Python class sets up the default Trick simulation configuration
# parameters for the Ball sim.
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
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, January 2026, --, Initial version. }
# @revs_end
#
##############################################################################

import os
import trick

# Import the TrickHLA base Trick Simulation Configuration class.
from TrickHLA_data.Trick.TrickSimConfig import *

class BallSimConfig( TrickSimConfig ):
   
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
   
   def ball_graphics( self ):
      
      # Set the Ball graphics command path.
      self.command_path = '../../../models/Ball/graphics/dist/BallDisplay.jar'
      
      # Check to see if the command exists.
      if ( os.path.isfile( self.command_path ) ) :
         
         # Build the Ball graphics command.
         self.ball_graphics_command = 'java -jar ' + self.command_path
         
         # Set this up as a Trick external application.
         self.ballGraphicsApp = trick.ExternalApplication()
         self.ballGraphicsApp.set_startup_command( self.ball_graphics_command )
         self.ballGraphicsApp.set_arguments( str( self.variable_server_port ) )
         trick.add_external_application( self.ballGraphicsApp )
                                      
      else :
         print('=====================================================================================')
         print('BallDisplay needs to be built. Please \"cd\" into models/Ball/graphics and type \"make\".')
         print('=====================================================================================')
         
      return
   