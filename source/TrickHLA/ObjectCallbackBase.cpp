/*!
@file TrickHLA/ObjectCallbackBase.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for object callback's.

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
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{ObjectCallbackBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, September 2025, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectCallbackBase.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ObjectCallbackBase::ObjectCallbackBase()
   : initialized( false ),
     object( NULL ),
     callback_name(),
     exec_control( NULL )
{
   return;
}

ObjectCallbackBase::ObjectCallbackBase(
   string name )
   : initialized( false ),
     object( NULL ),
     callback_name( name ),
     exec_control( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ObjectCallbackBase::~ObjectCallbackBase()
{
   return;
}

/*!
 * @job_class{default_data}
 */
void ObjectCallbackBase::initialize()
{
   if ( ( this->object != NULL ) && this->object->name.empty() ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ObjectCallbackBase::initialize():" << __LINE__
             << " ERROR: No Object name found, and it needs to be set"
             << " before calling the initialize() function!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   this->initialized = true;
}

/*!
 * @brief Initialize the callback object to the supplied Object pointer.
 * @param obj Associated object for this class.
 */
void ObjectCallbackBase::initialize_callback(
   Object *obj )
{
   this->object = obj;
   update_exec_control_ptr();
}

/*!
 * @job_class{default_data}
 */
void ObjectCallbackBase::set_object(
   Object *obj )
{
   // Check for initialization.
   if ( initialized ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ObjectCallbackBase::set_object():" << __LINE__
             << " ERROR: The initialize() function has already been called!"
             << endl;
      // Print message and terminate.
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Assign the object.
   this->object = obj;
   update_exec_control_ptr();
}

/*!
 * @brief Get the Attribute by FOM name.
 * @return Attribute for the given name.
 * @param attr_FOM_name Attribute FOM name.
 */
Attribute *ObjectCallbackBase::get_attribute(
   string const &attr_FOM_name )
{
   return ( object != NULL ) ? object->get_attribute( attr_FOM_name ) : NULL;
}

/*!
 * @brief This function returns the Attribute for the given attribute FOM name.
 * @return Attribute for the given name.
 * @param attr_FOM_name Attribute FOM name.
 */
Attribute *ObjectCallbackBase::get_attribute_and_validate(
   string const &attr_FOM_name )
{
   // Make sure the FOM name is not empty.
   if ( attr_FOM_name.empty() ) {
      ostringstream errmsg;
      errmsg << "ObjectCallbackBase::get_attribute_and_validate():" << __LINE__
             << " ERROR: No attribute FOM name specified." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the attribute by FOM name.
   Attribute *attr = get_attribute( attr_FOM_name );

   // Make sure we have found the attribute.
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectCallbackBase::get_attribute_and_validate():" << __LINE__
             << " ERROR: For FOM object '"
             << ( ( object != NULL ) ? object->get_FOM_name() : "NULL" )
             << "', failed to find the TrickHLA Attribute for an attribute named"
             << " '" << attr_FOM_name << "'. Make sure the FOM attribute name is"
             << " correct, the FOM contains an attribute named '"
             << attr_FOM_name << "' and that your input.py file is properly"
             << " configured for this attribute." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return attr;
}

/*! @brief Returns a copy of the object's lookahead time.
 *  @return A copy of the federate's lookahead time.
 */
Int64Interval ObjectCallbackBase::get_lookahead() const
{
   return ( object != NULL ) ? object->get_lookahead() : Int64Interval( -1.0 );
}

/*! @brief Returns a copy of the object's granted federation time.
 *  @return A copy of the federate's current granted time.
 */
Int64Time ObjectCallbackBase::get_granted_time() const
{
   return ( object != NULL ) ? object->get_granted_time()
                             : Int64Time( INT64_MAX );
}

/*!
 * @brief Get the current scenario time.
 * @return Returns the current scenario time.
 */
double ObjectCallbackBase::get_scenario_time() const
{
   return ( exec_control != NULL ) ? exec_control->get_scenario_time()
                                   : -std::numeric_limits< double >::max();
}

/*!
 * @brief Get the current Central Timing Equipment (CTE) time.
 * @return Returns the current CTE time.
 */
double ObjectCallbackBase::get_cte_time() const
{
   return ( exec_control != NULL ) ? exec_control->get_cte_time()
                                   : -std::numeric_limits< double >::max();
}

/*!
 * @brief Update the internal pointer to the Execution Control instance.
 */
void ObjectCallbackBase::update_exec_control_ptr()
{
   if ( object != NULL ) {
      Federate *fed = object->get_federate();
      if ( fed != NULL ) {
         exec_control = fed->get_execution_control();
      }
   }
}
