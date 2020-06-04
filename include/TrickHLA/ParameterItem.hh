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
@trick_link_dependency{../source/TrickHLA/ParameterItem.cpp}
@trick_link_dependency{../source/TrickHLA/Item.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, Feb 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_PARAMETER_ITEM_HH_
#define _TRICKHLA_PARAMETER_ITEM_HH_

// System includes
#include <cstddef>

// HLA include files.
#include "TrickHLA/StandardsSupport.hh"
#include RTI1516_HEADER

// TrickHLA include files.
#include "TrickHLA/Item.hh"

namespace TrickHLA
{

class ParameterItem : public Item
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
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
   ParameterItem( int                                          parameter_index,
                  const RTI1516_NAMESPACE::VariableLengthData *param_value );

   /*! @brief Destructor for the TrickHLA ParameterItem class. */
   virtual ~ParameterItem();

  public:
   /*! @brief Frees allocated memory. */
   void clear();

  public:
   int index; ///< @trick_units{--} Index to the applicable parameter.

   size_t         size; ///< @trick_units{--} Number of bytes in the parameter data.
   unsigned char *data; ///< @trick_units{--} Parameter data.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ParameterItem class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ParameterItem( const ParameterItem &rhs );
   /*! @brief Assignment operator for ParameterItem class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ParameterItem &operator=( const ParameterItem &rhs );
};

} // namespace TrickHLA

#endif // _TRICKHLA_PARAMETER_ITEM_HH_
