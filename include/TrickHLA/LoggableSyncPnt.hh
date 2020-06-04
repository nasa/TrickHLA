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

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, TrickHLA, July 2009, --, Checkpoint / restore of TrickHLA.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_LOGGABLE_SYNC_PNT_HH_
#define _TRICKHLA_LOGGABLE_SYNC_PNT_HH_

// Trick include files.
#include "trick/memorymanager_c_intf.h"

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

class LoggableSyncPnt
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__LoggableSyncPnt();

  public:
   /*! @brief Default constructor for the TrickHLA LoggableSyncPnt class. */
   LoggableSyncPnt() : label( NULL ), state( SYNC_PNT_STATE_UNKNOWN ) {}
   /*! @brief Destructor for the TrickHLA LoggableSyncPnt class. */
   virtual ~LoggableSyncPnt() { LoggableSyncPnt::clear(); }

  public:
   /*! @brief Clear the Trick allocated memory. */
   virtual void clear()
   {
      if ( label != NULL ) {
         if ( TMM_is_alloced( label ) ) {
            TMM_delete_var_a( label );
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
   LoggableSyncPnt( const LoggableSyncPnt &rhs );
   /*! @brief Assignment operator for LoggableSyncPnt class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   LoggableSyncPnt &operator=( const LoggableSyncPnt &rhs );
};

} // namespace TrickHLA

#endif /* _TRICKHLA_LOGGABLE_SYNC_PNT_HH_ */
