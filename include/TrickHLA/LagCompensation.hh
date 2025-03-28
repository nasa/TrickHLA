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
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Interval.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, DSES Initial Lag Compensation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, October 2023, --, Added lag-comp bypass functions.}
@revs_end

*/

#ifndef TRICKHLA_LAG_COMPENSATION_HH
#define TRICKHLA_LAG_COMPENSATION_HH

// TrickHLA include files.
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

// Forward declarations of TrickHLA classes needed for pointers.
class Attribute;
class Object;

class LagCompensation
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
   LagCompensation() : initialized( false ), object( NULL ) { return; }
   /*! @brief Destructor for the TrickHLA LagCompensation class. */
   virtual ~LagCompensation() { return; }

   //-----------------------------------------------------------------
   // These are virtual functions and must be defined by a full class.
   //-----------------------------------------------------------------

   /*! @brief Finish the initialization of the TrickHLA LAgCompensation object. */
   virtual void initialize() { initialized = true; }

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

   //-----------------------------------------------------------------
   // Helper functions.
   //-----------------------------------------------------------------

   /*! @brief Get the Attribute by FOM name.
    *  @return Attribute for the given name.
    *  @param attr_FOM_name Attribute FOM name. */
   Attribute *get_attribute( char const *attr_FOM_name );

   /*! @brief Get the Attribute for the given attribute FOM name an validate
    *  that is exists.
    *  @return Attribute for the given name.
    *  @param attr_FOM_name Attribute FOM name. */
   Attribute *get_attribute_and_validate( char const *attr_FOM_name );

   /*! @brief Returns a copy of the object's lookahead time.
    *  @return A copy of the federate's lookahead time. */
   Int64Interval get_lookahead() const;

   /*! @brief Returns a copy of the object's granted federation time.
    *  @return A copy of the federate's current granted time. */
   Int64Time get_granted_time() const;

   /*! @brief Returns the current scenario time.
    *  @return Current scenario time. */
   double get_scenario_time() const;

   /*! @brief Returns the current Central Timing Equipment (CTE) time.
    *  @return Current CTE time. */
   double get_cte_time() const;

   /*! @brief Initialize the callback object to the supplied Object pointer.
    *  @param obj Associated object for this class. */
   virtual void initialize_callback( Object *obj );

  protected:
   bool    initialized; ///< @trick_units{--} Initialization status flag.
   Object *object;      ///< @trick_io{**} Object associated with this lag-comp class.

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
