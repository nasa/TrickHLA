/*!
@file IMSim/SyncPntListBase.hh
@ingroup IMSim
@brief This class provides and abstract base class as the base implementation
for storing and managing HLA synchronization points for Trick.

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

@tldh
@trick_link_dependency{../../source/IMSim/SyncPntListBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPoint.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef IMSIM_SYNC_PNT_LIST_BASE_HH
#define IMSIM_SYNC_PNT_LIST_BASE_HH

// System includes.
#include <string>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPoint.hh"

// IMSim include files.
#include "IMSim/SyncPntLoggable.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

namespace IMSim
{

class SyncPntListBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrIMSim__SyncPntListBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SyncPntListBase class. */
   SyncPntListBase();

   /*! @brief Pure virtual destructor for the TrickHLA SyncPntListBase class. */
   virtual ~SyncPntListBase() = 0;

  public:
   /*! @brief Add the given synchronization point label to the list.
    *  @param label Synchronization point label.
    *  @return The synchronization point associated with the label. */
   virtual TrickHLA::SyncPoint *add_sync_point( std::wstring const &label );

   /*! @brief Get the pointer to the synchronization point associated with the
    * given label.
    *  @return The synchronization point associated with the label. */
   virtual TrickHLA::SyncPoint *get_sync_point( std::wstring const &label );

   /*! @brief Register a synchronization point associated with the given label.
    *  @return The newly created synchronization point.
    *  @param RTI_amb Reference to RTI Ambassador.
    *  @param label  Synchronization point label. */
   virtual TrickHLA::SyncPoint *register_sync_point( RTI1516_NAMESPACE::RTIambassador &RTI_amb,
                                                     std::wstring const               &label );

   /*! @brief Register a synchronization point associated with the given label.
    *  @return The newly created synchronization point.
    *  @param RTI_amb              Reference to RTI Ambassador.
    *  @param label               Synchronization point label.
    *  @param federate_handle_set Federate handle set. */
   virtual TrickHLA::SyncPoint *register_sync_point(
      RTI1516_NAMESPACE::RTIambassador           &RTI_amb,
      RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set,
      std::wstring const                         &label );

   /*! @brief Register all the synchronization point in this list.
    *  @param RTI_amb Reference to RTI Ambassador. */
   virtual void register_all_sync_points( RTI1516_NAMESPACE::RTIambassador &RTI_amb );

   /*! @brief Register all the sync-points with the RTI for the given set of federate handles.
    *  @param RTI_amb             Reference to the HLA RTI Ambassador instance.
    *  @param federate_handle_set Federate Handle set. */
   virtual void register_all_sync_points(
      RTI1516_NAMESPACE::RTIambassador           &RTI_amb,
      RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set );

   /*! @brief Callback from TrickHLA::FedAmb through TrickHLA::Federate for
    *  when registration of a synchronization point success.
    *  and is one of the sync-points created.
    *  @param label      Sync-point label. */
   virtual void sync_point_registration_succeeded( std::wstring const &label );

   /*! @brief Callback from TrickHLA::FedAmb through TrickHLA::Federate for
    *  when registration of a synchronization point fails.
    *  and is one of the sync-points created.
    *  @param label      Sync-point label.
    *  @param not_unique True if not unique label. */
   virtual void sync_point_registration_failed( std::wstring const &label,
                                                bool const          not_unique );

   /*! @brief Wait for all the sync-points to be registered with the RTI.
    *  @param fed_ptr Pointer to TrickHLA::Federate instance. */
   virtual void wait_for_all_announcements( TrickHLA::Federate *fed_ptr );

   /*! @brief The RTI has announced the existence of a synchronization point.
    *  @param RTI_amb    Reference to the HLA RTI Ambassador instance.
    *  @param label             Sync-point label.
    *  @param user_supplied_tag Use supplied tag.*/
   virtual void announce_sync_point(
      RTI1516_NAMESPACE::RTIambassador &RTI_amb,
      std::wstring const               &label,
      RTI1516_USERDATA const           &user_supplied_tag );

   /*! @brief Achieve all the synchronization points in the list.
    *  @return True is any synchronization point in the list was achieved.
    *  @param RTI_amb Reference to the HLA RTI Ambassador instance. */
   virtual bool achieve_all_sync_points( RTI1516_NAMESPACE::RTIambassador &RTI_amb );

   /*! @brief Wait for all the synchronization points in the list to be
    *  achieved.
    *  @param federate Federate associated with this list. */
   virtual void wait_for_list_synchronization( TrickHLA::Federate *federate );

   /*! @brief Achieve the synchronization point associated with this label.
    *  @return True if achieved, false otherwise.
    *  @param RTI_amb Reference to RTI Ambassador.
    *  @param label  Synchronization point label. */
   virtual bool achieve_sync_point( RTI1516_NAMESPACE::RTIambassador &RTI_amb,
                                    std::wstring const               &label );

   /*! @brief Achieve the given synchronization point.
    *  @return True if achieved, false otherwise.
    *  @param RTI_amb Reference to RTI Ambassador.
    *  @param sync_pnt       Specified synchronization point. */
   virtual bool achieve_sync_point( RTI1516_NAMESPACE::RTIambassador &RTI_amb,
                                    TrickHLA::SyncPoint              *sync_pnt );

   /*! @brief Achieve the specified sync-point and wait for the federation to
    *  be synchronized on it.
    *  @param RTI_amb Reference to RTI Ambassador.
    *  @param federate       Associated federate.
    *  @param label          Synchronization point label. */
   virtual void achieve_and_wait_for_synchronization(
      RTI1516_NAMESPACE::RTIambassador &RTI_amb,
      TrickHLA::Federate               *federate,
      std::wstring const               &label );

   /*! @brief Determine if the synchronization point is known to be in the list
    * of known synchronization points.
    *  @return True if the label is a known synchronization point.
    *  @param label The synchronization point label. */
   virtual bool contains( std::wstring const &label );

   /*! @brief Mark the given synchronization point as registered in the federation.
    *  @return True if synchronization point label is valid.
    *  @param label The synchronization point label. */
   virtual bool mark_registered( std::wstring const &label );

   /*! @brief Mark the given synchronization point as existing in the federation.
    *  @return True if synchronization point label is valid.
    *  @param label The synchronization point label. */
   virtual bool mark_announced( std::wstring const &label );

   /*! @brief Mark the given synchronization point as synchronized in the federation.
    *  @return True if synchronization point label is valid.
    *  @param label The synchronization point label. */
   virtual bool mark_synchronized( std::wstring const &label );

   /*! @brief Get the state of the given synchronization point label.
    *  @return Synchronization point state.
    *  @param label The synchronization point label. */
   virtual TrickHLA::SyncPtStateEnum get_sync_point_state( std::wstring const &label );

   /*! @brief Check if specified synchronization point has been announced.
    *  @return True if announced.
    *  @param label The synchronization point label. */
   virtual bool is_sync_point_announced( std::wstring const &label );

   /*! @brief Clear the given synchronization point label.
    *  @return True if synchronization point is cleared.
    *  @param label The synchronization point label. */
   virtual bool clear_sync_point( std::wstring const &label );

   /*! @brief Reset the list of synchronization points. */
   virtual void reset();

   /*! @brief Get the number of synchronization points in the list.
    *  @return The number of synchronization points in the list. */
   virtual size_t get_size() const
   {
      return sync_point_list.size();
   }

   /*! @brief
    *  @return  */
   virtual std::wstring const get_reconfig_name()
   {
      return reconfig_name;
   }

   /*! @brief Returns a wide string representing the state of the
    *  synchronization points.
    *  @return String summary of synchronization points. */
   virtual std::string to_string();

   /*! @brief Converts the vector of synchronization points to a
    *  checkpoint-able class.
    *  @param sync_points Area to populate. */
   virtual void convert_sync_points( SyncPntLoggable *sync_points );

   /*! @brief Dumps synchronization point information to the screen. */
   virtual void print_sync_points();

  protected:
   // Principal synchronization point functions.
   /*! @brief Register the synchronization point with the RTI.
    *  @param RTI_amb HLA RTI Ambassador.
    *  @param sync_pnt The SyncPoint instance. */
   virtual void register_sync_point( RTI1516_NAMESPACE::RTIambassador &RTI_amb,
                                     TrickHLA::SyncPoint              *sync_pnt );

   /*! @brief Register the synchronization point with the RTI.
    *  @param RTI_amb The HLA RTI Ambassador.
    *  @param federate_handle_set HLA Federation handle set.
    *  @param sync_pnt The SyncPoint instance. */
   virtual void register_sync_point(
      RTI1516_NAMESPACE::RTIambassador           &RTI_amb,
      RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set,
      TrickHLA::SyncPoint                        *sync_pnt );

   /*! @brief Wait for the announcement of the synchronization point.
    *  @param federate Pointer to TrickHLA::Federate instance.
    *  @param label    Sync-point label. */
   virtual bool wait_for_sync_point_announcement( TrickHLA::Federate *federate,
                                                  std::wstring const &label );

   /*! @brief Wait for the announcement of the synchronization point.
    *  @param federate The TrickHLA::Federate instance.
    *  @param sync_pnt The SyncPoint instance. */
   virtual bool wait_for_sync_point_announcement( TrickHLA::Federate  *federate,
                                                  TrickHLA::SyncPoint *sync_pnt );

   /*! @brief Wait for this synchronization point to be synchronized.
    *  @param federate The TrickHLA::Federate instance.
    *  @param sync_pnt The SyncPoint instance. */
   virtual bool wait_for_synchronization( TrickHLA::Federate  *federate,
                                          TrickHLA::SyncPoint *sync_pnt );

  protected:
   TrickHLA::MutexLock mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

   std::vector< TrickHLA::SyncPoint * > sync_point_list; ///< @trick_io{**} Vector of synchronization points.

   std::wstring reconfig_name; ///< @trick_io{**} Wide string of the reconfiguration name.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SyncPntListBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SyncPntListBase( SyncPntListBase const &rhs );
   /*! @brief Assignment operator for SyncPntListBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SyncPntListBase &operator=( SyncPntListBase const &rhs );
};

} // namespace IMSim

#endif /* IMSIM_SYNC_PNT_LIST_BASE_HH */
