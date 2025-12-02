/*!
@file TrickHLA/ObjectCallbackBase.hh
@ingroup TrickHLA
@brief Definition of the TrickHLA ObjectCallbackBase class.

This class is the abstract base class for object callbacks.

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
@trick_link_dependency{../../source/TrickHLA/ObjectCallbackBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/time/Int64Interval.cpp}
@trick_link_dependency{../../source/TrickHLA/time/Int64Time.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, September 2025, --, Initial implementation.}
@revs_end
*/

#ifndef TRICKHLA_OBJECT_CALLBACK_BASE_HH
#define TRICKHLA_OBJECT_CALLBACK_BASE_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/ObjectCallbackBase.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"

namespace TrickHLA
{

class Attribute;
class Object;

class ObjectCallbackBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ObjectCallbackBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA ObjectCallbackBase class. */
   ObjectCallbackBase();
   /*! @brief Constructor for the TrickHLA ObjectCallbackBase class with a callback name. */
   explicit ObjectCallbackBase( std::string name );
   /*! @brief Destructor for the TrickHLA ObjectCallbackBase class. */
   virtual ~ObjectCallbackBase();

   // Configure the packing object.
   /*! @brief Configure the TrickHLA ObjectCallbackBase object. */
   virtual void configure()
   {
      this->configured = true;
   }

   /*! @brief Check if the TrickHLA ObjectCallbackBase object is configured.
    *  @param status Configuration status. */
   virtual void set_configured( bool status = true )
   {
      this->configured = status;
   }

   /*! @brief Check if the TrickHLA ObjectCallbackBase object is configured. */
   virtual bool is_configured()
   {
      return ( this->configured );
   }

   // Initialize the packing object.
   /*! @brief Finish the initialization of the TrickHLA ObjectCallbackBase object. */
   virtual void initialize();

   /*! @brief Check if the TrickHLA ObjectCallbackBase object is initialized. */
   virtual bool is_initialized()
   {
      return ( this->initialized );
   }

   /*! @brief Initialize the callback object to the supplied Object pointer.
    *  @param obj Associated object for this class. */
   virtual void initialize_callback( Object *obj );

   /*! @brief Set the TrickHLA managed object associated with this callback object.
    *  @param obj Pointer to the associated TrickHLA managed Object. */
   virtual void set_object( Object *obj );

   /*! @brief Get the TrickHLA managed Object associated with this callback object.
    *  @return Pointer to the associated TrickHLA managed Object. */
   virtual Object *get_object()
   {
      return object;
   }

   /*! @brief Get the Attribute by FOM name.
    *  @return Attribute for the given name.
    *  @param attr_FOM_name Attribute FOM name. */
   Attribute *get_attribute( std::string const &attr_FOM_name );

   /*! @brief This function returns the Attribute for the given attribute FOM name.
    *  @return Attribute for the given name.
    *  @param attr_FOM_name Attribute FOM name. */
   Attribute *get_attribute_and_validate( std::string const &attr_FOM_name );

   /*! @brief Returns a copy of the object's lookahead time.
    *  @return A copy of the federate's lookahead time. */
   Int64Interval get_lookahead() const;

   /*! @brief Returns a copy of the object's granted federation time.
    *  @return A copy of the federate's current granted time. */
   Int64Time get_granted_time() const;

   /*! @brief Get the current scenario time.
    *  @return Returns the current scenario time. */
   double get_scenario_time() const;

   /*! @brief Get the current Central Timing Equipment (CTE) time.
    *  @return Returns the current CTE time. */
   double get_cte_time() const;

   std::string const &get_callback_name() const
   {
      return callback_name;
   }

  protected:
   /*! @brief Update the internal pointer to the Execution Control instance. */
   void update_exec_control_ptr();

   bool        configured;    ///< @trick_units{--} Configured status flag.
   bool        initialized;   ///< @trick_units{--} Initialization status flag.
   Object     *object;        ///< @trick_io{**} Object associated with this class.
   std::string callback_name; ///< @trick_io{**} Name of the object associated with this class.

   ExecutionControlBase *exec_control; ///< @trick_io{**} Execution control pointer.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ObjectCallbackBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ObjectCallbackBase( ObjectCallbackBase const &rhs );
   /*! @brief Assignment operator for ObjectCallbackBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ObjectCallbackBase &operator=( ObjectCallbackBase const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_OBJECT_CALLBACK_BASE_HH: Do NOT put anything after this line!
