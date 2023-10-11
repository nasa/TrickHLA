/*!
@file SpaceFOM/PhysicalEntityLagComp.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical entity latency/lag
compensation class that uses the Trick integrators.

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

@tldh:q
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityLagCompBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityLagComp.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_HH
#define SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_HH

// System include files.

// Trick includes.
#include "trick/Integrator.hh"

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityLagCompBase.hh"

namespace SpaceFOM
{

class PhysicalEntityLagComp : public PhysicalEntityLagCompBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalEntityLagComp();

  public:
   // Public constructors and destructors.
   PhysicalEntityLagComp( PhysicalEntityBase & entity_ref ); // Initialization constructor.
   virtual ~PhysicalEntityLagComp(); // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Sending side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void send_lag_compensation();

   /*! @brief Receive side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void receive_lag_compensation();

   void set_tolerance( double tol ){
      integ_tol = tol;
   }

  protected:
   double integ_t;   ///< @trick_units{s} Current compensation propagation time.
   double integ_dt;  ///< @trick_units{s} Default integration time steps.
   double integ_tol; ///< @trick_units{s} Tolerance for terminating a compensation step.

   double * integ_states[13]; ///< @trick_units{--} @trick_io{**} Integration states.
   Trick::Integrator * integrator; ///< @trick_units{--} Reference to a specific Trick integration method.

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   int compensate(
      const double t_begin,
      const double t_end   );

   /*! @brief Load the integration state into the integrator. */
   virtual void load();

   /*! @brief Load the integration state into the integrator. */
   virtual void unload();

  private:
   // This object is not copyable
   /*! @brief Copy constructor for PhysicalEntityLagComp class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalEntityLagComp( PhysicalEntityLagComp const &rhs );
   /*! @brief Assignment operator for PhysicalEntityLagComp class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalEntityLagComp &operator=( PhysicalEntityLagComp const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_HH: Do NOT put anything after this line!
