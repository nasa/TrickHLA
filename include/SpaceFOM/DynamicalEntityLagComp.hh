/*!
@file SpaceFOM/DynamicalEntityLagComp.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM DynamicalEntity latency/lag
compensation class that uses the Trick integrators.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the DynamicalEntity latency compensation object.

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
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityLagCompInteg.cpp}
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityLagComp.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_LAG_COMP_HH
#define SPACEFOM_DYNAMICAL_ENTITY_LAG_COMP_HH

// System include files.

// Trick includes.
#include "trick/Integrator.hh"

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityLagCompInteg.hh"

namespace SpaceFOM
{

class DynamicalEntityLagComp : public DynamicalEntityLagCompInteg
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__DynamicalEntityLagComp();

  public:
   // Public constructors and destructors.
   DynamicalEntityLagComp( DynamicalEntityBase & entity_ref ); // Initialization constructor.
   virtual ~DynamicalEntityLagComp(); // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

  protected:

   double * integ_states[13]; ///< @trick_units{--} @trick_io{**} Integration states.

   /*! @brief Update the latency compensation time from the integrator. */
   virtual void update_time();

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
   virtual void derivative_second( void * user_data ){ return; }

  private:
   // This object is not copyable
   /*! @brief Copy constructor for DynamicalEntityLagComp class.
    *  @details This constructor is private to prevent inadvertent copies. */
   DynamicalEntityLagComp( DynamicalEntityLagComp const &rhs );
   /*! @brief Assignment operator for DynamicalEntityLagComp class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   DynamicalEntityLagComp &operator=( DynamicalEntityLagComp const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_LAG_COMP_HH: Do NOT put anything after this line!
