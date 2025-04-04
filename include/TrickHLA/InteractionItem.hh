/*!
@file TrickHLA/InteractionItem.hh
@ingroup TrickHLA
@brief This class represents a queue for holding HLA Interactions of either
Timestamp Order (TSO) or Receive Order (RO).

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
@trick_link_dependency{../../source/TrickHLA/InteractionItem.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/Item.cpp}
@trick_link_dependency{../../source/TrickHLA/ItemQueue.cpp}
@trick_link_dependency{../../source/TrickHLA/Parameter.cpp}
@trick_link_dependency{../../source/TrickHLA/ParameterItem.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2007, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_INTERACTION_ITEM_HH
#define TRICKHLA_INTERACTION_ITEM_HH

// System include files.

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Item.hh"
#include "TrickHLA/ItemQueue.hh"
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

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Parameter;
class ParameterItem;

class InteractionItem : public Item
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__InteractionItem();

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below are configured by the user in the input files.
   //--------------------------------------------------------------------------
  public:
   int index; ///< @trick_units{--} Index to the applicable Interaction.

   ItemQueue parameter_queue; ///< @trick_io{**} Linked list queue of parameter items.

   InteractionTypeEnum interaction_type; ///< @trick_units{--} type of the containing interaction

   int            parm_items_count; ///< @trick_units{--} Number of array elements
   ParameterItem *parm_items;       ///< @trick_units{--} checkpoint-able parameter items array

   int            user_supplied_tag_size; ///< @trick_units{--} Number of bytes in the user supplied tag.
   unsigned char *user_supplied_tag;      ///< @trick_units{--} User supplied tag data.

   bool      order_is_TSO; ///< @trick_units{--} True if Timestamp Order, false for Receive Order.
   Int64Time time;         ///< @trick_units{--} Time associated with TSO interaction.

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA InteractionItem class. */
   InteractionItem();

   /*! @brief Initialization constructor for the TrickHLA InteractionItem class.
    *  @param inter_index        Interaction index.
    *  @param inter_type         Type of the containing interaction.
    *  @param param_count        Number of parameters.
    *  @param parameters         Interaction Parameters.
    *  @param theParameterValues Parameter values.
    *  @param theUserSuppliedTag User supplied tag. */
   InteractionItem( int const                                         inter_index,
                    InteractionTypeEnum const                         inter_type,
                    int const                                         param_count,
                    Parameter                                        *parameters,
                    RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
                    RTI1516_USERDATA const                           &theUserSuppliedTag );

   /*! @brief Initialization constructor for the TrickHLA InteractionItem class.
    *  @param inter_index        Interaction index.
    *  @param inter_type         Type of the containing interaction.
    *  @param param_count        Number of parameters.
    *  @param parameters         Interaction Parameters.
    *  @param theParameterValues Parameter values.
    *  @param theUserSuppliedTag User supplied tag.
    *  @param theTime            Time for TSO interaction. */
   InteractionItem( int const                                         inter_index,
                    InteractionTypeEnum const                         inter_type,
                    int const                                         param_count,
                    Parameter                                        *parameters,
                    RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
                    RTI1516_USERDATA const                           &theUserSuppliedTag,
                    RTI1516_NAMESPACE::LogicalTime const             &theTime );

   /*! @brief Destructor for the TrickHLA InteractionItem class. */
   virtual ~InteractionItem();

  public:
   /*! @brief Decode all the parameter_queue values into parm_items linear array. */
   void checkpoint_queue();

   /*! @brief removes all the parm_items values. */
   void clear_parm_items();

   /*! @brief Encode all the parm_items values into this InteractionItem. */
   void restore_queue();

   /*! @brief Query if this InteractionItem is sent TimeStamp Order (TSO).
    *  @return True if sent TimeStamp Order; False otherwise. */
   bool is_timestamp_order() const
   {
      return ( order_is_TSO );
   }

   /*! @brief Query if this InteractionItem is sent Receive Order (RO).
    *  @return True if sent Receive Order; False otherwise. */
   bool is_receive_order() const
   {
      return ( !order_is_TSO );
   }

  private:
   /*! @brief Decode the Interaction values into this Item.
    *  @param inter_type         Type of the containing interaction.
    *  @param param_count        Number of parameters.
    *  @param parameters         Interaction Parameters.
    *  @param theParameterValues Parameter values.
    *  @param theUserSuppliedTag User supplied tag. */
   void initialize( InteractionTypeEnum const                         inter_type,
                    int const                                         param_count,
                    Parameter                                        *parameters,
                    RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
                    RTI1516_USERDATA const                           &theUserSuppliedTag );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for InteractionItem class.
    *  @details This constructor is private to prevent inadvertent copies. */
   InteractionItem( InteractionItem const &rhs );
   /*! @brief Assignment operator for InteractionItem class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   InteractionItem &operator=( InteractionItem const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_INTERACTION_ITEM_HH
