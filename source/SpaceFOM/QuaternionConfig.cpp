/**
@file SpaceFOM/QuaternionConfig.cpp
@ingroup SpaceFOM
@brief The SpaceFOM Quaternion configuration helper class.

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

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/RecordElement.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM includes.
#include "SpaceFOM/QuaternionConfig.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @brief Configure the TrickHLA Attribute and the fixed record elements.
 */
void QuaternionConfig::configure(
   Attribute           *attribute,
   std::string const   &fom_name,
   std::string const   &trick_root_name,
   DataUpdateEnum const config,
   bool const           publish,
   bool const           subscribe,
   bool const           locally_owned )
{
   if ( attribute == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::QuaternionConfig::configure():" << __LINE__
             << " ERROR: Unexpected NULL attribute specified!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Build up the Attribute config for the Quaternion fixed record.
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
void QuaternionConfig::configure(
   RecordElement *rec_element,
   string const  &trick_root_name )
{
   if ( rec_element == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::QuaternionConfig::configure():" << __LINE__
             << " ERROR: Unexpected NULL rec_element specified!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // field-Name: attitude_quaternion, DataType: AttitudeQuaternion, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //    field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
   //    field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   rec_element->rti_encoding  = TrickHLA::ENCODING_FIXED_RECORD;
   rec_element->element_count = 2;
   rec_element->elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", rec_element->element_count ) );

   rec_element->elements[0].trick_name   = trick_root_name + string( ".scalar" );
   rec_element->elements[0].rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;
   rec_element->elements[1].trick_name   = trick_root_name + string( ".vector" );
   rec_element->elements[1].rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;
}
