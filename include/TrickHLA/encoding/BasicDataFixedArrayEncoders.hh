/*!
@file TrickHLA/encoding/BasicDataFixedArrayEncoders.hh
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

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_BASIC_DATA_FIXED_ARRAY_ENCODERS_HH
#define TRICKHLA_BASIC_DATA_FIXED_ARRAY_ENCODERS_HH

// System includes.
#include <cstddef>
#include <string>

// Trick includes.
#include "trick/attributes.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/EncodingConfig.h"
#include "RTI/encoding/HLAfixedArray.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

#define DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( EncoderClassName, SimpleDataType )  \
                                                                                    \
   class EncoderClassName : public EncoderBase                                      \
   {                                                                                \
      /* Let the Trick input processor access protected and private data. */        \
      /* InputProcessor is really just a marker class (does not really    */        \
      /* exists - at least yet). This friend statement just tells Trick   */        \
      /* to go ahead and process the protected and private data as well   */        \
      /* as the usual public data.                                        */        \
      friend class InputProcessor;                                                  \
      /* IMPORTANT Note: you must have the following line too.            */        \
      /* Syntax: friend void init_attr<namespace>__<class name>();        */        \
      friend void init_attrTrickHLA__EncoderClassName();                            \
                                                                                    \
     public:                                                                        \
      /*! @brief Default constructor for the TrickHLA EncoderClassName class. */    \
      EncoderClassName( void              *addr,                                    \
                        ATTRIBUTES        *attr,                                    \
                        std::string const &name );                                  \
                                                                                    \
      /*! @brief Destructor for the TrickHLA EncoderClassName class. */             \
      virtual ~EncoderClassName();                                                  \
                                                                                    \
      virtual void update_before_encode()                                           \
      {                                                                             \
         return;                                                                    \
      }                                                                             \
                                                                                    \
      virtual void update_after_decode()                                            \
      {                                                                             \
         return;                                                                    \
      }                                                                             \
                                                                                    \
      virtual int const get_data_size();                                            \
                                                                                    \
      virtual std::string to_string()                                               \
      {                                                                             \
         return std::string( #EncoderClassName ) + "[" + data_name + "]";           \
      }                                                                             \
                                                                                    \
     private:                                                                       \
      /* Do not allow the default, copy constructor or assignment operator. */      \
      EncoderClassName();                                                           \
      /*! @brief Assignment operator for EncoderClassName class.                 */ \
      /*  @details Assignment operator is private to prevent inadvertent copies. */ \
      EncoderClassName &operator=( EncoderClassName const &rhs );                   \
   };

DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIICharFixedArrayEncoder, char )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIIStringFixedArrayEncoder, std::string )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( BoolFixedArrayEncoder, bool )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ByteFixedArrayEncoder, RTI1516_NAMESPACE::Octet )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32BEFixedArrayEncoder, float )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32LEFixedArrayEncoder, float )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64BEFixedArrayEncoder, double )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64LEFixedArrayEncoder, double )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16BEFixedArrayEncoder, RTI1516_NAMESPACE::Integer16 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16LEFixedArrayEncoder, RTI1516_NAMESPACE::Integer16 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32BEFixedArrayEncoder, RTI1516_NAMESPACE::Integer32 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32LEFixedArrayEncoder, RTI1516_NAMESPACE::Integer32 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64BEFixedArrayEncoder, RTI1516_NAMESPACE::Integer64 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64LEFixedArrayEncoder, RTI1516_NAMESPACE::Integer64 )

#if defined( IEEE_1516_2025 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16BEFixedArrayEncoder, RTI1516_NAMESPACE::UnsignedInteger16 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16LEFixedArrayEncoder, RTI1516_NAMESPACE::UnsignedInteger16 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32BEFixedArrayEncoder, RTI1516_NAMESPACE::UnsignedInteger32 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32LEFixedArrayEncoder, RTI1516_NAMESPACE::UnsignedInteger32 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64BEFixedArrayEncoder, RTI1516_NAMESPACE::UnsignedInteger64 )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64LEFixedArrayEncoder, RTI1516_NAMESPACE::UnsignedInteger64 )
#endif // IEEE_1516_2025

DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeCharFixedArrayEncoder, wchar_t )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DEFINE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeStringFixedArrayEncoder, std::wstring )
#endif // TRICK_WSTRING_MM_SUPPORT

} // namespace TrickHLA

#endif // TRICKHLA_BASIC_DATA_FIXED_ARRAY_ENCODERS_HH
