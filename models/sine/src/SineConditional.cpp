/*!
@file models/sine/src/SineConditional.cpp
@ingroup TrickHLAModel
@brief Subclass the base class to provide sine wave-specific CONDITIONAL attribute.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{sine/src/SineConditional.o}
@trick_link_dependency{sine/src/SineData.o}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, DSES, October 2009, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h" // for send_hs

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"

// Model include files.
#include "../include/SineConditional.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;


/*!
 * @job_class{initialization}
 */
SineConditional::SineConditional()
   : TrickHLA::Conditional(),
     sim_data(NULL),
     attr_pos(-1)
{
}


/*!
 * @job_class{shutdown}
 */
SineConditional::~SineConditional()
{
}


/*!
 * @job_class{initialization}
 */
void SineConditional::initialize(
   SineData *data,
   const char *attr_FOM_name)
{
   sim_data = data;

   // Make a copy of the incoming data so that we have something to compare to
   // when it comes time to compare (especially SineData's 'name', which is a
   // char *).
   prev_sim_data = *data;

   attr_pos = convert_FOM_name_to_pos(attr_FOM_name);
}


/*!
 * @job_class{scheduled}
 */
bool SineConditional::should_send(
   TrickHLA::Attribute *attr)
{
   bool rc = false; // if there is no data or wrong attribute, send nothing!

   // If there is simulation data to compare to, if the attribute FOM name has
   // been specified and if the specified attribute position matches the
   // supplied attribute's position, check the value of the current simulation
   // variable versus the previous value. return true if there was a change.
   if ((sim_data != NULL) && (attr_pos != -1) &&
       (convert_FOM_name_to_pos(attr->get_FOM_name()) == attr_pos)) {

      switch (attr_pos) {
         case 0: { // "Time"
            if (sim_data->get_time() != prev_sim_data.get_time()) {
               rc = true;
            }
            break;
         }
         case 1: { // "Value"
            if (sim_data->get_value() != prev_sim_data.get_value()) {
               rc = true;
            }
            break;
         }
         case 2: { // "dvdt"
            if (sim_data->get_derivative() != prev_sim_data.get_derivative()) {
               rc = true;
            }
            break;
         }
         case 3: { // "Phase"
            if (sim_data->get_phase() != prev_sim_data.get_phase()) {
               rc = true;
            }
            break;
         }
         case 4: { // "Frequency"
            if (sim_data->get_frequency() != prev_sim_data.get_frequency()) {
               rc = true;
            }
            break;
         }
         case 5: { // "Amplitude"
            if (sim_data->get_amplitude() != prev_sim_data.get_amplitude()) {
               rc = true;
            }
            break;
         }
         case 6: { // "Tolerance"
            if (sim_data->get_tolerance() != prev_sim_data.get_tolerance()) {
               rc = true;
            }
            break;
         }
         case 7: { // "Name"
            if (strcmp(sim_data->get_name(), prev_sim_data.get_name()) != 0) {
               rc = true;
            }
            break;
         }
      }

      prev_sim_data = *sim_data; // make a copy of the current data
   } else {
      send_hs(stderr, "SineConditional::should_send('%s') => ERROR: Either you \
forgot to call the initialize() routine to specify the attribute FOM name from \
the sim_data you wish to track or you provided the wrong TrickHLA-Attribute to \
an already-initialized SineConditional!",
              attr->get_FOM_name());
   }
   return rc;
}


/*!
 * @details If a match does not exist or a empty string was supplied, -1 is
 * returned.
 * @job_class{scheduled}
 */
int SineConditional::convert_FOM_name_to_pos(
   const char *attr_FOM_name)
{
   string attr_name = attr_FOM_name;

   if (!attr_name.empty()) {
      // Speed up the code by NOT using string compares, which are very costly!
      // instead, compare the first character of the attribute. since there is
      // only one overlapping first character, this should be a very fast
      // algorithm...
      char first = attr_name[0];
      char second = attr_name[1];

      switch (first) {
         case 'A': {
            return 5; // "Amplitude"
         }
         case 'F': {
            return 4; // "Frequency"
         }
         case 'N': {
            return 7; // "Name"
         }
         case 'P': {
            return 3; // "Phase"
         }
         case 'T': {
            if (second == 'i') {
               return 0; // "Time"
            } else {
               return 6; // "Tolerance"
            }
         }
         case 'V': {
            return 1; // "Value"
         }
         case 'd': {
            return 2; // "dvdt"
         }
      }
   }

   return -1;
}
