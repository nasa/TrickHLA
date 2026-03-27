##############################################################################
#
# @file BallFederateConfig.py
# @ingroup TrickHLA
# @brief Federate configuration data for the Ball HLA sim.
# 
# This Python class sets up the default HLA federate configuration
# parameters for the Ball HLA sim.
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
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, February 2026, --, Initial version. }
# @revs_end
#
##############################################################################

import os
import trick

# Import the SpaceFOM base Federate Configuration class.
from TrickHLA_data.SpaceFOM.SpaceFOMFederateConfig2 import *

class BallFederateConfig( SpaceFOMFederateConfig2 ):
   
   ball_config = None
   
   # Class constructor.
   def __init__( self,
                 thla,
                 thla_init,
                 federate_name,
                 federate_type   = 'Ball',
                 federation_name = 'BallFederation',
                 enabled         = True ):
      
      # Save the Ball simple sim configuration.
      self.ball_config = thla.ExCO

      # Call the base class constructor.
      SpaceFOMFederateConfig2.__init__( self,
                                        thla_federate        = thla.federate,
                                        thla_control         = thla.execution_control,
                                        thla_config          = thla.ExCO,
                                        thla_federation_name = federation_name,
                                        thla_federate_name   = federate_name,
                                        thla_enabled         = enabled,
                                        thla_FOMs_path       = 'FOMs/SpaceFOM/' )

      # Add in required Ball FOM modules.
      self.add_FOM_module( 'FOMs/BallFOM.xml' )

      # Set the simulation timeline to be used for time computations.
      thla.execution_control.sim_timeline = thla_init.sim_timeline

      # Set the scenario timeline to be used for configuring federation freeze times.
      thla.execution_control.scenario_timeline = thla_init.scenario_timeline

      # Configure the CRC.
      # Pitch specific local settings designator:
      thla.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'

      # Specify the HLA base time unit (default: trick.HLA_BASE_TIME_MICROSECONDS)
      # and scale the Trick time tics value.
      self.set_HLA_base_time_unit_and_scale_trick_tics( trick.HLA_BASE_TIME_MICROSECONDS )

      # Must specify a federate HLA lookahead value in seconds.
      self.set_lookahead_time( 0.10 )

      # Must specify the Least Common Time Step for all federates in the
      # federation execution.
      # NOTE: This needs to be set AFTER the Master role is set.  Only the
      # Master federate can set the LCTS for a SpaceFOM simulation.
      #self.set_least_common_time_step( 0.10 )

      # Configure the federate time management.
      self.set_time_regulating( True )
      self.set_time_constrained( True )
            
      return
   

   def initialize( self ):

      # You can only initialize once.
      if self.initialized:
         print( 'BallFederateConfig.initialize(): Warning, already initialized! Ignoring!' )
         return

      # Call the base class initialization utility function.
      TrickHLAFederateConfig.initialize( self )

      return