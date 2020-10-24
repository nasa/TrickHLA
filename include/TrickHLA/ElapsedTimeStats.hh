/*!
@file TrickHLA/ElapsedTimeStats.hh
@ingroup TrickHLA
@brief This class gathers statistics on the elapsed time between calls to the
measure function.

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
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, Sept 2020, --, Initial implementation.}
@revs_end

*/

#ifndef _TRICKHLA_ELAPSED_TIME_STATS_HH_
#define _TRICKHLA_ELAPSED_TIME_STATS_HH_

// System includes
#include <string>

// Trick includes

// TrickHLA includes

namespace TrickHLA
{

class ElapsedTimeStats
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ElapsedTimeStats();

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the TrickHLA ElapsedTimeStats class. */
   ElapsedTimeStats();
   /*! @brief Destructor for the TrickHLA ElapsedTimeStats class. */
   virtual ~ElapsedTimeStats();

   // Use implicit copy constructor and assignment operator.

   /*! @brief Measure the elapsed time. */
   void measure();

   /*! @brief Returns a string summary of the elapsed time statistics. */
   const std::string to_string();

  private:
   bool first_pass; ///< @trick_io{--} Flag indicates first pass to determine external clock.

   long long elapsed_time; ///< @trick_io{--} Current elapsed time.

   long long prev_time; ///< @trick_io{--} Previous elapsed time.

   unsigned long long count; ///< @trick_io{--} Number of elapsed times measured.

   long long min; ///< @trick_io{--} Minmum elapsed time measured.
   long long max; ///< @trick_io{--} Maximum elapsed time measured.

   double time_sum;         ///< @trick_io{--} Sum of the elapsed time measured.
   double time_squared_sum; ///< @trick_io{--} Sum of the elapsed time squared.
};

} // namespace TrickHLA

#endif // _TRICKHLA_ELAPSED_TIME_STATS_HH_: Do NOT put anything after this line!
