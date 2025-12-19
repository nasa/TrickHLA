##############################################################################
# PURPOSE:
#    (This is a Python input file class to set up the general parameters that
#     describe an Space Reference FOM PhysicalEntity object.)
#
# REFERENCE:
#    (Trick documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAObjectConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA) (July 2023) (--) (Space FOM development)))
##############################################################################
import trick
from ..TrickHLA.TrickHLAObjectConfig import *
from ..TrickHLA.TrickHLAAttributeConfig import *


class SpaceFOMPhysicalEntityObject( TrickHLAObjectConfig ):

   # The PhysicalEntity FOM name is fixed for the SpaceFOM.
   entity_FOM_name = 'PhysicalEntity'

   # Trick simulation object name (constructed).
   trick_entity_sim_obj_name = None

   def __init__( self,
                 create_entity_object,
                 entity_instance_name,
                 entity_S_define_instance,
                 entity_S_define_instance_name,
                 entity_conditional         = None,
                 entity_lag_comp            = None,
                 entity_lag_comp_type       = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 entity_ownership           = None,
                 entity_deleted             = None,
                 entity_thla_manager_object = None,
                 entity_thread_IDs          = None ):

      # Copy the PhysicalEntity federation execution instance name.
      entity_federation_instance_name = str( entity_instance_name )

      # Save the PhysicalEntity name to use for trick_data_name generation.
      self.trick_entity_sim_obj_name = str( entity_S_define_instance_name )

      # By SpaceFOM rule 6-1 the PhysicalEntity instance name must exactly
      # match the PhysicalEntity name in the data.
      if ( create_entity_object ):
         entity_S_define_instance.set_name( entity_instance_name )
      else:
         entity_S_define_instance.set_name( '' )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     thla_create               = create_entity_object,
                                     thla_instance_name        = entity_instance_name,
                                     thla_FOM_name             = self.entity_FOM_name,
                                     thla_lag_comp_instance    = entity_lag_comp,
                                     thla_lag_comp_type        = entity_lag_comp_type,
                                     thla_ownership_instance   = entity_ownership,
                                     thla_deleted_instance     = entity_deleted,
                                     thla_conditional_instance = entity_conditional,
                                     thla_packing_instance     = entity_S_define_instance,
                                     thla_object               = entity_thla_manager_object,
                                     thla_thread_IDs           = entity_thread_IDs )

      # Build the object attribute list.
      self.add_attributes()

      return

   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      TrickHLAObjectConfig.initialize( self, thla_object )

      return

   def add_attributes( self ):

      # Short cut the sim_object name for the PhysicalEntity data.
      entity_instance_name = self.trick_entity_sim_obj_name

      # Set up the map to the reference PhysicalEntity's name.
      trick_data_name = str( entity_instance_name ) + '.pe_packing_data.name'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'name',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      # Set up the map to the reference PhysicalEntity's type.
      trick_data_name = str( entity_instance_name ) + '.pe_packing_data.type'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'type',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      # Set up the map to the reference PhysicalEntity's status.
      trick_data_name = str( entity_instance_name ) + '.pe_packing_data.status'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'status',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      # Set up the map to the name of the PhysicalEntity's parent reference frame.
      trick_data_name = str( entity_instance_name ) + '.pe_packing_data.parent_frame'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'parent_reference_frame',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      # Set up the map to the PhysicalEntity's space/time coordinate state.
      trick_data_name = str( entity_instance_name ) + '.stc_encoder.buffer'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'state',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_NONE )
      self.add_attribute( attribute )

      # Set up the map to the PhysicalEntity's translational acceleration.
      trick_data_name = str( entity_instance_name ) + '.pe_packing_data.accel'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'acceleration',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the PhysicalEntity's rotational acceleration.
      trick_data_name = str( entity_instance_name ) + '.pe_packing_data.ang_accel'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'rotational_acceleration',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the PhysicalEntity's center of mass.
      trick_data_name = str( entity_instance_name ) + '.pe_packing_data.cm'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'center_of_mass',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      # Set up the map to the PhysicalEntity's struct to body attitude quaternion.
      trick_data_name = str( entity_instance_name ) + '.quat_encoder.buffer'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'body_wrt_structural',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_NONE )
      self.add_attribute( attribute )

      return
