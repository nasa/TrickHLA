/*!
@file TrickHLA/Parameter.hh
@ingroup TrickHLA
@brief This class represents the HLA parameters of an interaction that is
managed by Trick.

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
@trick_link_dependency{../../source/TrickHLA/Parameter.cpp}
@trick_link_dependency{../../source/TrickHLA/RecordElement.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Aug 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Encoder helpers rewrite.}
@revs_end

*/

#ifndef TRICKHLA_PARAMETER_HH
#define TRICKHLA_PARAMETER_HH

// System include files.

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/attributes.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/RecordElement.hh"
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
#pragma GCC diagnostic pop

namespace TrickHLA
{

class Parameter : public RecordElement
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Parameter();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Parameter class. */
   Parameter();

   /*! @brief Destructor for the TrickHLA Parameter class. */
   virtual ~Parameter();

   /*! @brief Initializes the TrickHLA Parameter from the trick variable name
    *  supplied in the input file.
    *  @param interaction_fom_name The FOM name of the parent interaction.
    *  @param interaction_index The array index to the parent TrickHLA::Interaction.
    *  @param parameter_index The array index to this Parameter.
    */
   void initialize( char const *interaction_fom_name,
                    int const   interaction_index,
                    int const   parameter_index );

   /*! @brief Initializes the TrickHLA Parameter from the supplied address and
    * ATTRIBUTES of the trick variable.
    *  @param interaction_fom_name FOM name of the interaction.
    *  @param address Address of the trick variable.
    *  @param attr ATTRIBUTES of the trick variable.
    */
   void initialize( char const *interaction_fom_name,
                    void       *address,
                    ATTRIBUTES *attr );

   RTI1516_NAMESPACE::VariableLengthData &encode();

   bool const decode( RTI1516_NAMESPACE::VariableLengthData const &encoded_data );

   /*! @brief Get the FOM name for this parameter.
    *  @return The FOM name of this parameter. */
   char const *get_FOM_name() const
   {
      return FOM_name;
   }

   /*! @brief Set the FOM name for the parameter.
    *  @param in_name The FOM name for the parameter. */
   void set_FOM_name( char const *in_name )
   {
      if ( this->FOM_name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->FOM_name ) ) ) {
            message_publish( MSG_WARNING, "Parameter::set_FOM_name():%d WARNING failed to delete Trick Memory for 'this->FOM_name'\n",
                             __LINE__ );
         }
      }
      this->FOM_name = trick_MM->mm_strdup( in_name );
   }

   /*! @brief Get the Trick variable name associated with this parameter.
    *  @return The Trick variable name associated with this parameter. */
   char const *get_trick_name() const
   {
      return trick_name;
   }

   /*! @brief Set the RTI encoding and based on the new encoding determine if
    * we need to byte-swap.
    *  @param in_type The encoding type for this parameter. */
   void set_encoding( EncodingEnum in_type )
   {
      rti_encoding = in_type;
   }

   /*! @brief Get the HLA Parameter handle.
    *  @return The HLA Parameter handle. */
   RTI1516_NAMESPACE::ParameterHandle get_parameter_handle() const
   {
      return this->param_handle;
   }

   /*! @brief Set the associated HLA Parameter handle.
    *  @param hdl The associated HLA Parameter handle. */
   void set_parameter_handle( RTI1516_NAMESPACE::ParameterHandle const hdl )
   {
      this->param_handle = hdl;
   }

   /*! @brief Check if a parameter value has changed.
    *  @return True if a parameter value has changed; False otherwise. */
   bool is_changed() const
   {
      return this->value_changed;
   }

   /*! @brief Mark the parameter as having changed. */
   void mark_changed()
   {
      this->value_changed = true;
   }

   /*! @brief  Mark the parameter as having NOT changed. */
   void mark_unchanged()
   {
      this->value_changed = false;
   }

   /*! @brief Get the RTI encoding for this Parameter.
    *  @return The encoding type for this Parameter. */
   EncodingEnum get_rti_encoding() const
   {
      return rti_encoding;
   }

  private:
   bool value_changed; ///< @trick_units{--} Flag to indicate the attribute value changed.

   char *interaction_FOM_name; ///< @trick_io{**} Copy of the user-supplied interaction FOM_name

   RTI1516_NAMESPACE::ParameterHandle param_handle; ///< @trick_io{**} The RTI parameter handle.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Parameter class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Parameter( Parameter const &rhs );

   /*! @brief Assignment operator for Parameter class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Parameter &operator=( Parameter const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_PARAMETER_HH
