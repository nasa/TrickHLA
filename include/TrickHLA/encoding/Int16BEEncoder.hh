/*!
@file TrickHLA/Int16BEEncoder.hh
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
@trick_link_dependency{../../../source/TrickHLA/encoding/Int16BEEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_INT16BE_ENCODER_HH
#define TRICKHLA_INT16BE_ENCODER_HH

// System includes.
#include <cstdint>
#include <map>
#include <stdlib.h>
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

namespace TrickHLA
{

class Int16BEEncoder
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Int16BEEncoder();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Int16BEEncoder class. */
   Int16BEEncoder( std::string const &trick_variable_name,
                   std::string const &fom_variable_name,
                   EncodingEnum       hla_encoding );

   /*! @brief Destructor for the TrickHLA Int16BEEncoder class. */
   virtual ~Int16BEEncoder();

  protected:
   /*! @brief Initializes the TrickHLA Int16BEEncoder. */
   void initialize();

  protected:
   std::string trick_name; ///< @trick_units{--} Trick variable name.
   std::string fom_name;   ///< @trick_units{--} FOM variable name.

   EncodingEnum rti_encoding; ///< @trick_units{--} RTI encoding of the data.

   REF2 *ref2; ///< @trick_io{**} The ref_attributes of the given trick_name.

   bool is_array;        ///< @trick_units{--} Is the user data an array.
   bool is_1d_array;     ///< @trick_units{--} Is the user data a 1-D array.
   bool is_static_array; ///< @trick_units{--} Is the user data a static array.

   RTI1516_NAMESPACE::DataElement *encoder; ///< @trick_io{**} The RTI encoder data element.

   bool initialized; ///< @trick_units{--} Has this attribute been initialized?

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Int16BEEncoder class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Int16BEEncoder( Int16BEEncoder const &rhs );
   /*! @brief Assignment operator for Int16BEEncoder class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Int16BEEncoder &operator=( Int16BEEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_INT16BE_ENCODER_HH
