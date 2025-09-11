/*!
@file TrickHLA/encoding/StringUnicodeVariableArrayEncoder.hh
@ingroup TrickHLA
@brief This class represents the std::string Unicode string variable array
encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/StringUnicodeVariableArrayEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/VariableArrayEncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, Sept 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_STRING_UNICODE_VARIABLE_ARRAY_ENCODER_HH
#define TRICKHLA_STRING_UNICODE_VARIABLE_ARRAY_ENCODER_HH

// System includes.
#include <string>
#include <vector>

// Trick includes.
#include "trick/attributes.h"

// TrickHLA includes.
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class StringUnicodeVariableArrayEncoder : public VariableArrayEncoderBase
{
   /* Let the Trick input processor access protected and private data. */
   /* InputProcessor is really just a marker class (does not really    */
   /* exists - at least yet). This friend statement just tells Trick   */
   /* to go ahead and process the protected and private data as well   */
   /* as the usual public data.                                        */
   friend class InputProcessor;
   /* IMPORTANT Note: you must have the following line too.            */
   /* Syntax: friend void init_attr<namespace>__<class name>();        */
   friend void init_attrTrickHLA__StringUnicodeVariableArrayEncoder();

  public:
   /*! @brief Default constructor. */
   StringUnicodeVariableArrayEncoder( void              *addr,
                                            ATTRIBUTES        *attr,
                                            std::string const &name );

   /*! @brief Destructor for the TrickHLA StringUnicodeVariableArrayEncoder class. */
   virtual ~StringUnicodeVariableArrayEncoder();

   virtual void update_before_encode();

   virtual void update_after_decode();

   virtual int const get_data_size();

   virtual std::string to_string()
   {
      return "StringUnicodeVariableArrayEncoder[" + data_name + "]";
   }

  protected:
   void resize_data_elements( std::size_t const new_size );

  private:
   /* Do not allow the default, copy constructor or assignment operator. */
   StringUnicodeVariableArrayEncoder();
   /*! @brief Copy constructor for StringUnicodeVariableArrayEncoder class.
    *  @details This constructor is private to prevent inadvertent copies. */
   StringUnicodeVariableArrayEncoder( StringUnicodeVariableArrayEncoder const &rhs );
   /*! @brief Assignment operator for StringUnicodeVariableArrayEncoder class.
    *  @details Assignment operator is private to prevent inadvertent copies. */
   StringUnicodeVariableArrayEncoder &operator=( StringUnicodeVariableArrayEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_STRING_UNICODE_VARIABLE_ARRAY_ENCODER_HH
