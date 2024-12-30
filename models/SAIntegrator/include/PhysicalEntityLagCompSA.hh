/*!
@ingroup SAIntegrator
@file models/SAIntegrator/include/PhysicalEntityLagCompSA.hh
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
@trick_link_dependency{../src/PhysicalEntityLagCompSA.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_SA_HH
#define SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_SA_HH

// System include files.

// Trick includes.
#include "trick_utils/SAIntegrator/include/SAIntegrator.hh"

// TrickHLA include files.
#include "TrickHLA/LagCompensationIntegBase.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityLagCompInteg.hh"

namespace SpaceFOM
{

class PhysicalEntityLagCompSA : public TrickHLA::LagCompensationIntegBase, public PhysicalEntityLagCompBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalEntityLagCompSA();

  public:
   // Public constructors and destructors.
   explicit PhysicalEntityLagCompSA( PhysicalEntityBase &entity_ref ); // Initialization constructor.
   virtual ~PhysicalEntityLagCompSA();                                 // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

  protected:
   double             *integ_states[13]; ///< @trick_units{--} @trick_io{**} Integration states.
   SA::EulerIntegrator integrator;       ///< @trick_io{**} Integrator.

   /*! @brief Derivative routine used by the compensation integrator.
    *  @param t      Integration time (IN).
    *  @param states Integration states (IN).
    *  @param derivs Derivatives of the integration states (OUT).
    *  @param udata  Additional user data needed to compute the derivatives (IN).
    */
   static void derivatives( double t, double states[], double derivs[], void *udata );

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int compensate(
      double const t_begin,
      double const t_end )
   {
      this->compensate_dt = t_end - t_begin;
      return ( integrate( t_begin, t_end ) );
   }

   /*! @brief Update the latency compensation time from the integrator. */
   virtual void update_time() { lag_comp_data.time = this->integ_t; }

   /*! @brief Load the integration state into the integrator. */
   virtual void load();

   /*! @brief Unload the integration state from the integrator. */
   virtual void unload();

   /*! @brief Compute the first time derivative of the lag compensation state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_first( void *user_data = NULL );

   /*! @brief Compute the second time derivative of the lag compensation state vector.
    *  @details This function is called for second order integrators to compute
    *  second time derivative of the state vector.
    *  @param user_data Any special user data needed to compute the derivative values. */
   virtual void derivative_second( void *user_data = NULL ) { return; }

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int integrate(
      double const t_begin,
      double const t_end );

  private:
   // This object is not copyable
   /*! @brief Copy constructor for PhysicalEntityLagCompSA class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalEntityLagCompSA( PhysicalEntityLagCompSA const &rhs );
   /*! @brief Assignment operator for PhysicalEntityLagCompSA class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalEntityLagCompSA &operator=( PhysicalEntityLagCompSA const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_SA_HH: Do NOT put anything after this line!
