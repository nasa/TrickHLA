#ifndef Distributed2WayBusBase_EXISTS
#define Distributed2WayBusBase_EXISTS

/**
 * @defgroup Examples Examples
 * @details The collection of example code for TrickHLA
 */

/**
 * @defgroup DistIf DistIf
 * @ingroup Examples
 * @details A distributed systems interface code example for TrickHLA
 */

/**
@ingroup DistIf
@file    Distributed2WayBusBase.hh
@brief   Distributed 2-Way Bus Base Interface declarations

@details Classes for the Distributed 2-Way Bus Base Interface.

@copyright Copyright 2024 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{DistIf}

@tldh
@trick_link_dependency{../src/Distributed2WayBusBase.cpp}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

@{

*/

#include <string>
#include <vector>

namespace DistIf
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed 2-Way Bus Base Interface Data
///
/// @details  This holds the data that is transferred between instances of
///           Distributed2WayBusBase across the sim-sim interface (HLA, etc.)
///
/// @note     This must remain a base class, since it can be used in multiple inheritance elsewhere
///           in the baseline, to avoid the diamond inheritance problem.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Distributed2WayBusBaseInterfaceData // !!! NOTE this must remain a base class, see above !!!
{
  public:
   unsigned int mFrameCount;    /**< (1) Frame count driven by this side. */
   unsigned int mFrameLoopback; /**< (1) Frame count driven by other side, echoed back. */
   bool         mDemandMode;    /**< (1) Demand mode flag. */
   /// @brief  Default constructs this Distributed 2-Way Bus Base interface data.
   Distributed2WayBusBaseInterfaceData();
   /// @brief  Default destructs this Distributed 2-Way Bus Base interface data.
   virtual ~Distributed2WayBusBaseInterfaceData();
   /// @brief  Returns whether this object has received valid data.
   virtual bool hasValidData() const = 0;
   /// @brief Assignment operator for this Distributed 2-Way Bus Base interface data.
   Distributed2WayBusBaseInterfaceData &operator=( Distributed2WayBusBaseInterfaceData const &that );

  private:
   /// @brief Copy constructor unavailable since declared private and not implemented.
   Distributed2WayBusBaseInterfaceData( Distributed2WayBusBaseInterfaceData const & );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed 2-Way Bus Base notification message.
///
/// @details  This describes a notification message from the Distributed2WayBusBase to the
///           outside, including severity level and message string.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Distributed2WayBusNotification
{
  public:
   /// @brief Enumeration of notification severity levels.
   typedef enum {
      INFO = 0, ///< Information.
      WARN = 1, ///< Warning.
      ERR  = 2, ///< Error.
      NONE = 3  ///< No notification, empty queue.
   } NotificationLevel;
   NotificationLevel mLevel;   /**< (1) The severity level of the notification. */
   std::string       mMessage; /**< (1) the notification message. */
   /// @brief Default constructor.
   explicit Distributed2WayBusNotification( NotificationLevel const level = NONE, std::string const &message = "" );
   /// @brief Default destructor.
   virtual ~Distributed2WayBusNotification();
   /// @brief Copy constructor.
   Distributed2WayBusNotification( Distributed2WayBusNotification const &that );
   /// @brief Assignment operator.
   Distributed2WayBusNotification &operator=( Distributed2WayBusNotification const &that );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed 2-Way Bus Base Interface.
///
/// @details  This is the base class for 2-way bus distributed interface logic models.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Distributed2WayBusBase
{
  public:
   /// @brief Enumeration of interface roles. */
   typedef enum {
      NONE   = 0, ///< No role.
      SUPPLY = 1, ///< Supply role.
      DEMAND = 2  ///< Demand role.
   } Roles;
   /// @brief Default Constructor.
   Distributed2WayBusBase( Distributed2WayBusBaseInterfaceData *inDataPtr,
                           Distributed2WayBusBaseInterfaceData *outDataPtr );
   /// @brief Default Destructor.
   virtual ~Distributed2WayBusBase();
   /// @brief Initializes this Distributed 2-Way Bus Interface.
   void initialize( bool const isPairMaster );
   /// @brief Forces this interface to remain in Demand role.
   void forceDemandRole();
   /// @brief Forces this interface to remain in Supply role.
   void forceSupplyRole();
   /// @brief Resets the forced role and lets the interface logic determine role normally.
   void resetForceRole();
   /// @brief Returns whether this Distributed 2-Way Bus Interface is in the Demand role.
   bool isInDemandRole() const;
   /// @brief Pops a notification message off of the queue and returns the remaining queue size.
   unsigned int popNotification( Distributed2WayBusNotification &notification );

  protected:
   bool                                          mIsPairMaster;         /**<    (1) cio(**) This is the master side of the interface. */
   bool                                          mInDataLastDemandMode; /**<    (1)         Last-pass demand mode from the other paired link. */
   int                                           mFramesSinceFlip;      /**<    (1)         Number of frames since the last mode flip. */
   int                                           mLoopLatency;          /**<    (1)         Round-trip loop data lag measurement. */
   Roles                                         mForcedRole;           /**<    (1)         The role this interface is forced to be in, if any. */
   std::vector< Distributed2WayBusNotification > mNotifications;        /**< ** (1) cio(**) Notifications queue. */
   /// @brief Updates frame counters and the loop latency measurement.
   void updateFrameCounts();
   /// @brief Adds a notification message to the queue.
   void pushNotification( Distributed2WayBusNotification::NotificationLevel const level,
                          std::string const                                      &message );

  private:
   Distributed2WayBusBaseInterfaceData *mInDataPtr;  /**<    (1) cio(**) Pointer to the derived class incoming data instance. */
   Distributed2WayBusBaseInterfaceData *mOutDataPtr; /**<    (1) cio(**) Pointer to the derived class outgoing data instance. */
   /// @brief Copy constructor unavailable since declared private and not implemented.
   Distributed2WayBusBase( Distributed2WayBusBase const &that );
   /// @brief Assignment operator unavailable since declared private and not implemented.
   Distributed2WayBusBase &operator=( Distributed2WayBusBase const &that );
};

} // namespace DistIf

/// @}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Sets the mForcedRole attribute to DEMAND.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void DistIf::Distributed2WayBusBase::forceDemandRole()
{
   mForcedRole = DEMAND;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Sets the mForcedRole attribute to SUPPLY.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void DistIf::Distributed2WayBusBase::forceSupplyRole()
{
   mForcedRole = SUPPLY;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Sets the mForcedRole attribute to NONE.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void DistIf::Distributed2WayBusBase::resetForceRole()
{
   mForcedRole = NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @returns  bool (1) True if this is currently in the Demand role.
///
/// @details  Returns the value of mDemandMode from the derived class outgoing data instance.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool DistIf::Distributed2WayBusBase::isInDemandRole() const
{
   return mOutDataPtr->mDemandMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] level   (--) Severity level of the message.
/// @param[in] message (--) Detailed message string.
///
/// @details  Adds a new notification message object to the message queue.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void DistIf::Distributed2WayBusBase::pushNotification(
   DistIf::Distributed2WayBusNotification::NotificationLevel const level,
   std::string const                                              &message )
{
   mNotifications.push_back( DistIf::Distributed2WayBusNotification( level, message ) );
}

#endif
