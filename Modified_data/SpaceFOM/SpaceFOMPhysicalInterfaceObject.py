##############################################################################
# PURPOSE:
#    (This is a Python input file class to set up the general parameters that
#     describe an Space Reference FOM PhysicalInterface object.)
#
# REFERENCE:
#    (Trick documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAObjectConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA) (September 2023) (--) (Space FOM development)))
##############################################################################
import trick
from ..TrickHLA.TrickHLAObjectConfig import *
from ..TrickHLA.TrickHLAAttributeConfig import *

class SpaceFOMPhysicalInterfaceObject(TrickHLAObjectConfig):

   # The PhysicalInterface FOM name is fixed for the SpaceFOM.
   interface_FOM_name = 'PhysicalInterface'
   
   # Trick simulation object name (constructed).
   trick_interface_sim_obj_name = None

   def __init__( self,
                 create_interface_object,
                 interface_instance_name,
                 interface_S_define_instance,
                 interface_S_define_instance_name,
                 interface_lag_comp = None,
                 interface_ownership = None,
                 interface_deleted = None,
                 interface_thla_manager_object = None ):

      # Copy the PhysicalInterface federation execution instance name.
      interface_federation_instance_name = str( interface_instance_name )

      # Save the PhysicalInterface name to use for trick_data_name generation.
      self.trick_interface_sim_obj_name = str( interface_S_define_instance_name )
      
      # By SpaceFOM rule 6-1 the PhysicalInterface instance name must exactly
      # match the PhysicalInterface name in the data.
      if ( create_interface_object ) :
         interface_S_define_instance.set_name( interface_instance_name )
      else:
         interface_S_define_instance.set_name( '' )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     create_interface_object,
                                     interface_instance_name,
                                     self.interface_FOM_name,
                                     interface_lag_comp,
                                     interface_ownership,
                                     interface_deleted,
                                     interface_S_define_instance,
                                     interface_thla_manager_object )

      # Build the object attribute list.
      self.add_attributes()

      return


   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      TrickHLAObjectConfig.initialize( self, thla_object )

      return


   def add_attributes( self ):

      # Short cut the sim_object name for the PhysicalInterface data.
      interface_instance_name = self.trick_interface_sim_obj_name

      ## Set up the map to the reference PhysicalInterface's name.
      trick_data_name = str(interface_instance_name) + '.packing_data.name'
      attribute = TrickHLAAttributeConfig( 'name',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the name of the PhysicalInterface's parent reference frame.
      trick_data_name = str(interface_instance_name) + '.packing_data.parent_name'
      attribute = TrickHLAAttributeConfig( 'parent_name',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalInterface's translational acceleration.
      trick_data_name = str(interface_instance_name) + '.packing_data.position'
      attribute = TrickHLAAttributeConfig( 'position',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalInterface's struct to body attitude quaternion.
      trick_data_name = str(interface_instance_name) + '.quat_encoder.buffer'
      attribute = TrickHLAAttributeConfig( 'attitude',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_OPAQUE_DATA )
      self.add_attribute( attribute )

      return

