##############################################################################
#
# @file BallObject.py
# @ingroup SpaceFOM
# @brief Simple Ball object configuration.
# 
# This Python class sets up the configuration parameters for the Ball object
# in the Ball HLA sim.
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
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, March 2026, --, Initial version. }
# @revs_end
#
##############################################################################
import trick
from TrickHLA_data.TrickHLA.TrickHLAObjectConfig import *
from TrickHLA_data.TrickHLA.TrickHLAAttributeConfig import *


class BallObject( TrickHLAObjectConfig ):

   ball_sim_obj_name = None

   def __init__( self,
                 create_ball_object,
                 ball_instance_name,
                 ball_S_define_instance,
                 ball_S_define_instance_name,
                 ball_conditional         = None,
                 ball_lag_comp            = None,
                 ball_lag_comp_type       = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 ball_ownership           = None,
                 ball_deleted             = None,
                 ball_thla_manager_object = None,
                 ball_thread_IDs          = None ):

      # The Ball FOM name is fixed for the SpaceFOM.
      ball_FOM_name = 'Ball'

      # Copy the ball federation execution instance name.
      ball_federation_instance_name = str( ball_instance_name )

      # Save the ball name to use for trick_data_name generation.
      self.trick_ball_sim_obj_name = str( ball_S_define_instance_name )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     thla_create               = create_ball_object,
                                     thla_instance_name        = ball_instance_name,
                                     thla_FOM_name             = ball_FOM_name,
                                     thla_lag_comp_instance    = ball_lag_comp,
                                     thla_lag_comp_type        = ball_lag_comp_type,
                                     thla_ownership_instance   = ball_ownership,
                                     thla_deleted_instance     = ball_deleted,
                                     thla_conditional_instance = ball_conditional,
                                     thla_packing_instance     = ball_S_define_instance,
                                     thla_object               = ball_thla_manager_object,
                                     thla_thread_IDs           = ball_thread_IDs )

      # Set the ball instance name.
      ball_S_define_instance.set_name( ball_instance_name )

      # Build the object attribute list.
      self.add_attributes()

      return

   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      TrickHLAObjectConfig.initialize( self, thla_object )

      return

   def add_attributes( self ):

      # Short cut the sim_object name for the ball data.
      ball_instance_name = self.trick_ball_sim_obj_name

      # Set up the map to the ball's name.
      trick_data_name = str( ball_instance_name ) + '.name'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'name',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      # Set up the map to the time tag for the ball state.
      trick_data_name = str( ball_instance_name ) + '.time'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'time',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the ball state position.
      trick_data_name = str( ball_instance_name ) + '.position'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'position',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the ball state velocity.
      trick_data_name = str( ball_instance_name ) + '.velocity'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'velocity',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the ball state acceleration.
      trick_data_name = str( ball_instance_name ) + '.acceleration'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'acceleration',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the ball state force.
      trick_data_name = str( ball_instance_name ) + '.external_force'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'force',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      return
