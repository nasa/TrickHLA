/*!
@file SpaceFOM/RefFrameLagComp.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM reference frame latency/lag
compensation class that uses the Trick integrators.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the RefFrame latency compensation object.

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
@trick_link_dependency{../../source/SpaceFOM/RefFrameLagCompInteg.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameLagComp.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_LAG_COMP_HH
#define SPACEFOM_REF_FRAME_LAG_COMP_HH

// System include files.

// Trick includes.
#include "trick/Integrator.hh"

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/RefFrameLagCompInteg.hh"

namespace SpaceFOM
{

class RefFrameLagComp : public RefFrameLagCompInteg
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameLagComp();

  public:
   // Public constructors and destructors.
   RefFrameLagComp( RefFrameBase & ref_frame_ref ); // Initialization constructor.
   virtual ~RefFrameLagComp(); // Destructor.

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
   /*! @brief Copy constructor for RefFrameLagComp class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameLagComp( RefFrameLagComp const &rhs );
   /*! @brief Assignment operator for RefFrameLagComp class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameLagComp &operator=( RefFrameLagComp const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_LAG_COMP_HH: Do NOT put anything after this line!
