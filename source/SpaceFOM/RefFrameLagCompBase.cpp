/*!
@file SpaceFOM/RefFrameLagCompBase.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
RefFrame latency/lag compensation class.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{RefFrameLagCompBase.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>
#include <float.h>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h" // for send_hs
#include "trick/trick_math.h"
#include "trick/trick_math_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Attribute.hh"

// SpaceFOM include files.
#include "SpaceFOM/RefFrameLagCompBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameLagCompBase::RefFrameLagCompBase( RefFrameBase & ref_frame_ref ) // RETURN: -- None.
   : debug( false ),
     ref_frame( ref_frame_ref ),
     state_attr(NULL),
     compensate_dt( 0.0 )
{

}


/*!
 * @job_class{shutdown}
 */
RefFrameLagCompBase::~RefFrameLagCompBase() // RETURN: -- None.
{

}


/*!
 * @job_class{initialization}
 */
void RefFrameLagCompBase::initialize()
{

   // Return to calling routine.
   return;
}

/*!
 * @details From the TrickHLA::LagCompensation class. We override this function
 * so that we can initialize references to the TrickHLA::Attribute's that are
 * used in the unpack function to handle attribute ownership and different
 * attribute data rates.
 *
 * Use the initialize callback function as a way to setup TrickHLA::Attribute
 * references which are use to determine ownership or if data for an attribute
 * was received.
 *
 * @job_class{initialization}
 */
void RefFrameLagCompBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   this->TrickHLA::LagCompensation::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   state_attr = ref_frame.state_attr;

   return;
}


/*! @brief Initialization integration states. */
void RefFrameLagCompBase::initialize_states()
{

   // Copy the current RefFrameLag state over to the lag compensated state.
   this->lag_comp_data = this->ref_frame.packing_data.state;
   this->Q_dot.derivative_first( this->lag_comp_data.att,
                                 this->lag_comp_data.ang_vel);

   // Return to calling routine.
   return;
}


/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::bypass_send_lag_compensation()
{
   // When lag compensation is present but disabled, we still need to copy
   // the working data into the packing data.  This makes sure that the
   // current working state is packed.
   this->ref_frame.pack_from_working_data();
   return;
}


/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::bypass_receive_lag_compensation()
{
   // When lag compensation is present but disabled, we still need to copy
   // the packing data back into the working data.  This makes sure that the
   // working state is updated from the received packing data.
   this->ref_frame.unpack_into_working_data();
   return;
}


/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::unload_lag_comp_data()
{
   // Copy the current RefFrameLag state over to the lag compensated state.
   this->ref_frame.packing_data.state = this->lag_comp_data;

   return;
}


/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::load_lag_comp_data()
{
   // Copy the current RefFrameLag state over to the lag compensated state.
   this->lag_comp_data = this->ref_frame.packing_data.state;

   return;
}


/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::print_lag_comp_data()
{
   double euler_angles[3];
   double * quat = &(lag_comp_data.att.scalar);

   // Compute the attitude Euler angles.
   euler_quat( euler_angles, quat, 1, Pitch_Yaw_Roll );

   cout << "\tScenario time: " << this->get_scenario_time() << endl;
   cout << "\tLag comp time: " << this->lag_comp_data.time << endl;
   cout << "\tposition: "
        << "\t\t" << this->lag_comp_data.pos[0] << ", "
        << "\t\t" << this->lag_comp_data.pos[1] << ", "
        << "\t\t" << this->lag_comp_data.pos[2] << endl;
   cout << "\tvelocity: "
        << "\t\t" << this->lag_comp_data.vel[0] << ", "
        << "\t\t" << this->lag_comp_data.vel[1] << ", "
        << "\t\t" << this->lag_comp_data.vel[2] << endl;
   cout << "\tattitude (s;v): "
        << "\t\t" << this->lag_comp_data.att.scalar << "; "
        << "\t\t" << this->lag_comp_data.att.vector[0] << ", "
        << "\t\t" << this->lag_comp_data.att.vector[1] << ", "
        << "\t\t" << this->lag_comp_data.att.vector[2] << endl;
   cout << "\tattitude (PYR): "
        << "\t\t" << euler_angles[0] << ", "
        << "\t\t" << euler_angles[1] << ", "
        << "\t\t" << euler_angles[2] << endl;
   cout << "\tangular velocity: "
        << "\t\t" << this->lag_comp_data.ang_vel[0] << ", "
        << "\t\t" << this->lag_comp_data.ang_vel[1] << ", "
        << "\t\t" << this->lag_comp_data.ang_vel[2] << endl;
   cout << "\tattitude rate (s;v): "
        << "\t\t" << this->Q_dot.scalar << "; "
        << "\t\t" << this->Q_dot.vector[0] << ", "
        << "\t\t" << this->Q_dot.vector[1] << ", "
        << "\t\t" << this->Q_dot.vector[2] << endl;

   // Return to the calling routine.
   return;
}
