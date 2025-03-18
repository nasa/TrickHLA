/*!
@file TrickHLA/BasicClock.hh
@ingroup TrickHLA
@brief This class provides a function to get the current GMT time by using
either the CTE external clock if enabled or the system time-of-day.

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
@rev_entry{Dan Dexter, L-3 Communications, IMSim, March 2019, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_BASIC_CLOCK_HH
#define TRICKHLA_BASIC_CLOCK_HH

// System includes

// Trick includes
#include "trick/Executive.hh"
#include "trick/clock_proto.h"
#include "trick/exec_proto.h"

// TrickHLA includes

namespace TrickHLA
{

class BasicClock
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__BasicClock();

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the TrickHLA BasicClock class. */
   BasicClock() : first_pass( true )
   {
      return;
   }
   /*! @brief Destructor for the TrickHLA BasicClock class. */
   virtual ~BasicClock()
   {
      return;
   }

   // Use implicit copy constructor and assignment operator.

   // Get the GMT time in seconds.
   /*! @brief Get the computer system time in seconds.
    *  @return Computer system time in seconds. */
   static double get_time()
   {
      // Convert from an integer time to a floating-point time in seconds.
      return ( (double)clock_wall_time() / (double)exec_get_time_tic_value() );
   }

  private:
   bool first_pass; ///< @trick_io{**} Flag indicates first pass to determine external clock.
};

} // namespace TrickHLA

#endif // TRICKHLA_BASIC_CLOCK_HH: Do NOT put anything after this line!
