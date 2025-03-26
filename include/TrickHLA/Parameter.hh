/*!
@file TrickHLA/Parameter.hh
@ingroup TrickHLA
@brief This class represents the HLA parameters of an interaction that is
managed by Trick.

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
@trick_link_dependency{../../source/TrickHLA/Parameter.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Aug 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_PARAMETER_HH
#define TRICKHLA_PARAMETER_HH

// System include files.

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/attributes.h"
#include "trick/message_proto.h"

// TrickHLA include files.
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

class Parameter
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Parameter();

   //----------------------------- USER VARIABLES -----------------------------
   // The variables in this section are configured by the user in either the
   // input or modified-data file.
  public:
   char *trick_name; ///< @trick_units{--} Trick name for the attribute.
   char *FOM_name;   ///< @trick_units{--} FOM name for the attribute

   EncodingEnum rti_encoding; ///< @trick_units{--} RTI encoding of the data.

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Parameter class. */
   Parameter();

   /*! @brief Destructor for the TrickHLA Parameter class. */
   virtual ~Parameter();

   /*! @brief Initializes the TrickHLA Parameter from the trick variable name
    *  supplied in the input file.
    *  @param interaction_fom_name The FOM name of the parent interaction.
    *  @param interaction_index The array index to the parent TrickHLA::Interaction.
    *  @param parameter_index The array index to this Parameter.
    */
   void initialize( char const *interaction_fom_name,
                    int const   interaction_index,
                    int const   parameter_index );

   /*! @brief Initializes the TrickHLA Parameter from the supplied address and
    * ATTRIBUTES of the trick variable.
    *  @param interaction_fom_name FOM name of the interaction.
    *  @param in_addr Address of the trick variable.
    *  @param in_attr ATTRIBUTES of the trick variable.
    */
   void initialize( char const *interaction_fom_name,
                    void       *in_addr,
                    ATTRIBUTES *in_attr );

   /*! @brief Initializes the TrickHLA Parameter. */
   void complete_initialization();

   /*! @brief Get the FOM name for this parameter.
    *  @return The FOM name of this parameter. */
   char const *get_FOM_name() const
   {
      return FOM_name;
   }

   /*! @brief Set the FOM name for the parameter.
    *  @param in_name The FOM name for the parameter. */
   void set_FOM_name( char const *in_name )
   {
      if ( this->FOM_name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->FOM_name ) ) ) {
            message_publish( MSG_WARNING, "Parameter::set_FOM_name():%d WARNING failed to delete Trick Memory for 'this->FOM_name'\n",
                             __LINE__ );
         }
      }
      this->FOM_name = trick_MM->mm_strdup( in_name );
   }

   /*! @brief Get the Trick variable name associated with this parameter.
    *  @return The Trick variable name associated with this parameter. */
   char const *get_trick_name() const
   {
      return trick_name;
   }

   /*! @brief Set the RTI encoding and based on the new encoding determine if
    * we need to byte-swap.
    *  @param in_type The encoding type for this parameter. */
   void set_encoding( EncodingEnum in_type )
   {
      rti_encoding = in_type;

      // Determine if we need to do a byteswap for data transmission.
      byteswap = Utilities::is_transmission_byteswap( rti_encoding );
   }

   /*! @brief Calculate the number of attribute items associated with this parameter.
    *  @return The number of attribute items associated with this parameter. */
   int calculate_number_of_items()
   {
      calculate_size_and_number_of_items();
      return num_items;
   }

   /*! @brief Determines if the parameter is static in size.
    *  @return True if parameter size is static. */
   bool is_static_in_size() const;

   /*! @brief Determines if the parameter has to be byte swapped.
    *  @return True if parameter is byte swapped. */
   bool is_byteswap() const
   {
      return byteswap;
   }

   /*! @brief Get the HLA Parameter handle.
    *  @return The HLA Parameter handle. */
   RTI1516_NAMESPACE::ParameterHandle get_parameter_handle() const
   {
      return this->param_handle;
   }

   /*! @brief Set the associated HLA Parameter handle.
    *  @param hdl The associated HLA Parameter handle. */
   void set_parameter_handle( RTI1516_NAMESPACE::ParameterHandle const hdl )
   {
      this->param_handle = hdl;
   }

   /*! @brief Gets the HLA Parameter Value using the appropriate encoding.
    *  @return Encoded parameter value. */
   RTI1516_NAMESPACE::VariableLengthData get_encoded_parameter_value();

   /*! @brief Extract the data out of the HLA Parameter Value.
    *  @param param_size Parameter data size.
    *  @param param_data Parameter data.
    *  @return True if successfully extracted data, false otherwise. */
   bool extract_data( int const            param_size,
                      unsigned char const *param_data );

   /*! @brief Check if a parameter value has changed.
    *  @return True if a parameter value has changed; False otherwise. */
   bool is_changed() const
   {
      return this->value_changed;
   }

   /*! @brief Mark the parameter as having changed. */
   void mark_changed()
   {
      this->value_changed = true;
   }

   /*! @brief  Mark the parameter as having NOT changed. */
   void mark_unchanged()
   {
      this->value_changed = false;
   }

   /*! @brief Unpack the parameter from the buffer into the trick-variable
    * using the appropriate decoding. */
   void unpack_parameter_buffer();

   /*! @brief Prints the contents of buffer used to encode/decode the parameter
    * to the console on standard out. */
   void print_buffer() const;

   /*! @brief Get the Trick ATTRIBUTES for this Parameter.
    *  @return A copy of the Trick ATTRIBUTES structure for this Parameter. */
   ATTRIBUTES get_ref2_attributes() const
   {
      return ( *attr );
   }

   /*! @brief Get the RTI encoding for this Parameter.
    *  @return The encoding type for this Parameter. */
   EncodingEnum get_rti_encoding() const
   {
      return rti_encoding;
   }

  private:
   unsigned char *buffer;          ///< @trick_units{--} Byte buffer for the attribute value bytes.
   int            buffer_capacity; ///< @trick_units{--} The capacity of the buffer.

   bool size_is_static; ///< @trick_units{--} Flag to indicate the size of this attribute is static.

   int size;      ///< @trick_units{--} The size of the attribute in bytes.
   int num_items; ///< @trick_units{--} Number of attribute items, length of the array.

   bool value_changed; ///< @trick_units{--} Flag to indicate the attribute value changed.

   unsigned int HLAtrue; ///< @trick_units{--} A 32-bit integer with a value of 1 on a Big Endian computer.

   bool byteswap; ///< @trick_units{--} Flag to indicate byte-swap before RTI Rx/Tx.

   void       *address;              ///< @trick_io{**} Address of the trick variable
   ATTRIBUTES *attr;                 ///< @trick_io{**} ATTRIBUTES of the trick variable
   char       *interaction_FOM_name; ///< @trick_io{**} Copy of the user-supplied interaction FOM_name

   RTI1516_NAMESPACE::ParameterHandle param_handle; ///< @trick_io{**} The RTI parameter handle.

   /*! @brief Ensure the parameter buffer has at least the specified capacity.
    *  @param capacity Desired capacity of the buffer in bytes. */
   void ensure_buffer_capacity( int capacity );

   /*! @brief Pack the parameter into the buffer using the appropriate encoding. */
   void pack_parameter_buffer();

   /*! @brief Gets the parameter size in bytes.
    *  @return The size in bytes of the parameter. */
   int get_parameter_size();

   /*! @brief Calculates the parameter size in bytes and the number of items
    * it contains. */
   void calculate_size_and_number_of_items();

   /*! @brief Calculates the number of static items contained by the parameter. */
   void calculate_static_number_of_items();

   /*! @brief Determines if the HLA interaction parameter type is supported
    * given the RTI encoding.
    *  @return True if supported, false otherwise. */
   bool is_supported_parameter_type() const;

   /*! @brief Encode a boolean parameter into the buffer using the HLAboolean
    * data type which is encoded as a HLAinteger32BE. */
   void encode_boolean_to_buffer();

   /*! @brief Decode a boolean parameter from the buffer using the HLAboolean
    * data type which is encoded as a HLAinteger32BE. */
   void decode_boolean_from_buffer() const;

   /*! @brief Encode the interaction parameter using the HLAlogicalTime 64-bit
    * integer encoding. */
   void encode_logical_time() const;

   /*! @brief Decode the interaction parameter that is using the HLAlogicalTime
    * 64-bit integer encoding. */
   void decode_logical_time();

   /*! @brief Encode the data as HLA opaque data into the buffer. */
   void encode_opaque_data_to_buffer();

   /*! @brief Decode the opaque data in the buffer. */
   void decode_opaque_data_from_buffer();

   /*! @brief Encode a string parameter into the buffer using the appropriate encoding. */
   void encode_string_to_buffer();

   /*! @brief Decode a string from the buffer into the parameter using the appropriate decoding. */
   void decode_string_from_buffer();

   /*! @brief Copy the data from the source to the destination and byteswap as needed.
    *  @param dest      Destination to copy data to.
    *  @param src       Source of the data to byteswap and copy from.
    *  @param type      The type of the data.
    *  @param length    The length/number of entries in the source array.
    *  @param num_bytes The number of bytes in the source array. */
   void byteswap_buffer_copy( void       *dest,
                              void const *src,
                              int const   type,
                              int const   length,
                              int const   num_bytes ) const;

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Parameter class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Parameter( Parameter const &rhs );

   /*! @brief Assignment operator for Parameter class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Parameter &operator=( Parameter const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_PARAMETER_HH
