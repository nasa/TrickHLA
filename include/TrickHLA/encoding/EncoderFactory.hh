/*!
@file TrickHLA/EncoderFactory.hh
@ingroup TrickHLA
@brief This class represents the encoder factory implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderFactory.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_ENCODER_FACTORY_HH
#define TRICKHLA_ENCODER_FACTORY_HH

// System includes.
#include <cstddef>
#include <string>

// Trick include files.
#include "trick/reference.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

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

class EncoderFactory
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__EncoderFactory();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA EncoderFactory class. */
   EncoderFactory();

   /*! @brief Destructor for the TrickHLA EncoderFactory class. */
   virtual ~EncoderFactory();

   static TrickHLA::EncoderBase *create(
      std::string const &trick_name,
      EncodingEnum const hla_encoding );

  protected:
   static TrickHLA::EncoderBase *create_char_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_string_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_wchar_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_wstring_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_int16_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_int32_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_int64_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

#if defined( IEEE_1516_2025 )
   static TrickHLA::EncoderBase *create_uint16_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_uint32_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_uint64_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );
#endif

   static TrickHLA::EncoderBase *create_float32_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_float64_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

   static TrickHLA::EncoderBase *create_bool_encoder(
      std::string const &trick_name,
      EncodingEnum const hla_encoding,
      REF2              *ref2 );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for EncoderFactory class.
    *  @details This constructor is private to prevent inadvertent copies. */
   EncoderFactory( EncoderFactory const &rhs );
   /*! @brief Assignment operator for EncoderFactory class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   EncoderFactory &operator=( EncoderFactory const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_ENCODER_FACTORY_HH
