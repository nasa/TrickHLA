##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level SpaceFOM input
#     file interface for specifying a Quaternion fixed record.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA/ER6) (May 2026) (--) (Initial implementation.)))
##############################################################################
import trick
from ..TrickHLA.TrickHLARecordElementConfig import *


class SpaceFOMQuaternionFixedRecordConfig( TrickHLARecordElementConfig ):

   def __init__( self,
                 quat_rec_trick_name ):

      TrickHLARecordElementConfig.__init__(
                                    self,
                                    rti_encoding = trick.TrickHLA.ENCODING_FIXED_RECORD,
                                    trick_name   = quat_rec_trick_name )

      # field-Name: attitude_quaternion, DataType: AttitudeQuaternion, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
      #    field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
      #    field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
      trick_data_name = str( quat_rec_trick_name ) + '.scalar'
      quat_scalar_element = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                 trick_name   = trick_data_name )

      trick_data_name = str( quat_rec_trick_name ) + '.vector'
      quat_vector_element = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                 trick_name   = trick_data_name )

      self.add_record_element( quat_scalar_element )
      self.add_record_element( quat_vector_element )

      return
