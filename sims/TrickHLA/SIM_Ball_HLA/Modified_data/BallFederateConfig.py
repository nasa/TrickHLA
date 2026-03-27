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

# Import the TrickHLA base Trick Simulation Configuration class.
from TrickHLA_data.TrickHLA.TrickHLAFederateConfig import *

class BallFederateConfig( TrickHLAFederateConfig ):
   
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
      self.ball_config = thla.simple_sim_config

      # Call the base class constructor.
      TrickHLAFederateConfig.__init__( self,
                                       thla_federate        = thla.federate,
                                       thla_control         = thla.execution_control,
                                       thla_config          = thla.simple_sim_config,
                                       thla_federation_name = federation_name,
                                       thla_federate_name   = federate_name,
                                       thla_enabled         = enabled )

      # Add in required Ball FOM modules.
      self.add_FOM_module( 'FOMs/SimpleSimConfig.xml' )
      self.add_FOM_module( 'FOMs/BallFOM.xml' )

      # Set the simulation timeline to be used for time computations.
      thla.execution_control.sim_timeline = thla_init.sim_timeline

      # Set the scenario timeline to be used for configuring federation freeze times.
      thla.execution_control.scenario_timeline = thla_init.scenario_timeline


      # Configure the CRC.
      # Pitch specific local settings designator:
      thla.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
      
      # Set the federate lookahead time.
      self.set_lookahead_time( 0.100 )

      # Configure the federate time management.
      self.set_time_regulating( True )
      self.set_time_constrained( True )
      
      #---------------------------------------------
      # Set up for simulation configuration.
      #---------------------------------------------
      thla.simple_sim_config.owner = federate_name
            
      return
   

   def initialize( self ):

      # You can only initialize once.
      if self.initialized:
         print( 'BallFederateConfig.initialize(): Warning, already initialized! Ignoring!' )
         return

      # Call the base class initialization utility function.
      TrickHLAFederateConfig.initialize( self )

      return
   

   def run_duration( self, time ):

      # You can only initialize once.
      if self.initialized:
         print( 'BallFederateConfig.run_duration(): Warning, already initialized! Ignoring!' )
         return

      # Set the run duration in the simple simulation configuration object.
      self.ball_config.run_duration = time

      return