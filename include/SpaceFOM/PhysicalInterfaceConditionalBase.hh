/*!
@file SpaceFOM/PhysicalInterfaceConditionalBase.hh
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM PhysicalInterface objects.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Conditional.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalInterfaceConditionalBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_INTERFACE_CONDITIONAL_BASE_HH
#define SPACEFOM_PHYSICAL_INTERFACE_CONDITIONAL_BASE_HH

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/Object.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalInterfaceBase.hh"

namespace SpaceFOM
{

class PhysicalInterfaceConditionalBase : public TrickHLA::Conditional
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalInterfaceConditionalBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Initialization constructor to associate with PhysicalInterface instance. */
   PhysicalInterfaceConditionalBase( PhysicalInterfaceBase &interface_ref );
   /*! @brief Destructor. */
   virtual ~PhysicalInterfaceConditionalBase();

   /*! @brief Initializes the sim_data to the supplied.
    *  @param data External simulation data. */
   void initialize( );

   /*! @brief Initialization callback as part of the TrickHLA::Conditional functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   /*! @brief Determines if the attribute has changed and returns the truth of
    *  that determination.
    *  @return True if value should be sent.
    *  @param attr Attribute to check. */
   virtual bool should_send( TrickHLA::Attribute *attr );

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   PhysicalInterfaceBase &interface; ///< @trick_units{--} @trick_io{**} Associated PhysicalInterface.
   PhysicalInterfaceData  prev_data; ///< @trick_units{--} @trick_io{**} Previous comparison data.

   bool initialized; ///< @trick_units{--} Initialization indication flag.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;     ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *parent_attr;   ///< @trick_io{**} Parent entity or interface Attribute.
   TrickHLA::Attribute *position_attr; ///< @trick_io{**} Position Attribute.
   TrickHLA::Attribute *attitude_attr; ///< @trick_io{**} Attitude Attribute.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for PhysicalInterfaceConditionalBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalInterfaceConditionalBase( PhysicalInterfaceConditionalBase const &rhs );
   /*! @brief Assignment operator for PhysicalInterfaceConditionalBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalInterfaceConditionalBase &operator=( PhysicalInterfaceConditionalBase const &rhs );

};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_INTERFACE_CONDITIONAL_BASE_HH: Do NOT put anything after this line!
