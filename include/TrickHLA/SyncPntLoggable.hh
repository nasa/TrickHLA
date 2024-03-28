/*!
@file TrickHLA/LoggableSyncPnt.hh
@ingroup TrickHLA
@brief This class provides a mechanism for logging sync point data and
retrieving it from the log file.

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
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, TrickHLA, July 2009, --, Checkpoint / restore of TrickHLA.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_LOGGABLE_SYNC_PNT_HH
#define TRICKHLA_LOGGABLE_SYNC_PNT_HH

// System include files.

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h" // for send_hs

// TrickHLA include files.
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

class LoggableSyncPnt
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__LoggableSyncPnt();

  public:
   /*! @brief Default constructor for the TrickHLA LoggableSyncPnt class. */
   LoggableSyncPnt() : label( NULL ), state( SYNC_PT_STATE_UNKNOWN )
   {
      return;
   }
   /*! @brief Destructor for the TrickHLA LoggableSyncPnt class. */
   virtual ~LoggableSyncPnt()
   {
      LoggableSyncPnt::clear();
   }

  public:
   /*! @brief Clear the Trick allocated memory. */
   virtual void clear()
   {
      if ( label != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( label ) ) ) {
            send_hs( stderr, "LoggableSyncPnt::clear():%d ERROR deleting Trick Memory for 'label'\n", __LINE__ );
         }
         label = NULL;
      }
   }

   char *label; ///< @trick_units{--} Sync-point name.
   int   state; ///< @trick_units{--} Sync-point state.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for LoggableSyncPnt class.
    *  @details This constructor is private to prevent inadvertent copies. */
   LoggableSyncPnt( LoggableSyncPnt const &rhs );
   /*! @brief Assignment operator for LoggableSyncPnt class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   LoggableSyncPnt &operator=( LoggableSyncPnt const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_LOGGABLE_SYNC_PNT_HH */
