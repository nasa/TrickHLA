/*!
@file TrickHLA/SyncPointTimed.hh
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
@trick_link_dependency{../../source/TrickHLA/SyncPointTimed.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPoint.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_SYNC_POINT_TIMED_HH
#define TRICKHLA_SYNC_POINT_TIMED_HH

// System includes
#include <string>

// TrickHLA includes.
#include "Int64Time.hh"
#include "StandardsSupport.hh"
#include "SyncPoint.hh"

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

class SyncPointTimed : public TrickHLA::SyncPoint
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SyncPointTimed();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor. */
   SyncPointTimed();

   /*! @brief Initialization constructor.
    *  @param label Synchronization point label. */
   explicit SyncPointTimed( std::wstring const &label );

   /*! @brief Initialization constructor.
    *  @param t Synchronization point action time.
    *  @param label Synchronization point label. */
   SyncPointTimed( std::wstring const &label, TrickHLA::Int64Time const &t );

   /*! @brief Destructor for the TrickHLA SyncPointTimed class. */
   virtual ~SyncPointTimed();

   // Accessor functions.
   /*! @brief Get the synchronization point action time.
    *  @return Time for synchronization point action. */
   virtual TrickHLA::Int64Time const &get_time() const
   {
      return this->time;
   }

   /*! @brief Set the synchronization point action time.
    *  @param t The synchronization point action time. */
   virtual void set_time( TrickHLA::Int64Time const &t )
   {
      this->time = t;
   }

   /*! @brief Encode the user supplied tag data.
    *  @return The encoded user supplied tag. */
   virtual RTI1516_USERDATA const encode_user_supplied_tag();

   /*! @brief Decode the user supplied data.
    *  @param supplied_tag The supplied tag to decode as the user supplied tag. */
   virtual void decode_user_supplied_tag( RTI1516_USERDATA const &supplied_tag );

   // Utility functions.
   /*! @brief Create a string with the synchronization point label and current state.
    *  @return A string with the synchronization point label and current state. */
   virtual std::string to_string();

  protected:
   Int64Time time; ///< @trick_units{--} Synchronization point action time.
};

} // namespace TrickHLA

#endif /* TRICKHLA_SYNC_POINT_TIMED_HH */
