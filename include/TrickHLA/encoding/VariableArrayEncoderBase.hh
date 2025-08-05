/*!
@file TrickHLA/encoding/VariableArrayEncoderBase.hh
@ingroup TrickHLA
@brief This class represents the variable array base encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/VariableArrayEncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_VARIABLE_ARRAY_ENCODER_BASE_HH
#define TRICKHLA_VARIABLE_ARRAY_ENCODER_BASE_HH

// System includes.
#include <cstddef>
#include <string>
#include <vector>

// Trick includes.
#include "trick/attributes.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/parameter_types.h"

// TrickHLA includes.
#include "TrickHLA/StandardsSupport.hh"
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
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class VariableArrayEncoderBase : public EncoderBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__VariableArrayEncoderBase();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Constructor for the TrickHLA VariableArrayEncoderBase class. */
   VariableArrayEncoderBase( void              *addr,
                             ATTRIBUTES        *attr,
                             std::string const &name );

   /*! @brief Destructor for the TrickHLA VariableArrayEncoderBase class. */
   virtual ~VariableArrayEncoderBase();

   virtual void update_before_encode();

   virtual void update_after_decode();

   virtual int const get_data_size();

   virtual std::string to_string()
   {
      return "VariableArrayEncoderBase[" + data_name + "]";
   }

   void calculate_var_element_count();

   void resize_trick_var( std::size_t const new_size );

   bool const is_array()
   {
      return is_array_flag;
   }

   bool const is_1d_array()
   {
      return is_1d_array_flag;
   }

   bool const is_static_array()
   {
      return is_static_array_flag;
   }

   bool const is_dynamic_array()
   {
      return is_dynamic_array_flag;
   }

   bool const is_static_in_size()
   {
      return ( !is_array() || is_static_array() );
   }

  protected:
   void *address; ///< @trick_units{--} Address of the trick variable.

   TRICK_TYPE type; ///< @trick_units{--} The trick variable type.

   std::size_t var_element_count; ///< @trick_units{--} Number of elements (i.e. size) of the trick variable.

   std::vector< RTI1516_NAMESPACE::DataElement * > data_elements; ///< @trick_io{**} Vector of data elements.

   bool is_array_flag;         ///< @trick_units{--} Flag indicating is array.
   bool is_1d_array_flag;      ///< @trick_units{--} Flag indicating is 1D array.
   bool is_static_array_flag;  ///< @trick_units{--} Flag indicating is static array.
   bool is_dynamic_array_flag; ///< @trick_units{--} Flag indicating is dynamic array.

  private:
   // Do not allow the copy constructor or assignment operator.
   VariableArrayEncoderBase();
   /*! @brief Copy constructor for VariableArrayEncoderBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   VariableArrayEncoderBase( VariableArrayEncoderBase const &rhs );
   /*! @brief Assignment operator for VariableArrayEncoderBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   VariableArrayEncoderBase &operator=( VariableArrayEncoderBase const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_VARIABLE_ARRAY_ENCODER_BASE_HH
