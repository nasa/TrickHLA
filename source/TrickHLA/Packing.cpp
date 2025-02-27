/*!
@file TrickHLA/Packing.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for Trick HLA Packing class.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{Packing.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial version.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Sept 2009, --, Updated Packing API.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Oct 2009, --, Added get attribute function.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <limits>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Packing::Packing()
   : initialized( false ),
     object( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Packing::~Packing()
{
   return;
}

/*!
 * @brief Initialize the callback object to the supplied Object pointer.
 * @param obj Associated object for this class.
 */
void Packing::initialize_callback(
   Object *obj )
{
   this->object = obj;
}

/*!
 * @job_class{default_data}
 */
void Packing::set_object( TrickHLA::Object *mngr_obj )
{
   // Check for initialization.
   if ( initialized ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::Packing::set_object():" << __LINE__
             << " ERROR: The initialize() function has already been called\n";
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Assign the object.
   this->object = mngr_obj;

   return;
}

/*!
 * @brief Get the Attribute by FOM name.
 * @return Attribute for the given name.
 * @param attr_FOM_name Attribute FOM name.
 */
Attribute *Packing::get_attribute(
   char const *attr_FOM_name )
{
   return object->get_attribute( attr_FOM_name );
}

/*!
 * @brief This function returns the Attribute for the given attribute FOM name.
 * @return Attribute for the given name.
 * @param attr_FOM_name Attribute FOM name.
 */
Attribute *Packing::get_attribute_and_validate(
   char const *attr_FOM_name )
{
   // Make sure the FOM name is not NULL.
   if ( attr_FOM_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Packing::get_attribute_and_validate():" << __LINE__
             << " ERROR: Unexpected NULL attribute FOM name specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the attribute by FOM name.
   Attribute *attr = get_attribute( attr_FOM_name );

   // Make sure we have found the attribute.
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "Packing::get_attribute_and_validate():" << __LINE__
             << " ERROR: For FOM object '" << object->get_FOM_name()
             << "', failed to find the TrickHLA Attribute for an attribute named"
             << " '" << attr_FOM_name << "'. Make sure the FOM attribute name is"
             << " correct, the FOM contains an attribute named '"
             << attr_FOM_name << "' and that your input.py file is properly"
             << " configured for this attribute.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return attr;
}

/*!
 * @brief Get the current scenario time.
 * @return Returns the current scenario time.
 */
double Packing::get_scenario_time()
{
   if ( ( object != NULL ) && ( object->get_federate() != NULL ) ) {
      ExecutionControlBase *execution_control = object->get_federate()->get_execution_control();
      return ( execution_control->get_scenario_time() );
   }
   return -std::numeric_limits< double >::max();
}

/*!
 * @brief Get the current Central Timing Equipment (CTE) time.
 * @return Returns the current CTE time.
 */
double Packing::get_cte_time()
{
   if ( ( object != NULL ) && ( object->get_federate() != NULL ) ) {
      ExecutionControlBase *execution_control = object->get_federate()->get_execution_control();
      if ( execution_control->does_cte_timeline_exist() ) {
         return execution_control->get_cte_time();
      }
   }
   return -std::numeric_limits< double >::max();
}

/*!
 * @job_class{default_data}
 */
char *Packing::allocate_input_string( // RETURN: -- None.
   char const *c_string )             // IN: -- String to allocate.
{
   return allocate_input_string( string( c_string ) );
}

/*!
 * @job_class{default_data}
 */
char *Packing::allocate_input_string( // RETURN: -- None.
   string const &cpp_string )         // IN: -- String to allocate.
{
   char *new_c_str = static_cast< char * >( TMM_declare_var_1d( "char", cpp_string.length() + 1 ) );
   strncpy( new_c_str, cpp_string.c_str(), cpp_string.length() + 1 );

   return new_c_str;
}
