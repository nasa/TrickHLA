/*!
@file TrickHLA/OpaqueBuffer.hh
@ingroup TrickHLA
@brief This class provides a generic opaque buffer that is in the Trick managed
memory space. This is useful for sending a fixed record of data that includes
byte padding to ensure a byte alignment.

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
@trick_link_dependency{../../source/TrickHLA/OpaqueBuffer.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, July 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_OPAQUE_BUFFER_HH
#define TRICKHLA_OPAQUE_BUFFER_HH

// TrickHLA includes.
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

class OpaqueBuffer
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__OpaqueBuffer();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA OpaqueBuffer class. */
   OpaqueBuffer();
   /*! @brief Destructor for the TrickHLA OpaqueBuffer class. */
   virtual ~OpaqueBuffer();

   /*! @brief Set the byte alignment to use for the data in the buffer.
    *  @param size Byte alignment size. */
   void set_byte_alignment( unsigned int const size );

   /*! @brief Get the current buffer byte alignment.
    *  @return Buffer byte alignment. */
   unsigned int get_byte_alignment() const
   {
      return alignment;
   }

   /*! @brief Get the current buffer capacity.
    *  @return Buffer capacity. */
   size_t get_capacity() const
   {
      return capacity;
   }

   /*! @brief Ensure the buffer has at least the specified capacity.
    *  @param size Requested buffer capacity. */
   void ensure_buffer_capacity( size_t const size );

   /*! @brief Reset the push buffer position. */
   void reset_push_position()
   {
      this->push_pos = 0;
   }

   /*! @brief Reset the pull buffer position. */
   void reset_pull_position()
   {
      this->pull_pos = 0;
   }

   /*! @brief Reset both the push and pull buffer positions. */
   void reset_buffer_positions()
   {
      reset_push_position();
      reset_pull_position();
   }

   /*! @brief Push the specified data into the buffer with no encoding.
    *  @param src  Source of data to push into buffer.
    *  @param size Size of data in bytes. */
   void push_to_buffer( void const *src, size_t const size )
   {
      push_to_buffer( src, size, ENCODING_NONE );
   }

   /*! @brief Push the specified data into the buffer using the specified
    *  encoding.
    *  @param src      Source of data to push into buffer.
    *  @param size     Size of data in bytes.
    *  @param encoding One of ENCODING_LITTLE_ENDIAN, ENCODING_BIG_ENDIAN, or ENCODING_NONE. */
   void push_to_buffer( void const *src, size_t const size, EncodingEnum const encoding );

   /*! @brief Pull the specified number of data bytes from the buffer into the
    * specified destination with no encoding.
    *  @param dest Destination to pull data into from buffer.
    *  @param size Size of data in bytes. */
   void pull_from_buffer( void *dest, size_t const size )
   {
      pull_from_buffer( dest, size, ENCODING_NONE );
   }

   /*! @brief Pull the specified number of data bytes from the buffer into the
    * specified destination and for the specified encoding of the data in the
    * buffer.
    *  @param dest     Destination to pull data into from buffer.
    *  @param size     Size of data in bytes.
    *  @param encoding One of ENCODING_LITTLE_ENDIAN, ENCODING_BIG_ENDIAN, or ENCODING_NONE. */
   void pull_from_buffer( void *dest, size_t const size, EncodingEnum const encoding );

  protected:
   /*! @brief Push the specified number of pad bytes to the buffer.
    *  @param pad_size Size of data in bytes. */
   void push_pad_to_buffer( size_t const pad_size );

   /*! @brief Pull the specified number of pad bytes from the buffer.
    *  @param pad_size Size of data in bytes. */
   void pull_pad_from_buffer( size_t const pad_size );

   /*! @brief Copy the data from the source to the destination and byteswap
    * if needed.
    *  @param dest     Destination to put data.
    *  @param src      Source data.
    *  @param size     Number of bytes to copy.
    *  @param encoding One of ENCODING_LITTLE_ENDIAN, ENCODING_BIG_ENDIAN, or ENCODING_NONE. */
   static void byteswap_buffer_copy( void              *dest,
                                     void const        *src,
                                     size_t const       size,
                                     EncodingEnum const encoding );

  public:
   unsigned int alignment; ///< @trick_units{--} The byte alignment to use for the buffer.

   size_t push_pos; ///< @trick_units{--} Position to push data to.
   size_t pull_pos; ///< @trick_units{--} Position to pull data from.
   size_t capacity; ///< @trick_units{--} Capacity of the buffer.

   unsigned char *buffer; ///< @trick_units{--} Byte buffer.
};

} // namespace TrickHLA

#endif // TRICKHLA_OPAQUE_BUFFER_HH: Do NOT put anything after this line!
