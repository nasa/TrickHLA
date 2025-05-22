/*!
@file TrickHLA/BasicDataFixedArrayEncoders.hh
@ingroup TrickHLA
@brief This class represents the basic data fixed array encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/BasicDataFixedArrayEncoders.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_BASIC_DATA_FIXED_ARRAY_ENCODERS_HH
#define TRICKHLA_BASIC_DATA_FIXED_ARRAY_ENCODERS_HH

// System includes.
#include <cstdint>
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
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

#if !defined( SWIG )

namespace TrickHLA
{

#   define DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( EncoderClassName )                       \
                                                                                            \
      class EncoderClassName : public EncoderBase                                           \
      {                                                                                     \
         /* Let the Trick input processor access protected and private data. */             \
         /* InputProcessor is really just a marker class (does not really    */             \
         /* exists - at least yet). This friend statement just tells Trick   */             \
         /* to go ahead and process the protected and private data as well   */             \
         /* as the usual public data.                                        */             \
         friend class InputProcessor;                                                       \
         /* IMPORTANT Note: you must have the following line too.            */             \
         /* Syntax: friend void init_attr<namespace>__<class name>();        */             \
         friend void init_attrTrickHLA__EncoderClassName();                                 \
                                                                                            \
        public:                                                                             \
         /*! @brief Default constructor for the TrickHLA EncoderClassName class. */         \
         EncoderClassName( std::string const &trick_variable_name,                          \
                           EncodingEnum const hla_encoding,                                 \
                           REF2              *r2 );                                                      \
                                                                                            \
         /*! @brief Destructor for the TrickHLA EncoderClassName class. */                  \
         virtual ~EncoderClassName();                                                       \
                                                                                            \
         virtual std::string to_string();                                                   \
                                                                                            \
        private:                                                                            \
         /* Do not allow the default, copy constructor or assignment operator. */           \
         EncoderClassName();                                                                \
         /*! @brief Copy constructor for EncoderClassName class.      */                    \
         /*  @details This constructor is private to prevent inadvertent copies. */         \
         EncoderClassName( EncoderClassName const &rhs );                                   \
         /*! @brief Assignment operator for EncoderClassName class.           */            \
         /*  @details This assignment operator is private to prevent inadvertent copies. */ \
         EncoderClassName &operator=( EncoderClassName const &rhs );                        \
      };

DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIICharFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIIStringFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( BoolFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ByteFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32LEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64LEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16LEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32LEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64LEFixedArrayEncoder )
#   if defined( IEEE_1516_2025 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16LEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32LEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64BEFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64LEFixedArrayEncoder )
#   endif
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeCharFixedArrayEncoder )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeStringFixedArrayEncoder )

} // namespace TrickHLA

#endif // SWIG
#endif // TRICKHLA_BASIC_DATA_FIXED_ARRAY_ENCODERS_HH
