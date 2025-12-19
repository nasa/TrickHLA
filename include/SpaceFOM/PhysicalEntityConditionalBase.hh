/*!
@file SpaceFOM/PhysicalEntityConditionalBase.hh
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM PhysicalEntity objects.

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
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityConditionalBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_CONDITIONAL_BASE_HH
#define SPACEFOM_PHYSICAL_ENTITY_CONDITIONAL_BASE_HH

// SpaceFOM includes.
#include "SpaceFOM/PhysicalEntityBase.hh"
#include "SpaceFOM/PhysicalEntityData.hh"

// TrickHLA includes.
#include "TrickHLA/Conditional.hh"

namespace SpaceFOM
{

class PhysicalEntityConditionalBase : public TrickHLA::Conditional
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalEntityConditionalBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Initialization constructor to associate with PhysicalEntity instance. */
   explicit PhysicalEntityConditionalBase( PhysicalEntityBase &entity_ref );
   /*! @brief Destructor. */
   virtual ~PhysicalEntityConditionalBase();

   /*! @brief Initializes the conditional. */
   virtual void initialize();

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
   PhysicalEntityBase &entity;    ///< @trick_units{--} @trick_io{**} Associated PhysicalEntity.
   PhysicalEntityData  prev_data; ///< @trick_units{--} @trick_io{**} Previous comparison data.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;         ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *type_attr;         ///< @trick_io{**} Type type Attribute.
   TrickHLA::Attribute *status_attr;       ///< @trick_io{**} Status Attribute.
   TrickHLA::Attribute *parent_frame_attr; ///< @trick_io{**} Parent reference frame Attribute.
   TrickHLA::Attribute *state_attr;        ///< @trick_io{**} State Attribute.
   TrickHLA::Attribute *accel_attr;        ///< @trick_io{**} Acceleration Attribute.
   TrickHLA::Attribute *ang_accel_attr;    ///< @trick_io{**} Angular acceleration Attribute.
   TrickHLA::Attribute *cm_attr;           ///< @trick_io{**} Center of mass Attribute.
   TrickHLA::Attribute *body_frame_attr;   ///< @trick_io{**} Body frame orientation Attribute.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for PhysicalEntityConditionalBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalEntityConditionalBase( PhysicalEntityConditionalBase const &rhs );
   /*! @brief Assignment operator for PhysicalEntityConditionalBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalEntityConditionalBase &operator=( PhysicalEntityConditionalBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_CONDITIONAL_BASE_HH: Do NOT put anything after this line!
