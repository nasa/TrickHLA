/*!
@file TrickHLA/Interaction.hh
@ingroup TrickHLA
@brief This class represents an HLA Interaction that is managed by Trick.

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
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Interval.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/Interaction.cpp}
@trick_link_dependency{../../source/TrickHLA/InteractionItem.cpp}
@trick_link_dependency{../../source/TrickHLA/InteractionHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/Parameter.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Aug 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_INTERACTION_HH
#define TRICKHLA_INTERACTION_HH

// System include files.

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"

// TrickHLA include files
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

// Special handling of SWIG limitations for forward declarations.
#ifdef SWIG
#   include "TrickHLA/Parameter.hh"
#else
namespace TrickHLA
{
// NOTE: This forward declaration of TrickHLA::Parameter is here to go with
// the #ifdef SWIG include. Normally, it would go with the other forward
// declarations below.
class Parameter;
} // namespace TrickHLA
#endif // SWIG

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class Manager;
class InteractionItem;
class InteractionHandler;

class Interaction
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Interaction();

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below are configured by the user in the input files.
   //--------------------------------------------------------------------------
  public:
   char *FOM_name; ///< @trick_units{--} FOM name for the interaction.

   bool publish;   ///< @trick_units{--} True to publish interaction.
   bool subscribe; ///< @trick_units{--} True to subscribe to interaction.

   TransportationEnum preferred_order; ///< @trick_units{--} Either Timestamp (default) or Receive Order.

   int        param_count; ///< @trick_units{--} Number of interaction parameters.
   Parameter *parameters;  ///< @trick_units{--} Array of interaction parameters.

   InteractionHandler *handler; ///< @trick_units{--} Interaction handler.

   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Interaction class. */
   Interaction();
   /*! @brief Destructor for the TrickHLA Interaction class. */
   virtual ~Interaction();

  public:
   //
   // Post-constructor initialization stuff
   //
   /*! @brief Initializes the TrickHLA Interaction class.
    *  @param trickhla_mgr Pointer to the associated TrickHLA::Manager class. */
   void initialize( Manager *trickhla_mgr );

   //
   // RTI
   //
   /*! @brief Publishes the interaction to the RTI. */
   void publish_interaction();

   /*! @brief Unpublish the Interaction. */
   void unpublish_interaction();

   /*! @brief Subscribes to the Interaction. */
   void subscribe_to_interaction();

   /*! @brief Unsubscribes from the Interaction. */
   void unsubscribe_from_interaction();

   /*! @brief Setup the interaction preferred order with the RTI. */
   void setup_preferred_order_with_RTI();

   /*! @brief Will unpublish or unsubscribe the interaction. */
   void remove();

   /*! @brief Sends the interaction to the RTI using Receive Order.
    *  @return True if interaction was sent; False otherwise.
    *  @param the_user_supplied_tag Users tag. */
   bool send( RTI1516_USERDATA const &the_user_supplied_tag );

   /*! @brief ends the interaction to the RTI using Timestamp Order.
    *  @return True if interaction was sent; False otherwise.
    *  @param send_HLA_time The HLA logical time the user wants to send the interaction.
    *  @param the_user_supplied_tag Users tag. */
   bool send( double                  send_HLA_time,
              RTI1516_USERDATA const &the_user_supplied_tag );

   /*! @brief Process the interaction by decoding the parameter data into the
    * users simulation variables and calling the users interaction-handler. */
   void process_interaction();

   /*! @brief Extracts the parameters for the received Interaction.
    *  @param interaction_item Interaction item.
    *  @return True if successfull extracted data, false otherwise. */
   bool extract_data( InteractionItem *interaction_item );

   // Instance methods
   /*! @brief Get the FOM name for this interaction.
    *  @return Constant string with the FOM name for this interaction. */
   char const *get_FOM_name() const
   {
      return FOM_name;
   }

   /*! @brief Query if this interaction is published.
    *  @return True if this interaction is published; False otherwise. */
   bool is_publish() const
   {
      return publish;
   }

   /*! @brief Query if this interaction is subscribed.
    *  @return True if this interaction is subscribed; False otherwise. */
   bool is_subscribe() const
   {
      return subscribe;
   }

   /*! @brief Get this interactions InteractionClassHandle.
    *  @return Copy of this interactions InteractionClassHandle. */
   RTI1516_NAMESPACE::InteractionClassHandle get_class_handle() const
   {
      return class_handle;
   }

   /*! @brief Set the interaction InteractionClassHandle.
    *  @param id The interaction InteractionClassHandle. */
   void set_class_handle( RTI1516_NAMESPACE::InteractionClassHandle const &id )
   {
      this->class_handle = id;
   }

   /*! @brief Get the parameter count for this interaction.
    *  @return The parameter count for this interaction. */
   int get_parameter_count() const
   {
      return param_count;
   }

   /*! @brief Get the TrickHLA::Parameter array associated with this interaction.
    *  @return The Parameter array. */
   Parameter *get_parameters()
   {
      return parameters;
   }

   // Used by TrickHLA to determine if the interaction data changed.
   /*! @brief Query if the interaction data has changed.
    *  @return True if data has changed; False otherwise. */
   bool is_changed() const
   {
      return this->changed;
   }

   /*! @brief Mark the data as cahnged. */
   void mark_changed()
   {
      this->changed = true;
   }

   /*! @brief Mark the data as unchanged, and clear the change flag for all the parameters as well. */
   void mark_unchanged();

   //
   // These are needed for non-input processor access to private data!
   // TODO: Review this to make sure we still need this.
   /*! @brief Get the TrickHLA::InteractionHandler associated with this interaction.
    *  @return The interaction handler. */
   InteractionHandler *get_handler()
   {
      return handler;
   }
   /*! @brief Set the TrickHLA::InteractionHandler for this interaction.
    *  @param ptr The TrickHLA::InteractionHandler instance to use. */
   void set_handler( InteractionHandler *ptr )
   {
      this->handler = ptr;
   }

   // needed so that my InteractionHandler can signal the Manager to do something...
   /*! @brief Get the associated TrickHLA::Manager instance.
    *  @return Pointer to the associated TrickHLA::Manager instance. */
   Manager *get_manager()
   {
      return manager;
   }

   /*! @brief Returns a pointer to our federate, or NULL if one does not exist yet.
    *  @return A pointer to this federate's TrickHLA::Federate instance. */
   Federate *get_federate();

   /*! @brief Returns a pointer to the RTI ambassador, or NULL if one does not exist yet.
    *  @return Pointer to this federate's associated RTIambassador instance. */
   RTI1516_NAMESPACE::RTIambassador *get_RTI_ambassador();

   /*! @brief Return a copy of the federate's lookahead time.
    *  @return A copy of the federate's lookahead time. */
   Int64Interval get_lookahead() const;

   /*! @brief Return a copy of the granted HLA logical time.
    *  @return A copy of the federation granted time. */
   Int64Time get_granted_time() const;

   /*! @brief Check if federate is shutdown function was called.
    *  @return True if the manager is shutting down the federate. */
   bool is_shutdown_called() const;

   /*! @brief Set the FOM name for this interaction.
    *  @param in_name The FOM name for this interaction. */
   void set_FOM_name( char const *in_name )
   {
      if ( this->FOM_name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->FOM_name ) ) ) {
            message_publish( MSG_WARNING, "Interaction::set_FOM_name():%d WARNING failed to delete Trick Memory for 'this->FOM_name'\n", __LINE__ );
         }
         this->FOM_name = NULL;
      }
      this->FOM_name = trick_MM->mm_strdup( in_name );
   }

   /*! @brief Set the received user supplied tag.
    *  @param tag The user supplied tag.
    *  @param tag_size Size of the user supplied tag. */
   void set_user_supplied_tag( unsigned char const *tag, int tag_size );

   /*! @brief Mark this interaction as published. */
   void set_publish()
   {
      publish = true;
   }

   /*! @brief Mark this interaction as subscribed. */
   void set_subscribe()
   {
      subscribe = true;
   }

   /*! @brief Set the interaction parameter count.
    *  @param in_num The number of parameters in this interaction. */
   void set_parameter_count( int in_num )
   {
      param_count = in_num;
   }

   /*! @brief Set the parameter array.
    *  @param ptr Pointer to the TrickHLA::Parameter array associated with this interaction. */
   void set_parameters( Parameter *ptr )
   {
      parameters = ptr;
   }

   /*! @brief Get the preferred transport order for this interaction.
    *  @return The preferred transport order for this interaction. */
   TransportationEnum get_preferred_order() const
   {
      return preferred_order;
   }

   MutexLock mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

  private:
   bool changed; ///< @trick_units{--} Flag indicating the data has changed.

   bool received_as_TSO; ///< @trick_units{--} True if received interaction as Timestamp order.

   Int64Time time; ///< @trick_units{--} Time used for Timestamp Order interaction.

   Manager                                  *manager;      ///< @trick_units{--} TrickHLA Manager.
   RTI1516_NAMESPACE::InteractionClassHandle class_handle; ///< @trick_io{**} RTI Interaction Class handle.

   int            user_supplied_tag_size;     ///< @trick_units{--} Number of bytes in the user supplied tag.
   int            user_supplied_tag_capacity; ///< @trick_units{--} Capacity of the user supplied tag.
   unsigned char *user_supplied_tag;          ///< @trick_units{--} User supplied tag data.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Interaction class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Interaction( Interaction const &rhs );
   /*! @brief Assignment operator for Interaction class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Interaction &operator=( Interaction const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_INTERACTION_HH
