/*!
@file TrickHLA/encoding/BasicDataVariableArrayEncoders.hh
@ingroup TrickHLA
@brief This class represents the basic data variable array encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/BasicDataVariableArrayEncoders.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/VariableArrayEncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_BASIC_DATA_VARIABLE_ARRAY_ENCODERS_HH
#define TRICKHLA_BASIC_DATA_VARIABLE_ARRAY_ENCODERS_HH

// System includes.
#include <cstddef>
#include <string>

// Trick includes.
#include "trick/attributes.h"

// TrickHLA includes.
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAvariableArray.h"

namespace TrickHLA
{

#define DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( EncoderClassName )               \
                                                                                    \
   class EncoderClassName : public VariableArrayEncoderBase                         \
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
      /*! @brief Default constructor. */                                            \
      EncoderClassName( void *addr, ATTRIBUTES *attr );                             \
                                                                                    \
      /*! @brief Destructor for the TrickHLA EncoderClassName class. */             \
      virtual ~EncoderClassName();                                                  \
                                                                                    \
      virtual void update_before_encode();                                          \
                                                                                    \
      virtual void update_after_decode();                                           \
                                                                                    \
      virtual std::string to_string()                                               \
      {                                                                             \
         return #EncoderClassName;                                                  \
      }                                                                             \
                                                                                    \
     protected:                                                                     \
      void resize_data_elements( std::size_t new_size );                            \
                                                                                    \
     private:                                                                       \
      /* Do not allow the default, copy constructor or assignment operator. */      \
      /* EncoderClassName(); */                                                     \
      /*! @brief Copy constructor for EncoderClassName class.                 */    \
      /*  @details This constructor is private to prevent inadvertent copies. */    \
      EncoderClassName( EncoderClassName const &rhs );                              \
      /*! @brief Assignment operator for EncoderClassName class.                 */ \
      /*  @details Assignment operator is private to prevent inadvertent copies. */ \
      EncoderClassName &operator=( EncoderClassName const &rhs );                   \
   };

DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ASCIICharVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ASCIIStringVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( BoolVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ByteVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float32BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float32LEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float64BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float64LEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int16BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int16LEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int32BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int32LEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int64BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int64LEVariableArrayEncoder )

#if defined( IEEE_1516_2025 )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt16BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt16LEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt32BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt32LEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt64BEVariableArrayEncoder )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt64LEVariableArrayEncoder )
#endif // IEEE_1516_2025

DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeCharVariableArrayEncoder )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DEFINE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeStringVariableArrayEncoder )
#endif // TRICK_WSTRING_MM_SUPPORT

} // namespace TrickHLA

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic pop

#endif // TRICKHLA_BASIC_DATA_VARIABLE_ARRAY_ENCODERS_HH
