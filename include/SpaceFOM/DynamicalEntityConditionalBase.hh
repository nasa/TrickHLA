/*!
@file SpaceFOM/DynamicalEntityConditionalBase.hh
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM DynamicalEntity objects.

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
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityConditionalBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, December 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_CONDITIONAL_BASE_HH
#define SPACEFOM_DYNAMICAL_ENTITY_CONDITIONAL_BASE_HH

// SpaceFOM includes.
#include "SpaceFOM/DynamicalEntityBase.hh"
#include "SpaceFOM/DynamicalEntityData.hh"
#include "SpaceFOM/PhysicalEntityConditionalBase.hh"

namespace SpaceFOM
{

class DynamicalEntityConditionalBase : public SpaceFOM::PhysicalEntityConditionalBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__DynamicalEntityConditionalBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Initialization constructor to associate with DynamicalEntity instance. */
   explicit DynamicalEntityConditionalBase( DynamicalEntityBase &entity_ref );
   /*! @brief Destructor. */
   virtual ~DynamicalEntityConditionalBase();

   /*! @brief Initializes the DynamicalEntity. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Conditional functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   /*! @brief Determines if the attribute has changed and returns the truth of
    *  that determination.
    *  @return True if value should be sent.
    *  @param attr Attribute to check. */
   virtual bool should_send( TrickHLA::Attribute *attr );

  protected:
   DynamicalEntityBase &de_entity;    ///< @trick_units{--} @trick_io{**} Associated DynamicalEntity.
   DynamicalEntityData  prev_de_data; ///< @trick_units{--} @trick_io{**} Previous comparison data.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *force_attr;        ///< @trick_io{**} Force Attribute.
   TrickHLA::Attribute *torque_attr;       ///< @trick_io{**} Torque Attribute.
   TrickHLA::Attribute *mass_attr;         ///< @trick_io{**} Mass Attribute.
   TrickHLA::Attribute *mass_rate_attr;    ///< @trick_io{**} Mass rate Attribute.
   TrickHLA::Attribute *inertia_attr;      ///< @trick_io{**} Inertia matrix Attribute.
   TrickHLA::Attribute *inertia_rate_attr; ///< @trick_io{**} Inertia rate Attribute.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for DynamicalEntityConditionalBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   DynamicalEntityConditionalBase( DynamicalEntityConditionalBase const &rhs );
   /*! @brief Assignment operator for DynamicalEntityConditionalBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   DynamicalEntityConditionalBase &operator=( DynamicalEntityConditionalBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_CONDITIONAL_BASE_HH: Do NOT put anything after this line!
