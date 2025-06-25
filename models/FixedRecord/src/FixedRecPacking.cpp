/*!
@ingroup Sine
@file models/FixedRecord/src/FixedRecPacking.cpp
@brief This class provides data packing for the FixedRecData data.

@copyright Copyright 2025 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{FixedRecord/src/FixedRecData.cpp}
@trick_link_dependency{FixedRecord/src/FixedRecPacking.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation}
@revs_end

*/

// System includes.
#include <sstream>
#include <string>

// Trick includes.
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"

// FixedRecord model includes.
#include "FixedRecord/include/FixedRecPacking.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
FixedRecPacking::FixedRecPacking()
   : FixedRecData(),
     TrickHLA::Packing(),
     sim_data( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
FixedRecPacking::~FixedRecPacking()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void FixedRecPacking::configure(
   FixedRecData *sim_data )
{
   this->sim_data = sim_data;
}

void FixedRecPacking::pack()
{
   if ( !initialized ) {
      ostringstream msg;
      msg << "FixedRecPacking::pack():" << __LINE__
          << " ERROR: The initialize() function has not been called!\n";
      message_publish( MSG_WARNING, msg.str().c_str() );
   }

   // MainFixedRecObject
   field_2_float64 += 1.0;
   string str = "field_1_string-" + std::to_string( field_2_float64 );
   field_1_string = StringUtilities::mm_strdup_string( str );

   // MainFixedRecord
   elem_2_float64 += 1000.0;
   str = "elem_1_string-" + std::to_string( elem_2_float64 );
   elem_1_string = StringUtilities::mm_strdup_string( str );

   // SecondaryFixedRecord
   element_1_count += 10;
   str = "element_2_name-" + std::to_string( element_1_count );
   element_2_name = StringUtilities::mm_strdup_string( str );

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_PACKING ) ) {
      string        obj_name = ( this->object != NULL ) ? object->get_name() : "";
      ostringstream msg;
      msg << "FixedRecPacking::pack():" << __LINE__ << std::endl
          << "Object-Name:'" << obj_name << "'" << std::endl
          << "\t sim_data:" << sim_data->to_string()
          << "\t ----------" << std::endl
          << "\t pack:" << to_string();
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

void FixedRecPacking::unpack()
{
   if ( !initialized ) {
      ostringstream msg;
      msg << "FixedRecPacking::unpack():" << __LINE__
          << " ERROR: The initialize() function has not been called!\n";
      message_publish( MSG_WARNING, msg.str().c_str() );
   }

   // MainFixedRecObject
   if ( field_1_string != NULL ) {
      sim_data->field_1_string  = StringUtilities::mm_strdup_string( field_1_string );
   }
   sim_data->field_2_float64 = field_2_float64;

   // MainFixedRecord
   if ( elem_1_string != NULL ) {
      sim_data->elem_1_string  = StringUtilities::mm_strdup_string( elem_1_string );
   }
   sim_data->elem_2_float64 = elem_2_float64;

   // SecondaryFixedRecord
   if ( element_2_name != NULL ) {
      sim_data->element_2_name  = StringUtilities::mm_strdup_string( element_2_name );
   }
   sim_data->element_1_count = element_1_count;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_PACKING ) ) {

      string obj_name = ( this->object != NULL ) ? object->get_name() : "";

      ostringstream msg;
      msg << "FixedRecPacking::unpack():" << __LINE__ << '\n'
          << "Object-Name:'" << obj_name << "'\n"
          << "\t sim_data:" << sim_data->to_string()
          << "\t ----------" << std::endl
          << "\t unpack:" << to_string();

      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
