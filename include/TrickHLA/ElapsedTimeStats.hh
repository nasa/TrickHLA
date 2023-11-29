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

@tldh
@trick_link_dependency{../../source/TrickHLA/ElapsedTimeStats.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, Sept 2020, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_ELAPSED_TIME_STATS_HH
#define TRICKHLA_ELAPSED_TIME_STATS_HH

// System includes
#include <cstdint>
#include <string>

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

   /*! @brief Convert confidence level to Z value. */
   double const confidence_to_Z( double &confidence );

   /*! @brief Returns a string summary of the elapsed time statistics. */
   const std::string to_string();

  private:
   bool first_pass; ///< @trick_units{--} Flag indicates first pass to determine external clock.

   int64_t prev_time; ///< @trick_units{microseconds} Previous elapsed time.

   uint64_t count; ///< @trick_units{--} Number of elapsed times measured.

   double elapsed_time; ///< @trick_units{milliseconds} Current elapsed time.

   double min; ///< @trick_units{milliseconds} Minimum elapsed time measured.
   double max; ///< @trick_units{milliseconds} Maximum elapsed time measured.

   double time_sum;         ///< @trick_units{milliseconds} Sum of the elapsed time measured.
   double time_squared_sum; ///< @trick_units{milliseconds^2} Sum of the elapsed time squared.
};

} // namespace TrickHLA

#endif // TRICKHLA_ELAPSED_TIME_STATS_HH: Do NOT put anything after this line!
