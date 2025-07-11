/*!
@file TrickHLA/encoding/EncoderBase.hh
@ingroup TrickHLA
@brief This class represents the base encoder implementation.

@copyright Copyright 2025 United States Government as represented by the
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
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_ENCODER_BASE_HH
#define TRICKHLA_ENCODER_BASE_HH

// System includes.
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// TrickHLA includes.
#include "TrickHLA/StandardsSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/EncodingConfig.h"

namespace TrickHLA
{

class EncoderBase : public RTI1516_NAMESPACE::DataElement
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__EncoderBase();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA EncoderBase class. */
   EncoderBase();

   /*! @brief Destructor for the TrickHLA EncoderBase class. */
   virtual ~EncoderBase();

   virtual void update_before_encode() = 0;

   virtual void update_after_decode() = 0;

   /*! @brief Number of data bytes */
   virtual int const get_data_size() = 0;

   virtual std::string to_string()
   {
      return "EncoderBase";
   }

#if !defined( SWIG )
   virtual std::auto_ptr< RTI1516_NAMESPACE::DataElement > clone() const
   {
      return data_encoder->clone();
   }

   virtual RTI1516_NAMESPACE::VariableLengthData encode() const throw( RTI1516_NAMESPACE::EncoderException )
   {
      return ( data_encoder != NULL ) ? data_encoder->encode()
                                      : RTI1516_NAMESPACE::VariableLengthData();
   }

   virtual void encode( RTI1516_NAMESPACE::VariableLengthData &inData ) const throw( RTI1516_NAMESPACE::EncoderException )
   {
      if ( data_encoder != NULL ) {
         data_encoder->encode( inData );
      }
   }

   virtual void encodeInto( std::vector< RTI1516_NAMESPACE::Octet > &buffer ) const throw( RTI1516_NAMESPACE::EncoderException )
   {
      if ( data_encoder != NULL ) {
         data_encoder->encodeInto( buffer );
      }
   }

   virtual void decode( RTI1516_NAMESPACE::VariableLengthData const &inData ) throw( RTI1516_NAMESPACE::EncoderException )
   {
      if ( data_encoder != NULL ) {
         data_encoder->decode( inData );
      }
   }

   virtual size_t decodeFrom(
      std::vector< RTI1516_NAMESPACE::Octet > const &buffer,
      size_t                                         index ) throw( RTI1516_NAMESPACE::EncoderException )
   {
      return ( data_encoder != NULL ) ? data_encoder->decodeFrom( buffer, index ) : index;
   }

   virtual size_t getEncodedLength() const throw( RTI1516_NAMESPACE::EncoderException )
   {
      return ( data_encoder != NULL ) ? data_encoder->getEncodedLength() : 0;
   }

   virtual unsigned int getOctetBoundary() const
   {
      return ( data_encoder != NULL ) ? data_encoder->getOctetBoundary() : 1;
   }

   virtual bool isSameTypeAs( RTI1516_NAMESPACE::DataElement const &inData ) const
   {
      return ( data_encoder != NULL ) ? data_encoder->isSameTypeAs( inData ) : false;
   }

   virtual RTI1516_NAMESPACE::Integer64 hash() const
   {
      return ( data_encoder != NULL ) ? data_encoder->hash() : RTI1516_NAMESPACE::Integer64( 0 );
   }
#endif // SWIG

   RTI1516_NAMESPACE::DataElement *data_encoder; ///< @trick_io{**} Data encoder.

   RTI1516_NAMESPACE::VariableLengthData data; ///< @trick_io{**} Holds HLA encoded data.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for EncoderBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   EncoderBase( EncoderBase const &rhs );
   /*! @brief Assignment operator for EncoderBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   EncoderBase &operator=( EncoderBase const &rhs );
};

} // namespace TrickHLA

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic pop

#endif // TRICKHLA_ENCODER_BASE_HH
