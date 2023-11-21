/*!
@file TrickHLA/LagCompensationIntegBase.hh
@ingroup TrickHLA
@brief This class is really just an interface class for TrickHLA lag
compensation using integration.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLA}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef TRICKHLA_LAG_COMPENSATION_INTEG_BASE_HH
#define TRICKHLA_LAG_COMPENSATION_INTEG_BASE_HH

// System include files.
#include <stddef.h>

// TrickHLA include files.
#include "trick/Integrator.hh"

namespace TrickHLA
{

class LagCompensationIntegBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__LagCompensationIntegBase();

  public:
   //-----------------------------------------------------------------
   // Public constructors and destructors.
   //-----------------------------------------------------------------
   LagCompensationIntegBase()
      : integ_t( 0.0 ),
        integ_dt( 0.05 ),
        integ_tol( 1.0e-8 ){ return; }
   virtual ~LagCompensationIntegBase(){ return; }

   /*! @brief Set the lag compentation integration time step.
    *  @param dt Integration step time. */
   void set_integ_dt( double dt ){
      integ_dt = dt;
   }

   /*! @brief Set the lag compentation integration tolerance.
    *  @param tol Integration step time. */
   void set_integ_tolerance( double tol ){
      integ_tol = tol;
   }

  protected:
   double integ_t;   ///< @trick_units{s} Current compensation propagation time.
   double integ_dt;  ///< @trick_units{s} Default integration time steps.
   double integ_tol; ///< @trick_units{s} Tolerance for terminating a compensation step.

   /*! @brief Update the latency compensation time from the integrator. */
   virtual void update_time() = 0;

   /*! @brief Load the integration state into the integrator. */
   virtual void load() = 0;

   /*! @brief Unload the integration state from the integrator. */
   virtual void unload() = 0;

   /*! @brief Compute the first time derivative of the lag compensation state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_first( void * user_data = NULL ) = 0;

   /*! @brief Compute the second time derivative of the lag compensation state vector.
    *  @details This function is called for second order integrators to compute
    *  second time derivative of the state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_second( void * user_data = NULL ) = 0;

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int integrate(
      const double t_begin,
      const double t_end   ) = 0;

};

} // namespace TrickHLA

#endif // TRICKHLA_LAG_COMPENSATION_INTEG_BASE_HH: Do NOT put anything after this line!
