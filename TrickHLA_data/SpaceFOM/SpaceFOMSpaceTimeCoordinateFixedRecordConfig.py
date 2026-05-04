##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level SpaceFOM input
#     file interface for specifying a Space Time Coordinate fixed record.)
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
from ..SpaceFOM.SpaceFOMQuaternionFixedRecordConfig import *


class SpaceFOMSpaceTimeCoordinateFixedRecordConfig( TrickHLARecordElementConfig ):

   def __init__( self,
                 stc_rec_trick_name = None ):

      TrickHLARecordElementConfig.__init__(
                                 self,
                                 rti_encoding = trick.TrickHLA.ENCODING_FIXED_RECORD,
                                 trick_name   = stc_rec_trick_name )

      # field-Name: translational_state, dataType: ReferenceFrameTranslation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
      #    field-Name: position, dataType: PositionVectordata, dataType: (Length,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
      #    field-Name: velocity, dataType: VelocityVector, dataType: (Velocity,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
      # field-Name: rotational_state, dataType: ReferenceFrameRotation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
      #    field-Name: attitude_quaternion, DataType: AttitudeQuaternion, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
      #       field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
      #       field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
      #    field-Name: angular_velocity, dataType: AngularVelocityVector
      #       AngularVelocityVector: dataType:(AngularRate,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality:3
      # field-Name: time, dataType: HLAfloat64LE

      # translational_state: position
      trick_data_name = str( stc_rec_trick_name ) + '.pos'
      position_element = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                 trick_name   = trick_data_name )

      # translational_state: velocity
      trick_data_name = str( stc_rec_trick_name ) + '.vel'
      velocity_element = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                 trick_name   = trick_data_name )

      # translational_state, dataType: ReferenceFrameTranslation
      translational_state = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_FIXED_RECORD )
      translational_state.add_record_element( position_element )
      translational_state.add_record_element( velocity_element )

      # rotational_state: attitude_quaternion
      trick_data_name = str( stc_rec_trick_name ) + '.att'
      att_quat_element = SpaceFOMQuaternionFixedRecordConfig(
                                 quat_rec_trick_name = trick_data_name )

      # angular_velocity, dataType: AngularVelocityVector
      trick_data_name = str( stc_rec_trick_name ) + '.ang_vel'
      angular_vel_element = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                 trick_name   = trick_data_name )

      # rotational_state, dataType: ReferenceFrameRotation
      rotational_state = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_FIXED_RECORD )
      rotational_state.add_record_element( att_quat_element )
      rotational_state.add_record_element( angular_vel_element )

      # field-Name: time, dataType: HLAfloat64LE
      trick_data_name = str( stc_rec_trick_name ) + '.time'
      time_element = TrickHLARecordElementConfig(
                                 rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN,
                                 trick_name   = trick_data_name )

      # Space Time Coordinate (STC)
      self.add_record_element( translational_state )
      self.add_record_element( rotational_state )
      self.add_record_element( time_element )

      return
