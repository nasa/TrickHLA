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
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_ENCODER_BASE_HH
#define TRICKHLA_ENCODER_BASE_HH

// System includes.
#include <cstddef>
#include <string>
#include <vector>

// Trick include files.
#include "trick/attributes.h"

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
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

namespace TrickHLA
{

class EncoderBase
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
   EncoderBase( void       *var_address,
                ATTRIBUTES *var_attr );

   /*! @brief Destructor for the TrickHLA EncoderBase class. */
   virtual ~EncoderBase();

   virtual RTI1516_NAMESPACE::VariableLengthData &encode();

   virtual bool const decode( RTI1516_NAMESPACE::VariableLengthData const &encoded_data );

   virtual std::string to_string();

   void calculate_attr_element_count();

   bool const is_primitive()
   {
      return ( ( attr != NULL ) && ( attr->num_index == 0 ) );
   }

   bool const is_array()
   {
      return ( ( attr != NULL ) && ( attr->num_index > 0 ) );
   }

   bool const is_1d_array()
   {
      return ( ( attr != NULL ) && ( attr->num_index == 1 ) );
   }

   bool const is_static_array()
   {
      return ( is_array() && ( attr->index[attr->num_index - 1].size != 0 ) );
   }

   bool const is_dynamic_array()
   {
      return ( is_array() && ( attr->index[attr->num_index - 1].size == 0 ) );
   }

   bool const is_static_in_size()
   {
      return ( !is_array() || is_static_array() );
   }

   bool const is_null_address()
   {
      return ( address == NULL )
             || ( is_dynamic_array() && ( *static_cast< void ** >( address ) == NULL ) );
   }

  protected:
   void *address; ///< @trick_units{--} Address of the trick variable.

   ATTRIBUTES *attr; ///< @trick_io{**} The attributes of the trick variable.

   std::size_t attr_element_count; ///< @trick_units{--} Number of elements (i.e. size) of the trick variable.

   RTI1516_NAMESPACE::VariableLengthData data; ///< @trick_io{**} Holds HLA encoded data.

   std::vector< RTI1516_NAMESPACE::DataElement * > data_elements; ///< @trick_io{**} Vector of data elements.

   RTI1516_NAMESPACE::DataElement *encoder; ///< @trick_units{--} HLA data element encoder.

  private:
   // Do not allow the copy constructor or assignment operator.
   EncoderBase();
   /*! @brief Copy constructor for EncoderBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   EncoderBase( EncoderBase const &rhs );
   /*! @brief Assignment operator for EncoderBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   EncoderBase &operator=( EncoderBase const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_ENCODER_BASE_HH
