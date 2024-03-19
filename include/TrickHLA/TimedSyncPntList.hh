/*!
@file TrickHLA/TimedSyncPntList.hh
@ingroup TrickHLA
@brief This class extends the TrickHLA::SyncPntListBase class and provides an
instantiable implementation for storing and managing HLA synchronization points
for TrickHLA.

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
@trick_link_dependency{../../source/TrickHLA/TimedSyncPntList.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPnt.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPntListBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_TIMED_SYNC_PNT_LIST_HH
#define TRICKHLA_TIMED_SYNC_PNT_LIST_HH

// System includes.
#include <string>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/SyncPntListBase.hh"

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

class TimedSyncPntList : public TrickHLA::SyncPntListBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__TimedSyncPntList();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA TimedSyncPntList class. */
   TimedSyncPntList();

   /*! @brief Pure virtual destructor for the TrickHLA TimedSyncPntList class. */
   virtual ~TimedSyncPntList()
   {
      return;
   }

   /*! @brief Add the given synchronization point label to the list.
    *  @param label Synchronization point label.
    *  @return a pointer to the timed sync-point object added. */
   virtual SyncPnt *add_sync_point( std::wstring const &label );

   /*! @brief Add the given synchronization point label and action time to the list.
    *  @param label Synchronization point label.
    *  @param time  Action time.
    *  @return a pointer to the timed sync-point object added. */
   virtual SyncPnt *add_sync_point( std::wstring const &label, Int64Time const &time );

   /*! @brief Achieve all the synchronization points in the list.
    *  @return True is any synchronization point in the list was achieved.
    *  @param rti_ambassador Reference to the HLA RTI Ambassador instance. */
   virtual bool achieve_all_sync_points( RTI1516_NAMESPACE::RTIambassador &rti_ambassador )
   {
      Int64Time time( 0.0 );
      return ( achieve_all_sync_points( rti_ambassador, time ) );
   }

   /*! @brief Acknowledge all the synchronization point less than or equal to
    * the given time.
    *  @return True if acknowledged, false otherwise.
    *  @param rti_ambassador    Reference to RTI Ambassador.
    *  @param check_time Time to check. */
   virtual bool achieve_all_sync_points( RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
                                         Int64Time const                  &check_time );

   /*! @brief Determine if we have any synchronization point that has a action
    * time less than the given time.
    *  @return True if sync-point is ready to be cleared.
    *  @param check_time Time to check. */
   virtual bool check_sync_points( Int64Time const &check_time );

   /*! @brief Converts the vector of synchronization points to a
    *  checkpoint-able class.
    *  @param pts Area to populate. */
   virtual void convert_sync_points( LoggableSyncPnt *pts );

   /*! @brief Dumps synchronization point information to the screen. */
   virtual void print_sync_points();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for TimedSyncPntList class.
    *  @details This constructor is private to prevent inadvertent copies. */
   TimedSyncPntList( TimedSyncPntList const &rhs );
   /*! @brief Assignment operator for TimedSyncPntList class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   TimedSyncPntList &operator=( TimedSyncPntList const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_TIMED_SYNC_PNT_LIST_HH */
