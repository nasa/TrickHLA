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

@tldh
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityLagCompInteg.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityLagComp.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_HH
#define SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_HH

// SpaceFOM includes.
#include "SpaceFOM/PhysicalEntityLagCompInteg.hh"

namespace SpaceFOM
{

class PhysicalEntityLagComp : public PhysicalEntityLagCompInteg
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
   explicit PhysicalEntityLagComp( PhysicalEntityBase &entity_ref ); // Initialization constructor.
   virtual ~PhysicalEntityLagComp();                                 // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

  protected:
   double *integ_states[13]; ///< @trick_units{--} @trick_io{**} Integration states.

   /*! @brief Update the latency compensation time from the integrator. */
   virtual void update_time();

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
   virtual void derivative_second( void *user_data ) { return; }

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
