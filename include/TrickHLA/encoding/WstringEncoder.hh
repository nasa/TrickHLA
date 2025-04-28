/*!
@file TrickHLA/WstringEncoder.hh
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
@trick_link_dependency{../../../source/TrickHLA/encoding/WstringEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/Parameter.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_WSTRING_ENCODER_HH
#define TRICKHLA_WSTRING_ENCODER_HH

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

class WstringEncoder
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__WstringEncoder();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA WstringEncoder class. */
   WstringEncoder( std::string const &trick_name, std::string const &fom_name);

   /*! @brief Destructor for the TrickHLA WstringEncoder class. */
   virtual ~WstringEncoder();

   /*! @brief Initializes the TrickHLA WstringEncoder. */
   void initialize();

  protected:
   std::string trick_name; ///< @trick_units{--} Trick variable name.
   std::string fom_name;   ///< @trick_units{--} FOM variable name.

   REF2 *ref2; ///< @trick_io{**} The ref_attributes of the given trick_name.

   EncodingEnum rti_encoding; ///< @trick_units{--} RTI encoding of the data.

   bool is_array;          ///< @trick_units{--} Is the user data an array.
   bool is_1d_array;       ///< @trick_units{--} Is the user data a 1-D array.
   bool is_static_array;   ///< @trick_units{--} Is the user data a static array.

   RTI1516_NAMESPACE::DataElement *encoder; ///< @trick_io{**} The RTI encoder data element.

   bool initialized; ///< @trick_units{--} Has this attribute been initialized?

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for WstringEncoder class.
    *  @details This constructor is private to prevent inadvertent copies. */
   WstringEncoder( WstringEncoder const &rhs );
   /*! @brief Assignment operator for WstringEncoder class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   WstringEncoder &operator=( WstringEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_WSTRING_ENCODER_HH
