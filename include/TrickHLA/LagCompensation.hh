/*!
@file TrickHLA/LagCompensation.hh
@ingroup TrickHLA
@brief This class is the abstract base class for TrickHLA lag compensation.

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
@trick_link_dependency{../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../../source/TrickHLA/ObjectCallbackBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, DSES Initial Lag Compensation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, October 2023, --, Added lag-comp bypass functions.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, September 2025, --, Extends ObjectCallbackBase.}
@revs_end

*/

#ifndef TRICKHLA_LAG_COMPENSATION_HH
#define TRICKHLA_LAG_COMPENSATION_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "ObjectCallbackBase.hh"

namespace TrickHLA
{

class LagCompensation : public ObjectCallbackBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__LagCompensation();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA LagCompensation class. */
   LagCompensation();
   /*! @brief Constructor for the TrickHLA LagCompensation class with a name. */
   explicit LagCompensation( std::string name );
   /*! @brief Destructor for the TrickHLA LagCompensation class. */
   virtual ~LagCompensation();

   //-----------------------------------------------------------------
   // These are virtual functions and must be defined by a full class.
   //-----------------------------------------------------------------

   /*! @brief Send side lag compensation callback. */
   virtual void send_lag_compensation();

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the send side lag compensation and your implementation must copy
    * the sim-data to the lag-comp data to effect the bypass. */
   virtual void bypass_send_lag_compensation() = 0;

   /*! @brief Receive side lag compensation callback. */
   virtual void receive_lag_compensation();

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the receive side lag compensation and your implementation must
    * copy the lag-comp data to the sim-data to effect the bypass. You must
    * make sure to check the lag-comp data was received before copying to
    * the sim-data otherwise you will be copying stale data. */
   virtual void bypass_receive_lag_compensation() = 0;

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for LagCompensation class.
    *  @details This constructor is private to prevent inadvertent copies. */
   LagCompensation( LagCompensation const &rhs );
   /*! @brief Assignment operator for LagCompensation class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   LagCompensation &operator=( LagCompensation const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_LAG_COMPENSATION_HH: Do NOT put anything after this line!
