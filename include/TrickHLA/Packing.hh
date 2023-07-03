/*!
@file TrickHLA/Packing.hh
@ingroup TrickHLA
@brief Definition of the TrickHLA Packing class.

This class is the abstract base class for packing before data is sent to
the RTI and unpacking just after data is received from the RTI.

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
@trick_link_dependency{../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../source/TrickHLA/Object.cpp}
@trick_link_dependency{../source/TrickHLA/Packing.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial version.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Sept 2009, --, Updated Packing API.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Oct 2009, --, Added get attribute function.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end
*/

#ifndef TRICKHLA_PACKING_HH
#define TRICKHLA_PACKING_HH

// TrickHLA include files.
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Object;
class Attribute;

class Packing
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Packing();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Packing class. */
   Packing();
   /*! @brief Destructor for the TrickHLA Packing class. */
   virtual ~Packing();

   /*! @brief Get the Attribute by FOM name.
    *  @return Attribute for the given name.
    *  @param attr_FOM_name Attribute FOM name. */
   Attribute *get_attribute( char const *attr_FOM_name );

   /*! @brief This function returns the Attribute for the given attribute FOM name.
    *  @return Attribute for the given name.
    *  @param attr_FOM_name Attribute FOM name. */
   Attribute *get_attribute_and_validate( char const *attr_FOM_name );

   /*! @brief Get the current scenario time.
    *  @return Returns the current scenario time. */
   double get_scenario_time();

   /*! @brief Get the current Central Timing Equipment (CTE) time.
    *  @return Returns the current CTE time. */
   double get_cte_time();

   /*! @brief Initialize the callback object to the supplied Object pointer.
    *  @param obj Associated object for this class. */
   virtual void initialize_callback( Object *obj );

   //-----------------------------------------------------------------
   // These are virtual functions and must be defined by a full class.
   //-----------------------------------------------------------------

   /*! @brief Pack the data before being sent. */
   virtual void pack() = 0;

   /*! @brief Unpack the received data. The default */
   virtual void unpack() = 0;

  protected:
   Object *object; ///< @trick_io{**} Object associated with this packing class.

   /*! @brief Uses Trick memory allocation routines to allocate a new string
    *  that is input file compliant. */
   char *allocate_input_string( char const *c_string );
   /*! @brief Uses Trick memory allocation routines to allocate a new string
    *  that is input file compliant. */
   char *allocate_input_string( std::string const &cpp_string );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Packing class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Packing( Packing const &rhs );
   /*! @brief Assignment operator for Packing class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Packing &operator=( Packing const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_PACKING_HH: Do NOT put anything after this line!
