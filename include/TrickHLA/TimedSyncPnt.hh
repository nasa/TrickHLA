/*!
@file TrickHLA/TimedSyncPnt.hh
@ingroup TrickHLA
@brief This class extends the basis TrickHLA::SyncPnt synchronization point
implementation to add a time stamp.

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
@trick_link_dependency{../source/TrickHLA/TimedSyncPnt.cpp}
@trick_link_dependency{../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../source/TrickHLA/SyncPnt.cpp}
@trick_link_dependency{../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_TIMED_SYNC_PNT_HH
#define TRICKHLA_TIMED_SYNC_PNT_HH

// System includes
#include <string>

// TrickHLA includes.
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

namespace TrickHLA
{

class TimedSyncPnt : public TrickHLA::SyncPnt
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__TimedSyncPnt();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA TimedSyncPnt class. */
   TimedSyncPnt();

   /*! @brief Initialization constructor.
    *  @param label Synchronization point label. */
   explicit TimedSyncPnt( std::wstring const &label );

   /*! @brief Initialization constructor.
    *  @param t Synchronization point action time.
    *  @param label Synchronization point label. */
   TimedSyncPnt( Int64Time const &t, std::wstring const &label );

   /*! @brief Destructor for the TrickHLA TimedSyncPnt class. */
   virtual ~TimedSyncPnt();

   // Accessor functions.
   /*! @brief Get the synchronization point action time.
    *  @return Time for synchronization point action. */
   virtual Int64Time const &get_time() const
   {
      return time;
   }

   /*! @brief Set the synchronization point action time.
    *  @param t The synchronization point action time. */
   virtual void set_time( Int64Time const &t )
   {
      time = t;
   }

   // Utility functions.
   /*! @brief Create a C++ wide string with the synchronization point label and
    * current state.
    *  @return A string with the synchronization point label and current state. */
   virtual std::wstring to_wstring();

   /*! @brief Convert the synchronization point into and loggable
    * synchronization point.
    *  @param log_sync_pnt Reference to a loggable synchronization point. */
   virtual void convert( LoggableSyncPnt &log_sync_pnt );

  protected:
   Int64Time time; ///< @trick_units{--} Synchronization point action time.
};

} // namespace TrickHLA

#endif /* TRICKHLA_TIMED_SYNC_PNT_HH */
