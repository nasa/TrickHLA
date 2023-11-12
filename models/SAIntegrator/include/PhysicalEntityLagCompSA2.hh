/*!
@file SpaceFOM/PhysicalEntityLagCompSA2.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical entity latency/lag
compensation class that uses the Trick Stand-Alone (SA) integrators.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the PhysicalEntity latency compensation object.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../../source/SpaceFOM/PhysicalEntityLagCompBase.cpp}
@trick_link_dependency{../src/PhysicalEntityLagCompSA2.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_SA2_HH
#define SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_SA2_HH

// System include files.

// Trick includes.
#include "trick_utils/SAIntegrator/include/SAIntegrator.hh"

// TrickHLA include files.

// SpaceFOM include files.
#include "TrickHLA/LagCompensationIntegBase.hh"
#include "SpaceFOM/PhysicalEntityLagCompBase.hh"

namespace SpaceFOM
{

class PhysicalEntityLagCompSA2 : public TrickHLA::LagCompensationIntegBase, public PhysicalEntityLagCompBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalEntityLagCompSA2();

  public:
   // Public constructors and destructors.
   explicit PhysicalEntityLagCompSA2( PhysicalEntityBase & entity_ref ); // Initialization constructor.
   virtual ~PhysicalEntityLagCompSA2(); // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

  protected:

   double * integ_states[7]; ///< @trick_units{--} @trick_io{**} Integrator state vector.
   double * integ_derivs[7]; ///< @trick_units{--} @trick_io{**} Integrator derivative vector.
   SA::EulerCromerIntegrator integrator;  ///< @trick_io{**} Integrator.

   /*! @brief Derivative routine used by the compensation integrator.
    *  @param t      Integration time (IN).
    *  @param states Integration states (IN).
    *  @param derivs Derivatives of the integration states (OUT).
    *  @param udate  Additional user data needed to compute the derivatives (IN).
    */
   static void derivatives( double t,
                            double pos[],
                            double vel[],
                            double accel[],
                            void* udata);

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int compensate(
      const double t_begin,
      const double t_end   )
   {
      this->compensate_dt = t_end - t_begin;
      return( integrate( t_begin, t_end ) );
   }

   /*! @brief Update the latency compensation time from the integrator. */
   virtual void update_time(){ lag_comp_data.time = this->integ_t; }

   /*! @brief Load the integration state into the integrator. */
   virtual void load();

   /*! @brief Unload the integration state from the integrator. */
   virtual void unload();

   /*! @brief Compute the first time derivative of the lag compensation state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_first( void * user_data = NULL );

   /*! @brief Compute the second time derivative of the lag compensation state vector.
    *  @detail This function is called for second order integrators to compute
    *  second time derivative of the state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_second( void * user_data = NULL ){ return; }

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int integrate(
      const double t_begin,
      const double t_end   );

  private:
   // This object is not copyable
   /*! @brief Copy constructor for PhysicalEntityLagCompSA2 class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalEntityLagCompSA2( PhysicalEntityLagCompSA2 const &rhs );
   /*! @brief Assignment operator for PhysicalEntityLagCompSA2 class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalEntityLagCompSA2 &operator=( PhysicalEntityLagCompSA2 const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_SA2_HH: Do NOT put anything after this line!
