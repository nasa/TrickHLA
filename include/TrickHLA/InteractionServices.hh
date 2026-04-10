/*!
@file TrickHLA/InteractionServices.hh
@ingroup TrickHLA
@brief This class manages the HLA Interaction Services.

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
@trick_link_dependency{../../source/TrickHLA/InteractionServices.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/ItemQueue.cpp}
@trick_link_dependency{../../source/TrickHLA/Interaction.cpp}
@trick_link_dependency{../../source/TrickHLA/InteractionItem.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/utils/MutexLock.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2026, --, Refactored from old Manager class.}
@revs_end

*/

#ifndef TRICKHLA_INTERACTION_SERVICES_HH
#define TRICKHLA_INTERACTION_SERVICES_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/ItemQueue.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/utils/MutexLock.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

// Special handling of SWIG limitations for forward declarations.
#ifdef SWIG
#   include "TrickHLA/Interaction.hh"
#else
namespace TrickHLA
{
// NOTE: This forward declaration of TrickHLA::Interaction and TrickHLA::Object
// are here to go with the #ifdef SWIG include. Normally, it would go with the
// other forward declarations below.
class Federate;
} // namespace TrickHLA
#endif // SWIG

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Interaction;
class InteractionItem;

class InteractionServices : public CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__InteractionServices();

   // Needs to call some of InteractionServices's protexted and private data.
   friend class Federate;

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below this point are configured by the user in either the
   // input or modified-data files.
  public:
   int          inter_count;  ///< @trick_units{--} Number of TrickHLA Interactions.
   Interaction *interactions; ///< @trick_units{--} Array of TrickHLA Interactions.

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA InteractionServices class.
    *  @param fed Associated Federate instance. */
   explicit InteractionServices( Federate &fed );
   /*! @brief Destructor for the TrickHLA InteractionServices class. */
   virtual ~InteractionServices();
   /*! @brief Perform initialization after a checkpoint restart. */
   void restart_initialization();

   /*! @brief Verify the user specified interaction arrays and counts. */
   void verify_interaction_arrays();

   // Interactions
   /*! @brief Process the received interactions. */
   void process_interactions();

   /*! @brief Process all received interactions by calling in turn each
    * interaction handler that is subscribed to the interaction.
    * @param theInteraction     Interaction handle.
    * @param theParameterValues Parameter values.
    * @param theUserSuppliedTag Users tag.
    * @param theTime            HLA time for the interaction.
    * @param received_as_TSO    True if interaction was received by RTI as TSO. */
   void receive_interaction(
      RTI1516_NAMESPACE::InteractionClassHandle const  &theInteraction,
      RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
      RTI1516_NAMESPACE::VariableLengthData const      &theUserSuppliedTag,
      RTI1516_NAMESPACE::LogicalTime const             &theTime,
      bool const                                        received_as_TSO );

   /*! @brief Publishes Object & Interaction classes and their member data. */
   void publish();

   /*! @brief Unpublish the Object & Interaction classes. */
   void unpublish();

   /*! @brief Subscribe to Object and Interaction classes and their member data. */
   void subscribe();

   /*! @brief Unubscribe from the Object and Interaction classes. */
   void unsubscribe();

   /*! @brief Sets the RTI run-time type IDs/handles for the interaction and parameters. */
   void setup_interaction_RTI_handles();

   /*! @brief Sets the RTI run-time type IDs/handles for the specified
    * interactions and parameters.
    *  @param interactions_counter Number of interactions.
    *  @param in_interactions      Simulation TrickHLA Interaction objects. */
   void setup_interaction_RTI_handles( int const    interactions_counter,
                                       Interaction *in_interactions );

   /*! @brief Setup the preferred order (TSO or RO) for all the object
    * attributes and interactions. */
   void setup_preferred_order_with_RTI();

   /*! @brief Get the number of TrickHLA::Interactions.
    *  @return The number of TrickHLA::Interaction instances. */
   int get_interaction_count() const
   {
      return inter_count;
   }

   /*! @brief Get the array containing the TrickHLA::Interaction instances.
    *  @return Array of TrickHLA::Interaction instances. */
   Interaction *get_interactions()
   {
      return interactions;
   }

   /*! @brief Set up the Trick ref-attributes for the user specified
    * interactions and parameters. */
   void setup_interaction_ref_attributes();

   //
   // CheckpointConversionBase Interface.
   //
   /*! @brief Encode/setup the checkpoint data structures. */
   virtual void convert_data_before_checkpoint();

   /*! @brief Restore the state of this class from the Trick checkpoint. */
   virtual void restore_data_after_checkpoint();

   /*! @brief Clear/release the memory used for the checkpoint data structures. */
   virtual void free_converted_data_for_checkpoint();

   /*! @brief Echoes the contents of checkpoint InteractionItem linear array. */
   void print_converted_checkpoint();

   //
   // Private data.
   //
  protected:
   ItemQueue interactions_queue; ///< @trick_io{**} Interactions queue.

   std::size_t      check_interactions_count; ///< @trick_units{--} Number of checkpointed interactions
   InteractionItem *check_interactions;       ///< @trick_units{--} checkpoint-able version of interactions_queue

   //
   // References to the Federate and associated services.
   //
   Federate *federate; ///< @trick_units{--} Associated TrickHLA::Federate.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for InteractionServices class.
    *  @details This constructor is private to prevent inadvertent copies. */
   InteractionServices( InteractionServices const &rhs );
   /*! @brief Assignment operator for InteractionServices class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   InteractionServices &operator=( InteractionServices const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_INTERACTION_SERVICES_HH
