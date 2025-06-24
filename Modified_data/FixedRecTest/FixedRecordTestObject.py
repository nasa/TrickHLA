#     - elem_1_string:  HLAunicodeString##############################################################################
# PURPOSE:
#    (This is a Python input file class to set up the general parameters that
#     describe the Fixed Record Test data to configure a TrickHLA Object.)
#
# REFERENCE:
#    (Trick documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAObjectConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA) (Jue 2025) (--) (SpaceFOM sine fixed record example)))
##############################################################################
import trick
from ..TrickHLA.TrickHLAObjectConfig import *
from ..TrickHLA.TrickHLAAttributeConfig import *
from ..TrickHLA.TrickHLARecordElementConfig import *

class FixedRecordTestObject( TrickHLAObjectConfig ):

   # The fixed-record test HLA class FOM name.
   fixed_rec_FOM_name = str( 'MainFixedRecObject' )

   # Trick simulation object name (constructed).
   trick_sim_obj_name = None

   def __init__( self,
                 fixed_rec_create_object,
                 fixed_rec_obj_instance_name,
                 fixed_rec_trick_sim_obj_name,
                 fixed_rec_packing             = None,
                 fixed_rec_conditional         = None,
                 fixed_rec_lag_comp            = None,
                 fixed_rec_lag_comp_type       = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 fixed_rec_ownership           = None,
                 fixed_rec_deleted             = None,
                 fixed_rec_thla_object         = None,
                 fixed_rec_thread_IDs          = None,
                 fixed_rec_attribute_publish   = None,
                 fixed_rec_attribute_subscribe = None,
                 fixed_rec_attribute_config    = None ):

      # Save the Trick sim-object name to use for trick_data_name generation.
      self.trick_sim_obj_name = str( fixed_rec_trick_sim_obj_name )

      # Call the base class constructor.
      TrickHLAObjectConfig.__init__( self,
                                     thla_create               = fixed_rec_create_object,
                                     thla_instance_name        = fixed_rec_obj_instance_name,
                                     thla_FOM_name             = self.fixed_rec_FOM_name,
                                     thla_lag_comp_instance    = fixed_rec_lag_comp,
                                     thla_lag_comp_type        = fixed_rec_lag_comp_type,
                                     thla_ownership_instance   = fixed_rec_ownership,
                                     thla_deleted_instance     = fixed_rec_deleted,
                                     thla_conditional_instance = fixed_rec_conditional,
                                     thla_packing_instance     = fixed_rec_packing,
                                     thla_object               = fixed_rec_thla_object,
                                     thla_thread_IDs           = fixed_rec_thread_IDs,
                                     thla_attribute_publish    = fixed_rec_attribute_publish,
                                     thla_attribute_subscribe  = fixed_rec_attribute_subscribe,
                                     thla_attribute_config     = fixed_rec_attribute_config )

      # Build the object attribute list.
      self.add_attributes()

      return


   def initialize( self, thla_object ):

      # Call the base class initialization utility function.
      TrickHLAObjectConfig.initialize( self, thla_object )

      return


   def add_attributes( self ):

      # Short cut the sim_object name for the Sine data.
      fixed_rec_trick_sim_obj_name = self.trick_sim_obj_name

      # Set a default TrickHLA Attribute config if one is not set.
      if ( self.hla_attribute_config == None ):
         self.hla_attribute_config = trick.TrickHLA.CONFIG_CYCLIC

      # FixedRecordTest.xml:
      # - field_1_string:  HLAunicodeString
      # - field_2_float64: HLAfloat64LE
      # - field_3_rec:     MainFixedRecord
      #   + MainFixedRecord:  HLAfixedRecord
      #     - elem_1_string:  HLAunicodeString
      #     - elem_2_float64: HLAfloat64LE
      #     - elem_3_record:  SecondaryFixedRecord
      #       + SecondaryFixedRecord: HLAfixedRecord
      #         - element_1_count: HLAinteger32LE
      #         - element_2_name:  HLAunicodeString

      # - field_1_string:  HLAunicodeString
      trick_data_name = str( fixed_rec_trick_sim_obj_name ) + '.packing.field_1_string'
      field_1_string = TrickHLAAttributeConfig( FOM_name      = 'field_1_string',
                                                trick_name    = trick_data_name,
                                                publish       = self.hla_attribute_publish,
                                                subscribe     = self.hla_attribute_subscribe,
                                                locally_owned = self.hla_create,
                                                config        = self.hla_attribute_config,
                                                rti_encoding  = trick.TrickHLA.ENCODING_UNICODE_STRING )
      self.add_attribute( field_1_string )

      # - field_2_float64: HLAfloat64LE
      trick_data_name = str( fixed_rec_trick_sim_obj_name ) + '.packing.field_2_float64'
      field_2_float64 = TrickHLAAttributeConfig( FOM_name      = 'field_2_float64',
                                                 trick_name    = trick_data_name,
                                                 publish       = self.hla_attribute_publish,
                                                 subscribe     = self.hla_attribute_subscribe,
                                                 locally_owned = self.hla_create,
                                                 config        = self.hla_attribute_config,
                                                 rti_encoding  = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_attribute( field_2_float64 )

      # - field_3_rec:     MainFixedRecord
      #   + MainFixedRecord:  HLAfixedRecord
      #     - elem_1_string:  HLAunicodeString
      #     - elem_2_float64: HLAfloat64LE
      #     - elem_3_record:  SecondaryFixedRecord
      field_3_rec = TrickHLAAttributeConfig( FOM_name      = 'field_3_rec',
                                             trick_name    = None,
                                             publish       = self.hla_attribute_publish,
                                             subscribe     = self.hla_attribute_subscribe,
                                             locally_owned = self.hla_create,
                                             config        = self.hla_attribute_config,
                                             rti_encoding  = trick.TrickHLA.ENCODING_FIXED_RECORD )
      self.add_attribute( field_3_rec )

      #   + MainFixedRecord:  HLAfixedRecord
      #     - elem_1_string:  HLAunicodeString
      trick_data_name = str( fixed_rec_trick_sim_obj_name ) + '.packing.elem_1_string'
      elem_1_string = TrickHLARecordElementConfig( rti_encoding = trick.TrickHLA.ENCODING_UNICODE_STRING,
                                                   trick_name   = trick_data_name )
      field_3_rec.add_record_element( elem_1_string )

      #   + MainFixedRecord:  HLAfixedRecord
      #     - elem_2_float64: HLAfloat64LE
      trick_data_name = str( fixed_rec_trick_sim_obj_name ) + '.packing.elem_2_float64'
      elem_2_float64 = TrickHLARecordElementConfig( rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                                    trick_name   = trick_data_name )
      field_3_rec.add_record_element( elem_2_float64 )

      #   + MainFixedRecord:  HLAfixedRecord
      #     - elem_3_record:  SecondaryFixedRecord
      #       + SecondaryFixedRecord: HLAfixedRecord
      #         - element_1_count: HLAinteger32LE
      #         - element_2_name:  HLAunicodeString
      elem_3_record = TrickHLARecordElementConfig( rti_encoding = trick.TrickHLA.ENCODING_FIXED_RECORD )
      field_3_rec.add_record_element( elem_3_record )

      #       + SecondaryFixedRecord: HLAfixedRecord
      #         - element_1_count: HLAinteger32LE
      trick_data_name = str( fixed_rec_trick_sim_obj_name ) + '.packing.element_1_count'
      element_1_count = TrickHLARecordElementConfig( rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                                     trick_name   = trick_data_name )
      elem_3_record.add_record_element( element_1_count )

      #       + SecondaryFixedRecord: HLAfixedRecord
      #         - element_2_name:  HLAunicodeString
      trick_data_name = str( fixed_rec_trick_sim_obj_name ) + '.packing.element_2_name'
      element_2_name = TrickHLARecordElementConfig( rti_encoding = trick.TrickHLA.ENCODING_UNICODE_STRING,
                                                    trick_name   = trick_data_name )
      elem_3_record.add_record_element( element_2_name )

      return
