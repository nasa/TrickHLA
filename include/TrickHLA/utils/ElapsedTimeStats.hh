/*!
@file TrickHLA/utils/ElapsedTimeStats.hh
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
@trick_link_dependency{../../../source/TrickHLA/utils/ElapsedTimeStats.cpp}

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
   /*! @brief Constructor for the TrickHLA ElapsedTimeStats class. */
   explicit ElapsedTimeStats( bool const enable );
   /*! @brief Constructor for the TrickHLA ElapsedTimeStats class. */
   ElapsedTimeStats( bool const enable, std::string const &message );
   /*! @brief Destructor for the TrickHLA ElapsedTimeStats class. */
   virtual ~ElapsedTimeStats();

   /*! @brief Enable or disable the collection of statistics. */
   void set_enabled( bool const enable )
   {
      this->enabled = enable;
   }

   /*! @brief Is the collection of statistics enabled. */
   bool is_enabled() const
   {
      return this->enabled;
   }

   /*! @brief Set a description. */
   void set_description( std::string const &message )
   {
      this->description = message;
   }

   /*! @brief Start the timer for the elapsed time measurement. */
   void start_timer();

   /*! @brief Measure the elapsed time. */
   void measure();

   /*! @brief True if any valid measurements have been taken. */
   bool any_measurements() const
   {
      return ( this->count > 0 );
   }

   /*! @brief Convert confidence level to Z value. */
   static double confidence_to_Z( double &confidence );

   /*! @brief Returns a string summary of the elapsed time statistics. */
   std::string const to_string();

  private:
   bool enabled; ///< @trick_units{--} Flag to enable time measurements.

   std::string description; ///< @trick_units{--} Description message.

   int64_t start_time; ///< @trick_units{microseconds} Start time for the measurement.

   uint64_t count; ///< @trick_units{--} Number of elapsed times measured.

   double elapsed_time;      ///< @trick_units{milliseconds} Current elapsed time.
   double elapsed_time_prev; ///< @trick_units{milliseconds} Previous elapsed time.

   double elapsed_time_avg; ///< @trick_units{milliseconds} Running average elapsed time measured.
   double elapsed_time_min; ///< @trick_units{milliseconds} Minimum elapsed time measured.
   double elapsed_time_max; ///< @trick_units{milliseconds} Maximum elapsed time measured.

   double elapsed_time_sum;    ///< @trick_units{milliseconds} Sum of the elapsed time measured.
   double elapsed_time_sq_sum; ///< @trick_units{milliseconds^2} Sum of the elapsed time squared.

   double jitter; ///< @trick_units{milliseconds} Current jitter in the elapsed time.

   double jitter_avg; ///< @trick_units{milliseconds} Running average jitter time measured.
   double jitter_min; ///< @trick_units{milliseconds} Minimum jitter time measured.
   double jitter_max; ///< @trick_units{milliseconds} Maximum jitter time measured.

   double jitter_sum; ///< @trick_units{milliseconds} Sum of the jitter times.
};

} // namespace TrickHLA

#endif // TRICKHLA_ELAPSED_TIME_STATS_HH: Do NOT put anything after this line!
