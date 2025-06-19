/*!
@file TrickHLA/LagCompensationInteg.hh
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

@tldh
@trick_link_dependency{../../source/TrickHLA/LagCompensationInteg.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef TRICKHLA_LAG_COMPENSATION_INTEG_HH
#define TRICKHLA_LAG_COMPENSATION_INTEG_HH

// System includes.
#include <stddef.h>

// Trick includes.
#include "trick/Integrator.hh"

// TrickHLA includes.
#include "LagCompensationIntegBase.hh"

namespace TrickHLA
{

class LagCompensationInteg : public LagCompensationIntegBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__LagCompensationInteg();

  public:
   //-----------------------------------------------------------------
   // Public constructors and destructors.
   //-----------------------------------------------------------------
   LagCompensationInteg();
   virtual ~LagCompensationInteg() = 0;

  protected:
   Trick::Integrator *integrator; ///< @trick_units{--} Reference to a specific Trick integration method.

   /*! @brief Update the latency compensation time from the integrator. */
   virtual void update_time() = 0;

   /*! @brief Load the integration state into the integrator. */
   virtual void load() = 0;

   /*! @brief Unload the integration state from the integrator. */
   virtual void unload() = 0;

   /*! @brief Compute the first time derivative of the lag compensation state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_first( void *user_data = NULL ) = 0;

   /*! @brief Compute the second time derivative of the lag compensation state vector.
    *  @details This function is called for second order integrators to compute
    *  second time derivative of the state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_second( void *user_data = NULL ) = 0;

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int integrate(
      double const t_begin,
      double const t_end );
};

} // namespace TrickHLA

#endif // TRICKHLA_LAG_COMPENSATION_INTEG_HH: Do NOT put anything after this line!
