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

class SpaceFOMPhysicalEntityObject(TrickHLAObjectConfig):

   # The PhysicalEntity FOM name is fixed for the SpaceFOM.
   entity_FOM_name = 'PhysicalEntity'
   
   # Trick simulation object name (constructed).
   trick_entity_sim_obj_name = None

   def __init__( self,
                 create_entity_object,
                 entity_instance_name,
                 entity_S_define_instance,
                 entity_S_define_instance_name,
                 entity_lag_comp = None,
                 entity_ownership = None,
                 entity_thla_manager_object = None ):

      # Copy the PhysicalEntity federation execution instance name.
      entity_federation_instance_name = str( entity_instance_name )

      # Save the PhysicalEntity name to use for trick_data_name generation.
      self.trick_entity_sim_obj_name = str( entity_S_define_instance_name )
      
      # By SpaceFOM rule 6-1 the PhysicalEntity instance name must exactly
      # match the PhysicalEntity name in the data.
      if ( create_entity_object ) :
         entity_S_define_instance.set_name( entity_instance_name )
      else:
         entity_S_define_instance.set_name( '' )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     create_entity_object,
                                     entity_instance_name,
                                     self.entity_FOM_name,
                                     entity_lag_comp,
                                     entity_ownership,
                                     entity_S_define_instance,
                                     entity_thla_manager_object )

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

      ## Set up the map to the reference PhysicalEntity's name.
      trick_data_name = str(entity_instance_name) + '.name'
      attribute = TrickHLAAttributeConfig( 'name',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the reference PhysicalEntity's type.
      trick_data_name = str(entity_instance_name) + '.type'
      attribute = TrickHLAAttributeConfig( 'type',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the reference PhysicalEntity's status.
      trick_data_name = str(entity_instance_name) + '.status'
      attribute = TrickHLAAttributeConfig( 'status',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the name of the PhysicalEntity's parent reference frame.
      trick_data_name = str(entity_instance_name) + '.parent_frame'
      attribute = TrickHLAAttributeConfig( 'parent_reference_frame',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's space/time coordinate state.
      trick_data_name = str(entity_instance_name) + '.stc_encoder.buffer'
      attribute = TrickHLAAttributeConfig( 'state',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_OPAQUE_DATA )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's translational acceleration.
      trick_data_name = str(entity_instance_name) + '.accel'
      attribute = TrickHLAAttributeConfig( 'acceleration',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's rotational acceleration.
      trick_data_name = str(entity_instance_name) + '.rot_accel'
      attribute = TrickHLAAttributeConfig( 'rotational_acceleration',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's center of mass.
      trick_data_name = str(entity_instance_name) + '.cm'
      attribute = TrickHLAAttributeConfig( 'center_of_mass',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's struct to body attitude quaternion.
      trick_data_name = str(entity_instance_name) + '.quat_encoder.buffer'
      attribute = TrickHLAAttributeConfig( 'body_wrt_structural',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_OPAQUE_DATA )
      self.add_attribute( attribute )

      return

