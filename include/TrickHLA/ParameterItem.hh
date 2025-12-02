/*!
@file TrickHLA/ParameterItem.hh
@ingroup TrickHLA
@brief This class represents a queue for holding HLA parameters.

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
@trick_link_dependency{../../source/TrickHLA/ParameterItem.cpp}
@trick_link_dependency{../../source/TrickHLA/Item.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, Feb 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_PARAMETER_ITEM_HH
#define TRICKHLA_PARAMETER_ITEM_HH

// TrickHLA includes.
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Item.hh"

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

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class ParameterItem : public Item
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ParameterItem();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA ParameterItem class. */
   ParameterItem();
   /*! @brief Initialization constructor for the TrickHLA ParameterItem class.
    *  @param parameter_index Parameter index.
    *  @param param_value HLA RTI Parameter value. */
   ParameterItem( int const                                    parameter_index,
                  RTI1516_NAMESPACE::VariableLengthData const *param_value );

   /*! @brief Destructor for the TrickHLA ParameterItem class. */
   virtual ~ParameterItem();

  public:
   /*! @brief Frees allocated memory. */
   void clear();

  public:
   int index; ///< @trick_units{--} Index to the applicable parameter.

   int            size; ///< @trick_units{--} Number of bytes in the parameter data.
   unsigned char *data; ///< @trick_units{--} Parameter data.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ParameterItem class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ParameterItem( ParameterItem const &rhs );
   /*! @brief Assignment operator for ParameterItem class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ParameterItem &operator=( ParameterItem const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_PARAMETER_ITEM_HH
