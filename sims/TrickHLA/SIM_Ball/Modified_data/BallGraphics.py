##############################################################################
#
# @file BallGraphics.py
# @ingroup TrickHLA
# @brief Input file class to start up the Ball Graphics application.
# 
# This Python class is used to startup the Ball JAVA-based graphics.
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

class BallGraphics( object ):
   
   # Class constructor.
   def __init__( self ):

      # Get the variable server port.
      self.variable_server_port = trick.var_server_get_port();
      
      # Set the Ball graphics command path.
      self.command_path = "../../../models/Ball/graphics/dist/BallDisplay.jar"
      
      # Check to see if the command exists.
      if ( os.path.isfile( self.command_path ) ) :
         self.command = "java -jar " \
                        + self.command_path + " " \
                        + str( self.variable_server_port ) + " &"
                        
         print( 'Executing command: ' + self.command )
      else :
         print('==================================================================================')
         print('BallDisplay needs to be built. Please \"cd\" into models/Ball/graphics and type \"make\".')
         print('==================================================================================')
      
            
      return
   
   # Function to start the application.
   def start( self ):
      os.system( self.command )
      return
   