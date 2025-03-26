/*!
@file TrickHLA/LagCompensationInteg.cpp
@ingroup TrickHLA
@brief This class provides the implementation for a TrickHLA latency/lag
compensation class using integration.

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
@trick_link_dependency{LagCompensationInteg.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <cmath>
#include <iostream>
#include <sstream>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/LagCompensationInteg.hh"

// Uncomment this define if you want additional debug information.
#define TRICK_HLA_DEBUG_INTEG

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
LagCompensationInteg::LagCompensationInteg()
   : integrator( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
LagCompensationInteg::~LagCompensationInteg()
{
   return;
}

/*!
 * @job_class{integration}
 */
int LagCompensationInteg::integrate(
   double const t_begin,
   double const t_end )
{
   int    ipass;
   double compensate_dt = t_end - t_begin;
   double dt_go         = compensate_dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream msg;
      msg << "LagCompensationInteg::integrate(): " << __LINE__
          << " Compensate: t_begin, t_end, dt_go: "
          << t_begin << ", " << t_end << ", " << dt_go << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Propagate the current RefFrame state to the desired time.
   // Set the current integration time for the integrator.
   this->integ_t          = t_begin;
   this->integrator->time = 0.0;

   // Loop through integrating the state forward to the current scenario time.
   while ( ( dt_go >= 0.0 ) && ( fabs( dt_go ) > this->integ_tol ) ) {

      // Use the inherited debug-handler to allow debug comments to be turned
      // on and off from a setting in the input file.
      if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
         ostringstream msg;
         msg << "LagCompensationInteg::integrate(): " << __LINE__
             << "Integ dt, tol, t, dt_go: "
             << this->integ_dt << ", " << this->integ_tol << ", "
             << integ_t << ", " << dt_go << '\n';
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Integration inner loop.
      // Step through the integrator's integration steps.
      do {
         // Compute the derivatives of the lag compensation state vector.
         derivative_first();

         // Load the integration states and derivatives.
         load();

         // Perform the integration propagation one integration step.
         if ( dt_go > this->integ_dt ) {
            // Not near the end; so, use the defined integration step size.
            this->integrator->dt = this->integ_dt;
         } else {
            // Near the end; so, use the remainder of the time step.
            this->integrator->dt = dt_go;
         }

         // Call the integrator.
         // Was: ipass |= integrator->integrate();
         // Only need the |= if using multiple integrators.
         ipass = integrator->integrate();

         // Unload the integrated states.
         unload();

      } while ( ipass );

      // Update the integration time.
      this->integ_t = t_begin + this->integrator->time;

      // Compute the remaining time in the compensation step.
      dt_go = compensate_dt - this->integrator->time;
   }

   // Update the lag compensated time,
   update_time();

   // Compute the derivatives of the lag compensation state vector.
   derivative_first();

   return ( 0 );
}
