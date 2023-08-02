##############################################################################
# PURPOSE:
#    (This is a python input file class to set up the general parameters that
#     describe an Space Reference FOM PhysicalEntity object.)
#
# REFERENCE:
#    (Trick 16 documentation.)
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
from ..SpaceFOM.SpaceFOMPhysicalEntityObject import *

class SpaceFOMDynamicalEntityObject(SpaceFOMPhysicalEntityObject):

   def __init__( self,
                 create_entity_object,
                 entity_instance_name,
                 entity_S_define_instance,
                 entity_S_define_instance_name,
                 entity_lag_comp = None,
                 entity_ownership = None,
                 entity_thla_manager_object = None ):


      # The PhysicalEntity FOM name is fixed for the SpaceFOM.
      self.entity_FOM_name = 'PhysicalEntity.DynamicalEntity'

      # Call the base class constructor.
      SpaceFOMPhysicalEntityObject.__init__( self,
                                             create_entity_object,
                                             entity_instance_name,
                                             entity_S_define_instance,
                                             entity_S_define_instance_name,
                                             entity_lag_comp,
                                             entity_ownership,
                                             entity_thla_manager_object )

      # Build the object attribute list.
      self.add_attributes()

      return


   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      SpaceFOMPhysicalEntityObject.initialize( self, thla_object )

      return


   def add_attributes( self ):
      
      # Add the PhysicalEntity attributes.
      SpaceFOMPhysicalEntityObject.add_attributes(self)

      # Short cut the sim_object name for the PhysicalEntity data.
      entity_instance_name = self.trick_entity_sim_obj_name

      ## Set up the map to the PhysicalEntity's force.
      trick_data_name = str(entity_instance_name) + '.force'
      attribute = TrickHLAAttributeConfig( 'force',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's torque.
      trick_data_name = str(entity_instance_name) + '.torque'
      attribute = TrickHLAAttributeConfig( 'torque',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's mass.
      trick_data_name = str(entity_instance_name) + '.mass'
      attribute = TrickHLAAttributeConfig( 'mass',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's mass rate.
      trick_data_name = str(entity_instance_name) + '.mass_rate'
      attribute = TrickHLAAttributeConfig( 'mass_rate',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's inertia tensor.
      trick_data_name = str(entity_instance_name) + '.inertia'
      attribute = TrickHLAAttributeConfig( 'inertia',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the PhysicalEntity's inertia rate.
      trick_data_name = str(entity_instance_name) + '.inertia_rate'
      attribute = TrickHLAAttributeConfig( 'inertia_rate',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      return

