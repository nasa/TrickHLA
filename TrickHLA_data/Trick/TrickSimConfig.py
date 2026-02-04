##############################################################################
#
# @file Trick/TrickSimConfig.py
# @ingroup Trick
# @brief Definition of the Trick simulation configuration Python abstract base
# class.
# 
# This is the base implementation for the Trick simulation configuration class.
# This Python class is used to define Trick simulation configuration parameters
# in the Trick input file.
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

import sys
import trick
from enum import IntEnum
from abc import ABC, abstractmethod

class TrickSimConfig( ABC ):
   
   # Make this a singleton class.
   _instance = None
   
   # Override the allocation function to ensure on and only one instance.
   def __new__( tsc, *args, **kwargs ):
      
      # Check if and instance already exists.
      if tsc._instance is None:
         # Not already allocated.  So, allocate one and only one instance.
         tsc._instance = super( TrickSimConfig, tsc).__new__(tsc)
      else:
         # Already allocated!  This is an error.
         sys.exit( 'TrickSimConfig:Error: Can only have one TrickSimConfig instance!' )
         
      # Return the single existing instance.
      return tsc._instance

   @abstractmethod
   # Class constructor.
   def __init__( self,
                 name ):
            
      return
   
   
   # Common Trick realtime configuration.
   def realtime( self, frame_rate = 0.1 ):
      
      trick.frame_log_on()
      trick.real_time_enable()
      trick.exec_set_software_frame( frame_rate )
      trick.itimer_enable()

      trick.exec_set_enable_freeze( True )
      trick.exec_set_freeze_command( False )
      
      return
   
   
   # Setup to use the Trick Simulation Control Panel
   def sim_control_panel( self ):

      trick.exec_set_freeze_command( True )
      self.simControlPanel = trick.SimControlPanel()
      trick.add_external_application( self.simControlPanel )
      
      return
   
   