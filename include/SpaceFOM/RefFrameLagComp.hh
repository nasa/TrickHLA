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

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the send side lag compensation and your implementation must copy
    * the sim-data to the lag-comp data to effect the bypass. */
   virtual void bypass_send_lag_compensation();

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the receive side lag compensation and your implementation must
    * copy the lag-comp data to the sim-data to effect the bypass. You must
    * make sure to check the lag-comp data was received before copying to
    * the sim-data otherwise you will be copying stale data. */
   virtual void bypass_receive_lag_compensation();

  protected:

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
   /*! @brief Copy constructor for RefFrameLagComp class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameLagComp( RefFrameLagComp const &rhs );
   /*! @brief Assignment operator for RefFrameLagComp class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameLagComp &operator=( RefFrameLagComp const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_LAG_COMP_HH: Do NOT put anything after this line!
