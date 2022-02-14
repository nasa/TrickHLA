/*!
@file TrickHLA/Attribute.hh
@ingroup TrickHLA
@brief This class represents the HLA attributes of an object that is managed
by Trick.

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
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Conditional.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_ATTRIBUTE_HH_
#define _TRICKHLA_ATTRIBUTE_HH_

// System includes.
#include <map>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/reference.h"

// TrickHLA include files.
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

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

class Attribute
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Attribute();

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below are configured by the user in the input files.
   //--------------------------------------------------------------------------
  public:
   char *trick_name; ///< @trick_units{--} Trick name for the attribute.
   char *FOM_name;   ///< @trick_units{--} FOM name for the attribute.

   DataUpdateEnum config; ///< @trick_units{--} The attribute configuration.

   TransportationEnum preferred_order; ///< @trick_units{--} Either Timestamp (default) or Receive Order.

   bool publish;   ///< @trick_units{--} True to publish attribute that is owned locally.
   bool subscribe; ///< @trick_units{--} True to subscribe to attribute.

   bool locally_owned; ///< @trick_units{--} Flag to indicate attribute is locally owned.

   EncodingEnum rti_encoding; ///< @trick_units{--} RTI encoding of the data.

   double cycle_time; ///< @trick_units{s} Send the cyclic attribute at the specified rate.

   // Added for conditional sending of this attribute
   Conditional *conditional; ///< @trick_units{--} Handler for a conditional attribute

   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Attribute class. */
   Attribute();
   /*! @brief Destructor for the TrickHLA Attribute class. */
   virtual ~Attribute();

   /*! @brief Initializes the TrickHLA Attribute.
    *  @param obj_FOM_name The FOM name of the parent object.
    *  @param object_index The array index to the parent Object.
    *  @param attribute_index The array index to this Attribute.
    */
   void initialize( const char *obj_FOM_name,
                    const int   object_index,
                    const int   attribute_index );

   /*! @brief Get the reflection rate configuration type.
    *  @return The reflection rate configuration type enumeration value. */
   const DataUpdateEnum get_configuration() const
   {
      return config;
   }

   /*! @brief Set the reflection rate configuration type. */
   void set_configuration( const DataUpdateEnum c )
   {
      this->config = c;
   }

   /*! @brief Determine the cycle-ratio given the core job cycle rate and the
    * cycle-time for this attribute.
    *  @param core_job_cycle_time Core job cycle time in seconds. */
   void determine_cycle_ratio( const double core_job_cycle_time );

   /*! @brief Pack the attribute into the buffer using the appropriate encoding. */
   void pack_attribute_buffer();

   /*! @brief Unpack the attribute from the buffer into the trick-variable
    *         using the appropriate decoding. */
   void unpack_attribute_buffer();

   /*! @brief Gets the encoded attribute value.
    *  @return The attribute value that contains the buffer of the encoded attribute. */
   RTI1516_NAMESPACE::VariableLengthData get_attribute_value();

   /*! @brief Extract the data out of the HLA Attribute Value.
    *  @param attr_value The variable length data buffer containing the attribute value. */
   void extract_data( RTI1516_NAMESPACE::VariableLengthData *attr_value );

   /*! @brief Determine if an attribute was received from another federate.
    *  @return True if new attribute value has been received. */
   bool is_received() const
   {
      return ( value_changed && !locally_owned );
   }

   /*! @brief Determine if an attribute value has changed.
    *  @return True if the attribute value is marked as changed. */
   bool is_changed() const
   {
      return this->value_changed;
   }

   /*! @brief Mark the attribute value as changed. */
   void mark_changed()
   {
      this->value_changed = true;
   }

   /*! @brief Mark the attribute value as unchanged. */
   void mark_unchanged()
   {
      this->value_changed = false;
   }

   /*! @brief Get the Federation Object Model attribute name.
    *  @return FOM name for the attribute. */
   const char *get_FOM_name() const
   {
      return FOM_name;
   }

   /*! @brief Get the associated Trick variable space name.
    *  @return The Trick variable space name associated with this attribute. */
   const char *get_trick_name() const
   {
      return trick_name;
   }

   /*! @brief Determine if the attribute is published.
    *  @return True if attribute is published. */
   bool is_publish() const
   {
      return publish;
   }

   /*! @brief Set the attribute publish flag.
    *  @param enable Flag to set the publish state. */
   void set_publish( bool enable )
   {
      this->publish = enable;
   }

   /*! @brief Determine if the attribute is subscribed.
    *  @return True if attribute is subscribed. */
   bool is_subscribe() const
   {
      return subscribe;
   }

   /*! @brief Set the attribute is subscribed flag.
    *  @param enable Flag to set the subscribe state. */
   void set_subscribe( bool enable )
   {
      this->subscribe = enable;
   }

   /*! @brief Determine if the attribute is locally owned.
    *  @return True is attribute is locally owned. */
   bool is_locally_owned() const
   {
      return locally_owned;
   }

   /*! @brief Mark the attribute is locally owned flag. */
   void mark_locally_owned()
   {
      if ( !locally_owned ) {
         this->locally_owned = true;
         // Reset the sub-rate count if we now own the attribute.
         this->cycle_cnt = 0;
      }
   }

   /*! @brief Mark the attribute is NOT locally owned flag. */
   void unmark_locally_owned()
   {
      this->locally_owned = false;
      // Reset the sub-rate count now that we don't own the attribute.
      this->cycle_cnt = 0;
   }

   /*! @brief Determine is the attribute is remotely owned.
    *  @return True is attribute is remotely owned. */
   bool is_remotely_owned() const
   {
      return ( !locally_owned );
   }

   /*! @brief Mark the attribute as remotely owned. */
   void mark_remotely_owned()
   {
      unmark_locally_owned();
   }

   /*! @brief Determine if someone is requesting an ownership transfer of this attribute.
    *  @return True if an ownership pull is requested. */
   bool is_pull_requested() const
   {
      return pull_requested;
   }

   /*! @brief Set the pull requested flag.
    *  @param enable Flag to set the pull requested state. */
   void set_pull_requested( const bool enable )
   {
      this->pull_requested = enable;
   }

   /*! @brief Determine if this federate is trying to push ownership of this attribute.
    *  @return True if an ownership push is requested. */
   bool is_push_requested() const
   {
      return push_requested;
   }

   /*! @brief Set the ownership push flag.
    *  @param enable Flag to set the push requested state. */
   void set_push_requested( const bool enable )
   {
      this->push_requested = enable;
   }

   /*! @brief Determine if this federate is requesting to divest ownership of this attribute.
    *  @return True if divest ownership is requested. */
   bool is_divest_requested() const
   {
      return divest_requested;
   }

   /*! @brief Set the ownership divest requested flag.
    *  @param enable Flag to set divest requested state. */
   void set_divest_requested( const bool enable )
   {
      this->divest_requested = enable;
   }

   /*! @brief Determine if byteswapping is required.
    *  @return True is byte swapping of the attribute date is required. */
   bool is_byteswap() const
   {
      return byteswap;
   }

   /*! @brief Determine is the data cycle is ready for sending data.
    *  @return True if the data cycle is ready for a send, false otherwise.*/
   bool is_data_cycle_ready() const
   {
      return ( ( cycle_ratio <= 1 ) || ( cycle_cnt <= 0 ) );
   }

   /*! @brief Determine is the data cycle is ready for sending data.
    *  @return True if the data cycle is ready for a send, false otherwise.*/
   const bool check_data_cycle_ready() // RETURN: -- True if the data cycle is ready for a send, false otherwise.
   {
      if ( ( cycle_ratio <= 1 ) || ( ( ++cycle_cnt ) >= cycle_ratio ) ) {
         cycle_cnt = 0;
         return true;
      }
      return false;
   }

   /*! @brief Set the preferred transportation order.
    *  @param order The transportation type enumeration value. */
   void set_preferred_order( const TransportationEnum order )
   {
      this->preferred_order = order;
   }

   /*! @brief Get the current preferred transportation order.
    *  @return The preferred transportation order enumeration value. */
   TransportationEnum get_preferred_order() const
   {
      return preferred_order;
   }

   /*! @brief Determine if an update is requested.
    *  @return True if an update is requested. */
   bool is_update_requested() const
   {
      return update_requested;
   }

   /*! @brief Set the attribute update requested flag.
    *  @param request_update Request update flag. */
   void set_update_requested( const bool request_update )
   {
      this->update_requested = request_update;
   }

   /*! @brief Get the RTI attribute handle.
    *  @return The RTI AttributeHandle associate with this attribute. */
   RTI1516_NAMESPACE::AttributeHandle get_attribute_handle() const
   {
      return this->attr_handle;
   }

   /*! @brief Set the RTI attribute handle.
    *  @param id The RTI attribute handle associated with this attribute. */
   void set_attribute_handle( RTI1516_NAMESPACE::AttributeHandle id )
   {
      this->attr_handle = id;
   }

   /*! @brief Get the Trick simulation variable associated with this attribute. */
   void *get_sim_variable_address()
   {
      // Address to a string is different so handle differently.
      if ( ( ref2->attr->type == TRICK_STRING )
           || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( ref2->attr->num_index > 0 )
                && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
         return ( *( (void **)( ref2->address ) ) );
      } else {
         return ( ref2->address );
      }
   }

   /*! @brief Prints the contents of buffer used to encode/decode the attribute
    *         to the console on standard out. */
   void print_buffer() const;

   /*! @brief Check if attribute is sent conditionally.
    *  @return True if attribute is to be sent conditionally. */
   bool has_conditional() const
   {
      return ( conditional != NULL );
   }

   /*! @brief Get the associated conditionality handler.
    *  @return The conditionality handler class associated with this attribute. */
   Conditional *get_conditional()
   {
      return conditional;
   }

   /*! @brief Get the Trick "Ref Attributes" associated with this attribute.
    *  @return A pointer to the Trick "Ref Attributes" class. */
   ATTRIBUTES get_ref2_attributes() const
   {
      return ( *( ref2->attr ) );
   }

   /*! @brief Get the RTI encoding for this attribute.
    *  @return The RTI encoding type enumeration value. */
   EncodingEnum get_rti_encoding() const
   {
      return rti_encoding;
   }

   // Set the RTI encoding and based on the new encoding determine if we
   // need to byte-swap.
   void set_encoding( const EncodingEnum in_type )
   {
      rti_encoding = in_type;

      // Determine if we need to do a byteswap for data transmission.
      byteswap = Utilities::is_transmission_byteswap( rti_encoding );
   }

   /*! @brief Determines if the attribute is static in size.
    *  @return True if attribute size is static. */
   bool is_static_in_size() const;

   /*! @brief Calculate the number of items in the attribute.
    *  @return Number of items in the attribute. */
   const size_t calculate_number_of_items()
   {
      calculate_size_and_number_of_items();
      return num_items;
   }

   /*! @brief Gets the attribute size in bytes.
    *  @return The size in bytes of the attribute. */
   size_t get_attribute_size();

  private:
   /*! @brief Calculates the attribute size in bytes and the number of items it contains. */
   void calculate_size_and_number_of_items();

   /*! @brief Calculates the number of items contained by the attribute. */
   void calculate_static_number_of_items();

   /*! @brief Ensure the attribute buffer has at least the specified capacity.
    *  @param capacity Desired capacity of the buffer in bytes. */
   void ensure_buffer_capacity( size_t capacity );

   /*! @brief Determines if the HLA object attribute type is supported given
    *         the RTI encoding.
    *  @return True if supported, false otherwise. */
   bool is_supported_attribute_type() const;

   /*! @brief Encode a boolean attribute into the buffer using the HLAboolean
    * data type which is encoded as a HLAinteger32BE. */
   void encode_boolean_to_buffer();

   /*! @brief Decode a boolean attribute from the buffer using the HLAboolean
    * data type which is encoded as a HLAinteger32BE. */
   void decode_boolean_from_buffer() const;

   /*! @brief Encode the object attribute using the HLAlogicalTime 64-bit
    * integer encoding. */
   void encode_logical_time() const;

   /*! @brief Decode the object attribute that is using the HLAlogicalTime
    * 64-bit integer encoding. */
   void decode_logical_time();

   /*! @brief Encode the data as HLA opaque data into the buffer. */
   void encode_opaque_data_to_buffer();

   /*! @brief Decode the opaque data in the buffer. */
   void decode_opaque_data_from_buffer();

   /*! @brief Encode a string attribute into the buffer using the appropriate
    * encoding. */
   void encode_string_to_buffer();

   /*! @brief Decode a string from the buffer into the attribute using the
    * appropriate decoding. */
   void decode_string_from_buffer();

   /*! @brief Copy the data from the source to the destination and byteswap as
    * needed.
    *  @param dest      Destination to copy data to.
    *  @param src       Source of the data to byteswap and copy from.
    *  @param type      The type of the data.
    *  @param length    The length/number of entries in the source array.
    *  @param num_bytes The number of bytes in the source array.
    *  */
   void byteswap_buffer_copy( void * dest,
                              void * src,
                              int    type,
                              size_t length,
                              size_t num_bytes ) const;

   unsigned char *buffer;          ///< @trick_units{--} Byte buffer for the attribute value bytes.
   size_t         buffer_capacity; ///< @trick_units{count} The capacity of the buffer.

   bool size_is_static; ///< @trick_units{--} Flag to indicate the size of this attribute is static.

   size_t size;      ///< @trick_units{count} The size of the attribute in bytes.
   size_t num_items; ///< @trick_units{count} Number of attribute items, length of the array.

   bool value_changed; ///< @trick_units{--} Flag to indicate the attribute value changed.

   bool update_requested; ///< @trick_units{--} Flag to indicate another federate has requested an attribute update.

   unsigned int HLAtrue; ///< @trick_units{--} A 32-bit integer with a value of 1 on a Big Endian computer.

   bool byteswap; ///< @trick_units{--} Flag to indicate byte-swap before RTI Rx/Tx.

   int cycle_ratio; ///< @trick_units{--} Ratio of the attribute cycle-time to the send_cyclic_and_requested_data job cycle time.
   int cycle_cnt;   ///< @trick_units{count} Internal cycle counter used to determine when cyclic data will be sent.

   REF2 *ref2; ///< @trick_io{**} The ref_attributes of the given trick_name.

   RTI1516_NAMESPACE::AttributeHandle attr_handle; ///< @trick_io{**} The RTI attribute handle.

   bool pull_requested;   ///< @trick_units{--} Has someone asked to own us?
   bool push_requested;   ///< @trick_units{--} Is someone giving up ownership?
   bool divest_requested; ///< @trick_units{--} Are we releasing ownership?

   bool initialized; ///< @trick_units{--} Has this attribute been initialized?

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Attribute class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Attribute( const Attribute &rhs );
   /*! @brief Assignment operator for Attribute class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Attribute &operator=( const Attribute &rhs );
};

typedef std::map< RTI1516_NAMESPACE::AttributeHandle, Attribute * > AttributeMap; ///< @trick_io{**} Map of attributes.

} // namespace TrickHLA

#endif // _TRICKHLA_ATTRIBUTE_HH_
