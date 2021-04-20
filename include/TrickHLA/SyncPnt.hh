/*!
@file TrickHLA/SyncPnt.hh
@ingroup TrickHLA
@brief This class provides a sync-point implementation for storing and
managing TrickHLA synchronization points.

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
@trick_link_dependency{../source/TrickHLA/SyncPnt.cpp}
@trick_link_dependency{../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_SYNC_PNT_HH_
#define _TRICKHLA_SYNC_PNT_HH_

// System includes
#include <string>

// TrickHLA includes.
#include "TrickHLA/LoggableSyncPnt.hh"
#include "TrickHLA/StandardsSupport.hh"
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

// Forward declaration.
class Federate;

class SyncPnt
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SyncPnt();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SyncPnt class. */
   SyncPnt();

   /*! @brief Initialization constructor.
    *  @param l Synchronization point label. */
   explicit SyncPnt( std::wstring const &l );

   /*! @brief Destructor for the TrickHLA SyncPnt class. */
   virtual ~SyncPnt();

   // Functions to check synchronization point state.
   /*! @brief Check if the synchronization point has been created and exists
    *  in at least on valid state.
    *  @return True if the synchronization point has a valid state. */
   virtual bool is_valid();

   // Functions to check synchronization point state.
   /*! @brief Check if the synchronization point exists.
    *  @return True if the synchronization point exists. */
   virtual bool exists();

   /*! @brief Check if the synchronization point is registered.
    *  @return True if the synchronization point is registered. */
   virtual bool is_registered();

   /*! @brief Check if the synchronization point is announced.
    *  @return True if the synchronization point is announced. */
   virtual bool is_announced();

   /*! @brief Check if the synchronization point is achieved.
    *  @return True if the synchronization point is achieved. */
   virtual bool is_achieved();

   /*! @brief Check if the synchronization point is synchronized.
    *  @return True if the synchronization point is synchronized. */
   virtual bool is_synchronized();

   /*! @brief Check if the synchronization point has a bad state.
    *  @return True if the synchronization point has a bad state. */
   virtual bool is_error();

   // Accessor functions.
   /*! @brief Get the synchronization point label.
    *  @return The synchronization point label. */
   virtual std::wstring const &get_label() const
   {
      return label;
   }

   /*! @brief Get the synchronization point state.
    *  @return The current state for this synchronization point. */
   virtual const SyncPtStateEnum get_state() const
   {
      return state;
   }

   /*! @brief Set the synchronization point label.
    *  @param l The synchronization point label. */
   virtual void set_label( std::wstring const &l )
   {
      label = l;
   }

   /*! @brief Set the current state of the synchronization point.
    *  @param s Current synchronization point state. */
   virtual void set_state( SyncPtStateEnum s )
   {
      state = s;
   }

   // Utility functions.
   /*! @brief Create a C++ wide string with the synchronization point label and
    * current state.
    *  @return A wide string with the synchronization point label and current state. */
   virtual std::wstring to_wstring();

   /*! @brief Convert the synchronization point into and loggable
    * synchronization point.
    *  @param log_sync_pnt Reference to a loggable synchronization point. */
   virtual void convert( LoggableSyncPnt &log_sync_pnt );

  protected:
   std::wstring    label; ///< @trick_io{**} Sync-point name.
   SyncPtStateEnum state; ///< @trick_units{--} Sync-point state.
};

} // namespace TrickHLA

#endif /* _TRICKHLA_SYNC_PNT_HH_ */
