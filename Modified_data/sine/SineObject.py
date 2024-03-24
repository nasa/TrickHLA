##############################################################################
# PURPOSE:
#    (This is a Python input file class to set up the general parameters that
#     describe the sine data to configure a TrickHLA Object.)
#
# REFERENCE:
#    (Trick documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAObjectConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA) (July 2023) (--) (Space FOM development))
#     ((Dan Dexter) (NASA) (Mar 2024) (--) (Sine SpaceFOM example)))
##############################################################################
import trick
from ..TrickHLA.TrickHLAObjectConfig import *
from ..TrickHLA.TrickHLAAttributeConfig import *

class SineObject(TrickHLAObjectConfig):

   # The Sine HLA class FOM name.
   sine_FOM_name = 'Sine'
   
   # Trick simulation object name (constructed).
   trick_sine_sim_obj_name = None

   def __init__( self,
                 sine_create_object,
                 sine_instance_name,
                 sine_S_define_instance,
                 sine_S_define_instance_name,
                 sine_conditional         = None,
                 sine_lag_comp            = None,
                 sine_lag_comp_type       = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 sine_ownership           = None,
                 sine_deleted             = None,
                 sine_thla_object         = None,
                 sine_thread_IDs          = None ):

      # Copy the federation execution instance name.
      sine_federation_instance_name = str( sine_instance_name )

      # Save the Sine name to use for trick_data_name generation.
      self.trick_sine_sim_obj_name = str( sine_S_define_instance_name )

      if ( sine_create_object ) :
         sine_S_define_instance.set_name( sine_instance_name )
      else:
         sine_S_define_instance.set_name( '' )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     thla_create               = sine_create_object,
                                     thla_instance_name        = sine_instance_name,
                                     thla_FOM_name             = self.sine_FOM_name,
                                     thla_lag_comp_instance    = sine_lag_comp,
                                     thla_lag_comp_type        = sine_lag_comp_type,
                                     thla_ownership_instance   = sine_ownership,
                                     thla_deleted_instance     = sine_deleted,
                                     thla_conditional_instance = sine_conditional,
                                     thla_packing_instance     = sine_S_define_instance,
                                     thla_object               = sine_thla_object,
                                     thla_thread_IDs           = sine_thread_IDs )

      # Build the object attribute list.
      self.add_attributes()

      return


   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      TrickHLAObjectConfig.initialize( self, thla_object )

      return


   def add_attributes( self ):

      # Short cut the sim_object name for the Sine data.
      sine_instance_name = self.trick_sine_sim_obj_name

      ## Set up the map to the reference Sine Name.
      trick_data_name = str(sine_instance_name) + '.packing.name'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'Name',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( attribute )

      ## Set up the map to the Sine Time.
      trick_data_name = str(sine_instance_name) + '.packing.time'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'Time',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the reference Sine Value.
      trick_data_name = str(sine_instance_name) + '.packing.value'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'Value',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the name of the Sine dvdt.
      trick_data_name = str(sine_instance_name) + '.packing.dvdt'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'dvdt',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the name of the Sine Phase.
      trick_data_name = str(sine_instance_name) + '.packing.phase_deg'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'Phase',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the name of the Sine Frequency.
      trick_data_name = str(sine_instance_name) + '.packing.freq'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'Frequency',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the name of the Sine Amplitude.
      trick_data_name = str(sine_instance_name) + '.packing.amp'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'Amplitude',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      ## Set up the map to the name of the Sine Tolerance.
      trick_data_name = str(sine_instance_name) + '.packing.tol'
      attribute = TrickHLAAttributeConfig( FOM_name      = 'Tolerance',
                                           trick_name    = trick_data_name,
                                           publish       = self.hla_create,
                                           subscribe     = not self.hla_create,
                                           locally_owned = self.hla_create,
                                           config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                           rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( attribute )

      return

