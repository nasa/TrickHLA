/**
@file SpaceFOM/SpaceTimeCoordinateConfig.cpp
@ingroup SpaceFOM
@brief The SpaceFOM Space Time Coordinate configuration helper class.

@copyright Copyright 2026 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{SpaceTimeCoordinateConfig.cpp}
@trick_link_dependency{QuaternionConfig.cpp}
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/RecordElement.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2026, --, Initial implementation.}
@revs_end
*/

// System includes.
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/memorymanager_c_intf.h"

// SpaceFOM includes.
#include "SpaceFOM/QuaternionConfig.hh"
#include "SpaceFOM/SpaceTimeCoordinateConfig.hh"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/RecordElement.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @brief Configure the TrickHLA Attribute and the fixed record elements.
 */
void SpaceTimeCoordinateConfig::configure(
   Attribute           *attribute,
   string const        &fom_name,
   string const        &trick_root_name,
   DataUpdateEnum const config,
   bool const           publish,
   bool const           subscribe,
   bool const           locally_owned )
{
   if ( attribute == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::SpaceTimeCoordinateConfig::configure():" << __LINE__
             << " ERROR: Unexpected NULL attribute specified!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Build up the Attribute config for the SpaceTimeCoordinate fixed record.
   attribute->FOM_name      = fom_name;
   attribute->config        = config;
   attribute->publish       = publish;
   attribute->subscribe     = subscribe;
   attribute->locally_owned = locally_owned;

   configure( attribute, trick_root_name );
}

/*!
 * @brief Configure the TrickHLA RecordElement and the fixed record elements.
 */
void SpaceTimeCoordinateConfig::configure(
   RecordElement *rec_element,
   string const  &trick_root_name )
{
   if ( rec_element == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::SpaceTimeCoordinateConfig::configure():" << __LINE__
             << " ERROR: Unexpected NULL rec_element specified!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // field-Name: translational_state, dataType: ReferenceFrameTranslation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //    field-Name: position, dataType: PositionVectordata, dataType: (Length,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //    field-Name: velocity, dataType: VelocityVector, dataType: (Velocity,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   // field-Name: rotational_state, dataType: ReferenceFrameRotation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //    field-Name: attitude_quaternion, DataType: AttitudeQuaternion, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //       field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
   //       field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //    field-Name: angular_velocity, dataType: AngularVelocityVector
   //       AngularVelocityVector: dataType:(AngularRate,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality:3
   // field-Name: time, dataType: HLAfloat64LE
   rec_element->rti_encoding  = TrickHLA::ENCODING_FIXED_RECORD;
   rec_element->element_count = 3;
   rec_element->elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", rec_element->element_count ) );

   // field-Name: translational_state, dataType: ReferenceFrameTranslation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //    field-Name: position, dataType: PositionVectordata, dataType: (Length,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //    field-Name: velocity, dataType: VelocityVector, dataType: (Velocity,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   rec_element->elements[0].rti_encoding  = TrickHLA::ENCODING_FIXED_RECORD;
   rec_element->elements[0].element_count = 2;
   rec_element->elements[0].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", rec_element->elements[0].element_count ) );

   // ReferenceFrameTranslation
   rec_element->elements[0].elements[0].trick_name   = trick_root_name + string( ".pos" );
   rec_element->elements[0].elements[0].rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;
   rec_element->elements[0].elements[1].trick_name   = trick_root_name + string( ".vel" );
   rec_element->elements[0].elements[1].rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   // field-Name: rotational_state, dataType: ReferenceFrameRotation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //    field-Name: attitude_quaternion, DataType: AttitudeQuaternion, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //       field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
   //       field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //    field-Name: angular_velocity, dataType: AngularVelocityVector
   //       AngularVelocityVector: dataType:(AngularRate,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality:3
   rec_element->elements[1].rti_encoding  = TrickHLA::ENCODING_FIXED_RECORD;
   rec_element->elements[1].element_count = 2;
   rec_element->elements[1].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", rec_element->elements[1].element_count ) );

   // Quaternion - Attitude
   string const att_trick_root_name = trick_root_name + string( ".att" );
   QuaternionConfig::configure( &rec_element->elements[1].elements[0], att_trick_root_name );

   // AngularVelocityVector
   rec_element->elements[1].elements[1].trick_name   = trick_root_name + string( ".ang_vel" );
   rec_element->elements[1].elements[1].rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   // field-Name: time, dataType: HLAfloat64LE
   rec_element->elements[2].trick_name   = trick_root_name + string( ".time" );
   rec_element->elements[2].rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;
}
