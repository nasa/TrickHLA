##############################################################################
#
# @file BallRealtime.py
# @ingroup TrickHLA
# @brief Input file class to setup Trick realtime parameters.
# 
# This Python class is used to setup the realtime parameters for Trick.
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

import trick

class BallRealtime( object ):
   
   # Class constructor.
   def __init__( self ):
      
      trick.frame_log_on()
      trick.real_time_enable()
      trick.exec_set_software_frame(0.1)
      trick.itimer_enable()

      trick.exec_set_enable_freeze(True)
      trick.exec_set_freeze_command(True)

      self.simControlPanel = trick.SimControlPanel()
      trick.add_external_application( self.simControlPanel )
            
      return