/*!
@file IMSim/SyncPntTimedLoggable.hh
@ingroup IMSim
@brief This class provides a mechanism for logging timed synchronization point
data and retrieving it from the log file.

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

@python_module{IMSim}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, TrickHLA, July 2009, --, Checkpoint / restore of TrickHLA.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_SYNC_PNT_TIMED_LOGGABLE_HH
#define TRICKHLA_SYNC_PNT_TIMED_LOGGABLE_HH

// System include files.
#include <cstdint>

// Trick include files.
#include "trick/memorymanager_c_intf.h"

// TrickHLA include files.
#include "TrickHLA/SyncPntLoggable.hh"

namespace IMSim
{

class SyncPntTimedLoggable : public TrickHLA::SyncPntLoggable
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrIMSim__SyncPntTimedLoggable();

  public:
   /*! @brief Default constructor for the TrickHLA SyncPntTimedLoggable class. */
   SyncPntTimedLoggable() : time( 0 )
   {
      return;
   }

   /*! @brief Destructor for the TrickHLA SyncPntTimedLoggable class. */
   ~SyncPntTimedLoggable()
   {
      return;
   }

  public:
   int64_t time; ///< @trick_units{--} Freeze time.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SyncPntTimedLoggable class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SyncPntTimedLoggable( SyncPntTimedLoggable const &rhs );
   /*! @brief Assignment operator for SyncPntTimedLoggable class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SyncPntTimedLoggable &operator=( SyncPntTimedLoggable const &rhs );
};

} // namespace IMSim

#endif /* TRICKHLA_SYNC_PNT_TIMED_LOGGABLE_HH */
