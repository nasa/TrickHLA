##############################################################################
# PURPOSE:
#    (This is a python input file class to set up the general parameters that
#     describe an Space Reference FOM reference frame object.)
#
# REFERENCE:
#    (Trick 16 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base HLAObjectConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA) (June 2016) (--) (Space FOM development)))
##############################################################################
import trick
from ..TrickHLA.TrickHLAObjectConfig import *
from ..TrickHLA.TrickHLAAttributeConfig import *

class SpaceFOMRefFrameObject( TrickHLAObjectConfig ):

   trick_frame_sim_obj_name = None

   def __init__( self,
                 create_frame_object,
                 frame_instance_name,
                 frame_S_define_instance,
                 frame_S_define_instance_name,
                 parent_S_define_instance  = None,
                 parent_name               = None,
                 frame_conditional         = None,
                 frame_lag_comp            = None,
                 frame_lag_comp_type       = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 frame_ownership           = None,
                 frame_deleted             = None,
                 frame_thla_manager_object = None,
                 frame_thread_IDs          = None ):

      # The Reference Frame FOM name is fixed for the SpaceFOM.
      frame_FOM_name = 'ReferenceFrame'
      
      # Copy the frame federation execution instance name.
      frame_federation_instance_name = str( frame_instance_name )

      # Save the frame name to use for trick_data_name generation.
      self.trick_frame_sim_obj_name = str( frame_S_define_instance_name )
      
      # By SpaceFOM rule 5-1 the Reference Frame instance name must exactly
      # match the Reference Frame name in the data.
      if ( create_frame_object ):
         frame_S_define_instance.set_name( frame_instance_name )
      else:
         frame_S_define_instance.set_name( '' )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     thla_create               = create_frame_object,
                                     thla_instance_name        = frame_instance_name,
                                     thla_FOM_name             = frame_FOM_name,
                                     thla_lag_comp_instance    = frame_lag_comp,
                                     thla_lag_comp_type        = frame_lag_comp_type,
                                     thla_ownership_instance   = frame_ownership,
                                     thla_deleted_instance     = frame_deleted,
                                     thla_conditional_instance = frame_conditional,
                                     thla_packing_instance     = frame_S_define_instance,
                                     thla_object               = frame_thla_manager_object,
                                     thla_thread_IDs           = frame_thread_IDs )

      # Set the frame instance name.
      frame_S_define_instance.set_name( frame_instance_name )

      # Set the frame parent instance name.
      if ( parent_name ):
         frame_S_define_instance.set_parent_name( parent_name )
      else:
         frame_S_define_instance.set_parent_name( '' )

      # Set the frame parent instance name.
      if ( parent_S_define_instance != None ):
         frame_S_define_instance.set_parent_frame( parent_S_define_instance )
      
      # Build the object attribute list.
      self.add_attributes()

      return


   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      TrickHLAObjectConfig.initialize( self, thla_object )

      return


   def add_attributes( self ):

      # Short cut the sim_object name for the frame data.
      frame_instance_name = self.trick_frame_sim_obj_name

      ## Set up the map to the reference frame's name.
      trick_data_name = str( frame_instance_name ) + '.packing_data.name'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'name',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the name of the reference frame's parent frame.
      trick_data_name = str( frame_instance_name ) + '.packing_data.parent_name'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'parent_name',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the reference frame's space/time coordinate state.
      trick_data_name = str( frame_instance_name ) + '.stc_encoder.buffer'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'state',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_attribute_publish,
                                           subscribe     = self.hla_attribute_subscribe,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_NONE )
      self.add_attribute( attribute )

      return

