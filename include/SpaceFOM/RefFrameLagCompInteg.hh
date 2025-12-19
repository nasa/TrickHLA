/*!
@file SpaceFOM/RefFrameLagCompInteg.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM reference frame latency/lag
compensation class that uses integration to compensate the reference frame.

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
@trick_link_dependency{../../source/TrickHLA/LagCompensationInteg.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameLagCompBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameLagCompInteg.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_LAG_COMP_INTEG_HH
#define SPACEFOM_REF_FRAME_LAG_COMP_INTEG_HH

// SpaceFOM includes.
#include "SpaceFOM/RefFrameLagCompBase.hh"

// TrickHLA includes.
#include "TrickHLA/LagCompensationInteg.hh"

namespace SpaceFOM
{

class RefFrameLagCompInteg : public RefFrameLagCompBase, public TrickHLA::LagCompensationInteg
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameLagCompInteg();

  public:
   // Public constructors and destructors.
   explicit RefFrameLagCompInteg( RefFrameBase &ref_frame_ref ); // Initialization constructor.
   virtual ~RefFrameLagCompInteg();                              // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Sending side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void send_lag_compensation();

   /*! @brief Receive side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void receive_lag_compensation();

  protected:
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

  private:
   // This object is not copyable
   /*! @brief Copy constructor for RefFrameLagCompInteg class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameLagCompInteg( RefFrameLagCompInteg const &rhs );
   /*! @brief Assignment operator for RefFrameLagCompInteg class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameLagCompInteg &operator=( RefFrameLagCompInteg const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_LAG_COMP_INTEG_HH: Do NOT put anything after this line!
