##############################################################################
# PURPOSE:
#    (This is a Python input file class to set up the general parameters that
#     describe an Space Reference FOM DynamicalEntity object.  This is built
#     off the SpaceFOMPhysicalEntityObject base class.)
#
# REFERENCE:
#    (Trick documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base SpaceFOMPhysicalEntityObject class)
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
                 entity_conditional         = None,
                 entity_lag_comp            = None,
                 entity_lag_comp_type       = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 entity_ownership           = None,
                 entity_deleted             = None,
                 entity_thla_manager_object = None ):


      # The DynamicalEntity FOM name is fixed for the SpaceFOM and
      # needs to be overridden before calling the DynamicalEntity constructor.
      self.entity_FOM_name = 'PhysicalEntity.DynamicalEntity'

      # Call the base class constructor.
      SpaceFOMPhysicalEntityObject.__init__( self,
                                             create_entity_object,
                                             entity_instance_name,
                                             entity_S_define_instance,
                                             entity_S_define_instance_name,
                                             entity_conditional,
                                             entity_lag_comp,
                                             entity_lag_comp_type,
                                             entity_ownership,
                                             entity_deleted,
                                             entity_thla_manager_object )

      #######################################################################
      # NOTE: We do not need to call add_attributes() here.  This would
      # duplicate the attributes if we do.  The call to add_attributes() in
      # the SpaceFOMPhysicalEntityObject.__init__() will bind to the
      # SpaceFOMDynamicalEntityObject.add_attributes() call below and insure
      # that both are updated.
      # The power and beauty of object oriented programming. ;-)
      #######################################################################
      # Build the object attribute list.
      # self.add_attributes()

      return


   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      SpaceFOMPhysicalEntityObject.initialize( self, thla_object )

      return


   def add_attributes( self ):
      
      # Add the PhysicalEntity attributes.
      SpaceFOMPhysicalEntityObject.add_attributes(self)

      # Short cut the sim_object name for the DynamicalEntity data.
      entity_instance_name = self.trick_entity_sim_obj_name

      ## Set up the map to the DynamicalEntity's force.
      trick_data_name = str(entity_instance_name) + '.de_packing_data.force'
      attribute = TrickHLAAttributeConfig( 'force',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the DynamicalEntity's torque.
      trick_data_name = str(entity_instance_name) + '.de_packing_data.torque'
      attribute = TrickHLAAttributeConfig( 'torque',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the DynamicalEntity's mass.
      trick_data_name = str(entity_instance_name) + '.de_packing_data.mass'
      attribute = TrickHLAAttributeConfig( 'mass',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the DynamicalEntity's mass rate.
      trick_data_name = str(entity_instance_name) + '.de_packing_data.mass_rate'
      attribute = TrickHLAAttributeConfig( 'mass_rate',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the DynamicalEntity's inertia tensor.
      trick_data_name = str(entity_instance_name) + '.de_packing_data.inertia'
      attribute = TrickHLAAttributeConfig( 'inertia',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the DynamicalEntity's inertia rate.
      trick_data_name = str(entity_instance_name) + '.de_packing_data.inertia_rate'
      attribute = TrickHLAAttributeConfig( 'inertia_rate',
                                           trick_data_name,
                                           self.hla_create,
                                           not self.hla_create,
                                           self.hla_create,
                                           trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      return

