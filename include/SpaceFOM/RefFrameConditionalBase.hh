/*!
@file SpaceFOM/RefFrameConditionalBase.hh
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM RefFrame objects.

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
@trick_link_dependency{../../source/SpaceFOM/RefFrameConditionalBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_CONDITIONAL_BASE_HH
#define SPACEFOM_REF_FRAME_CONDITIONAL_BASE_HH

// TrickHLA includes.
#include "../TrickHLA/Conditional.hh"

// SpaceFOM includes.
#include "RefFrameBase.hh"
#include "RefFrameData.hh"

namespace SpaceFOM
{

class RefFrameConditionalBase : public TrickHLA::Conditional
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameConditionalBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Initialization constructor to associate with RefFrame instance. */
   explicit RefFrameConditionalBase( RefFrameBase &entity_ref );
   /*! @brief Destructor. */
   virtual ~RefFrameConditionalBase();

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
   RefFrameBase &frame;     ///< @trick_units{--} @trick_io{**} Associated RefFrame.
   RefFrameData  prev_data; ///< @trick_units{--} @trick_io{**} Previous comparison data.

   TrickHLA::Attribute *name_attr;        ///< @trick_io{**} Reference frame name Attribute.
   TrickHLA::Attribute *parent_name_attr; ///< @trick_io{**} Parent reference frame name Attribute.
   TrickHLA::Attribute *state_attr;       ///< @trick_io{**} Reference frame state Attribute.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for RefFrameConditionalBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameConditionalBase( RefFrameConditionalBase const &rhs );
   /*! @brief Assignment operator for RefFrameConditionalBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameConditionalBase &operator=( RefFrameConditionalBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_CONDITIONAL_BASE_HH: Do NOT put anything after this line!
