##############################################################################
#
# @file WallsObject.py
# @ingroup SpaceFOM
# @brief Simple Walls object configuration.
# 
# This Python class sets up the configuration parameters for the Walls object
# in the Walls HLA sim.
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


class WallsObject( TrickHLAObjectConfig ):

   walls_sim_obj_name = None

   def __init__( self,
                 create_walls_object,
                 walls_instance_name,
                 walls_S_define_instance,
                 walls_S_define_instance_name,
                 walls_conditional         = None,
                 walls_lag_comp            = None,
                 walls_lag_comp_type       = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 walls_ownership           = None,
                 walls_deleted             = None,
                 walls_thla_manager_object = None,
                 walls_thread_IDs          = None ):

      # The Walls FOM name is fixed for the SpaceFOM.
      walls_FOM_name = 'Walls'

      # Copy the walls federation execution instance name.
      walls_federation_instance_name = str( walls_instance_name )

      # Save the walls name to use for trick_data_name generation.
      self.trick_walls_sim_obj_name = str( walls_S_define_instance_name )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     thla_create               = create_walls_object,
                                     thla_instance_name        = walls_instance_name,
                                     thla_FOM_name             = walls_FOM_name,
                                     thla_lag_comp_instance    = walls_lag_comp,
                                     thla_lag_comp_type        = walls_lag_comp_type,
                                     thla_ownership_instance   = walls_ownership,
                                     thla_deleted_instance     = walls_deleted,
                                     thla_conditional_instance = walls_conditional,
                                     thla_packing_instance     = walls_S_define_instance,
                                     thla_object               = walls_thla_manager_object,
                                     thla_thread_IDs           = walls_thread_IDs )

      # Build the object attribute list.
      self.add_attributes()

      return

   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      TrickHLAObjectConfig.initialize( self, thla_object )

      return

   def add_attributes( self ):

      # Short cut the sim_object name for the walls data.
      walls_instance_name = self.trick_walls_sim_obj_name

      # Set up the map to the walls's name.
      trick_data_name = str( walls_instance_name ) + '.ceiling_y_pos'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'ceiling',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the walls right wall position.
      trick_data_name = str( walls_instance_name ) + '.right_wall_x_pos'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'right',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the walls left wall position.
      trick_data_name = str( walls_instance_name ) + '.left_wall_x_pos'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'left',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the walls floor position.
      trick_data_name = str( walls_instance_name ) + '.floor_y_pos'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'floor',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      return
