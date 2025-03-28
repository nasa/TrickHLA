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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{RefFrameLagCompBase.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <float.h>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/trick_math.h"
#include "trick/trick_math_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/RefFrameLagCompBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameLagCompBase::RefFrameLagCompBase( RefFrameBase &ref_frame_ref ) // RETURN: -- None.
   : debug( false ),
     ref_frame( ref_frame_ref ),
     state_attr( NULL ),
     compensate_dt( 0.0 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameLagCompBase::~RefFrameLagCompBase() // RETURN: -- None.
{
   return;
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
   TrickHLA::LagCompensation::initialize_callback( obj );

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
   this->lag_comp_data = ref_frame.packing_data.state;
   Q_dot.derivative_first( lag_comp_data.att, lag_comp_data.ang_vel );

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
   ref_frame.pack_from_working_data();
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
   ref_frame.unpack_into_working_data();
   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::unload_lag_comp_data()
{
   // Copy the current RefFrameLag state over to the lag compensated state.
   ref_frame.packing_data.state = this->lag_comp_data;

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::load_lag_comp_data()
{
   // Copy the current RefFrameLag state over to the lag compensated state.
   this->lag_comp_data = ref_frame.packing_data.state;

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameLagCompBase::print_lag_comp_data( std::ostream &stream ) const
{
   double euler_angles[3];
   double quat[4];
   quat[0] = lag_comp_data.att.scalar;
   quat[1] = lag_comp_data.att.vector[0];
   quat[2] = lag_comp_data.att.vector[1];
   quat[3] = lag_comp_data.att.vector[2];

   // Set the print precision.
   stream.precision( 15 );

   // Compute the attitude Euler angles.
   euler_quat( euler_angles, quat, 1, Pitch_Yaw_Roll );

   stream << "\tScenario time: " << get_scenario_time() << '\n';
   stream << "\tLag comp time: " << this->lag_comp_data.time << '\n';
   stream << "\tposition: "
          << "\t\t" << this->lag_comp_data.pos[0] << ", "
          << "\t\t" << this->lag_comp_data.pos[1] << ", "
          << "\t\t" << this->lag_comp_data.pos[2] << '\n';
   stream << "\tvelocity: "
          << "\t\t" << this->lag_comp_data.vel[0] << ", "
          << "\t\t" << this->lag_comp_data.vel[1] << ", "
          << "\t\t" << this->lag_comp_data.vel[2] << '\n';
   stream << "\tattitude (s;v): "
          << "\t\t" << this->lag_comp_data.att.scalar << "; "
          << "\t\t" << this->lag_comp_data.att.vector[0] << ", "
          << "\t\t" << this->lag_comp_data.att.vector[1] << ", "
          << "\t\t" << this->lag_comp_data.att.vector[2] << '\n';
   stream << "\tattitude (PYR): "
          << "\t\t" << euler_angles[0] << ", "
          << "\t\t" << euler_angles[1] << ", "
          << "\t\t" << euler_angles[2] << '\n';
   stream << "\tangular velocity: "
          << "\t\t" << this->lag_comp_data.ang_vel[0] << ", "
          << "\t\t" << this->lag_comp_data.ang_vel[1] << ", "
          << "\t\t" << this->lag_comp_data.ang_vel[2] << '\n';
   stream << "\tattitude rate (s;v): "
          << "\t\t" << this->Q_dot.scalar << "; "
          << "\t\t" << this->Q_dot.vector[0] << ", "
          << "\t\t" << this->Q_dot.vector[1] << ", "
          << "\t\t" << this->Q_dot.vector[2] << '\n';

   // Return to the calling routine.
   return;
}
