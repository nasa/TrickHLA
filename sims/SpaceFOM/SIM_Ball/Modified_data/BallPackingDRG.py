##############################################################################
#
# @file BallPackingDRG.py
# @ingroup TrickHLA
# @brief Definition of the Ball Packing Data Recording Group.
# 
# This Python class sets up the Ball packing that need to be recorded.
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
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, October 2026, --, Initial version. }
# @revs_end
#
##############################################################################


# Import the TrickHLA base Trick Data Recording Classes.
from TrickHLA_data.Trick.TrickDataRecordingGroup import *

class BallPackingDRG( TrickDataRecordingGroup ):
   
   # Class constructor.
   def __init__( self,
                 base_name,
                 group_name,
                 rate,
                 format     = TrickDRFormat.ASCII,
                 when       = TrickDRWhen.ALWAYS,
                 change_var = None,
                 buffer     = TrickDRBuffer.BUFFER,
                 enabled    = True ):

      # Set the base name for looking up the state variables.
      self.base_name = base_name
      
      # Call the TrickDataRecordingGroup constructor.
      TrickDataRecordingGroup.__init__( self,
                                        group_name,
                                        rate,
                                        format,
                                        when,
                                        change_var,
                                        buffer,
                                        enabled )
      
      # Add the state recording variables.
      self.drg.add_variable( self.base_name + '.packing.time' )
      self.drg.add_variable( self.base_name + '.packing.position[0]' )
      self.drg.add_variable( self.base_name + '.packing.position[1]' )
      self.drg.add_variable( self.base_name + '.packing.velocity[0]' )
      self.drg.add_variable( self.base_name + '.packing.velocity[1]' )
      self.drg.add_variable( self.base_name + '.packing.acceleration[0]' )
      self.drg.add_variable( self.base_name + '.packing.acceleration[1]' )
      self.drg.add_variable( self.base_name + '.packing.external_force[0]' )
      self.drg.add_variable( self.base_name + '.packing.external_force[1]' )
            
      return
   
   
