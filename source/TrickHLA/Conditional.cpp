/*!
@file TrickHLA/Conditional.cpp
@ingroup TrickHLA
@brief This class provides a user API for the handling of a Conditional
attribute.

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
@trick_link_dependency{Conditional.cpp}
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Object.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, DSES, Oct 2009, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, November 2023, --, Added initialize_callback().}
@revs_end

*/

// TrickHLA include files.
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Conditional::Conditional() // RETURN: -- None.
   : object( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Conditional::~Conditional() // RETURN: -- None.
{
   return;
}

/*!
 * @brief Initialize the callback object to the supplied Object pointer.
 * @param obj Associated object for this class.
 */
void Conditional::initialize_callback(
   Object *obj )
{
   this->object = obj;
}

/*!
 * @brief Default implementation to always send the attribute.
 * @return Defaults to true.
 * @param attr Pointer to TrickHLA Attribute.
 */
bool Conditional::should_send( // RETURN: -- None.
   Attribute *attr )           // IN: ** Attribute data to send
{
   return true;
}

/*!
 * @brief Get the Attribute by FOM name.
 * @return Attribute for the given name.
 * @param attr_FOM_name Attribute FOM name.
 */
Attribute *Conditional::get_attribute(
   char const *attr_FOM_name )
{
   return object->get_attribute( attr_FOM_name );
}

/*!
 * @brief This function returns the Attribute for the given attribute FOM name.
 * @return Attribute for the given name.
 * @param attr_FOM_name Attribute FOM name.
 */
Attribute *Conditional::get_attribute_and_validate(
   char const *attr_FOM_name )
{
   // Make sure the FOM name is not NULL.
   if ( attr_FOM_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Conditional::get_attribute_and_validate():" << __LINE__
             << " ERROR: Unexpected NULL attribute FOM name specified."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the attribute by FOM name.
   Attribute *attr = get_attribute( attr_FOM_name );

   // Make sure we have found the attribute.
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "Conditional::get_attribute_and_validate():" << __LINE__
             << " ERROR: For FOM object '" << object->get_FOM_name()
             << "', failed to find the Attribute for an attribute named"
             << " '" << attr_FOM_name << "'. Make sure the FOM attribute name is"
             << " correct, the FOM contains an attribute named '"
             << attr_FOM_name << "' and that your input.py file is properly"
             << " configured for this attribute." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return attr;
}
