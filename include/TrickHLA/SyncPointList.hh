/*!
@file TrickHLA/SyncPointList.hh
@ingroup TrickHLA
@brief This class will manage different lists of HLA synchronization points. It
       is intended for this class to be extended by an Execution Control class.

@copyright Copyright 2024 United States Government as represented by the
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
@trick_link_dependency{../../source/TrickHLA/SyncPointList.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPoint.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPointTimed.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2024, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_SYNC_POINT_LIST_HH
#define TRICKHLA_SYNC_POINT_LIST_HH

// System includes.
#include <map>
#include <string>
#include <vector>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointTimed.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

#define SYNC_POINT_TMM_ARRAY 1

namespace TrickHLA
{

#if SYNC_POINT_TMM_ARRAY
typedef SyncPointTimed *SyncPointTimesPtr; // Needed so that Trick will ICG SyncPointTimed.
#else
typedef std::vector< SyncPoint * > SyncPointVector;
#endif

class SyncPointList : public TrickHLA::CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SyncPointList();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor. */
   SyncPointList();

   /*! @brief Constructor for the TrickHLA SyncPointList class. */
   SyncPointList( std::string const &name,
                  MutexLock         &mtx,
                  Federate          *fed );

   /*! @brief Pure virtual destructor for the TrickHLA SyncPointList class. */
   virtual ~SyncPointList();

  public:
   void setup( Federate *fed );

   void set_list_name( std::string const &name );

   std::string &get_list_name();

   void set_mutex( MutexLock &mtx );

   void set_federate( Federate *fed );

   SyncPtStateEnum const get_state( std::wstring const &label );

   void clear();

   SyncPoint *get( std::wstring const &label ); // Search all lists for the unique sync-point label.

   bool const add( std::wstring const &label );

   bool const add( std::wstring const &label, Int64Time const &time );

   bool const contains( std::wstring const &label );

   bool const is_registered( std::wstring const &label );

   bool const mark_registered( std::wstring const &label );

   bool const register_sync_point( std::wstring const &label );

   bool const register_sync_point( std::wstring const &label, RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool const register_all();

   bool const register_all( RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool const register_sync_point( SyncPoint *sp );

   bool const register_sync_point( SyncPoint *sp, RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool const is_announced( std::wstring const &label );

   bool const mark_announced( std::wstring const &label, RTI1516_USERDATA const &user_supplied_tag );

   bool const wait_for_announced( std::wstring const &label );

   bool const wait_for_all_announced();

   bool const wait_for_announced( SyncPoint *sp );

   bool const is_achieved( std::wstring const &label );

   bool const achieve( std::wstring const &label );

   bool const achieve_all();

   bool const achieve_sync_point( SyncPoint *sp );

   bool const is_synchronized( std::wstring const &label );

   bool const mark_synchronized( std::wstring const &label );

   bool const wait_for_synchronized( std::wstring const &label );

   bool const wait_for_all_synchronized();

   bool const wait_for_synchronized( SyncPoint const *sp );

   std::string to_string();

   std::string to_string( std::wstring const &label );

   /*! @brief Encode the variables to a form Trick can checkpoint. */
   virtual void encode_checkpoint();

   /*! @brief Decode the state of this class from the Trick checkpoint. */
   virtual void decode_checkpoint();

   /*! @brief Free/release the memory used for the checkpoint data structures. */
   virtual void free_checkpoint();

  protected:
#if SYNC_POINT_TMM_ARRAY
   SyncPoint **list;       ///< @trick_units(--) Vector of sync-points objects.
   int         list_count; ///< @trick_units(--) Number of sync-points objects in the list.
#else
   SyncPointVector list; ///< @trick_io(**) Vector of sync-points objects.
#endif

   std::string list_name; ///< @trick_io(**) Name of this sync-point list.

   MutexLock *mutex; ///< @trick_io(**) Mutex to lock thread over critical code sections.

   Federate *federate; ///< @trick_units(--) Associated TrickHLA Federate.

   char *list_name_chkpt; ///< @trick_units(--) Trick checkpointable list name.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SyncPointList class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SyncPointList( SyncPointList const &rhs );
   /*! @brief Assignment operator for SyncPointList class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SyncPointList &operator=( SyncPointList const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_SYNC_POINT_LIST_HH */
