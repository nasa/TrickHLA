/*!
@file TrickHLA/ExecutionConfigurationBase.hh
@ingroup TrickHLA
@brief The abstract base class for the TrickHLA simulation execution
configuration class.

@details This class is used to provide the fundamentals for exchanging
startup, initialization, and run time configuration information between
participating federates in an HLA federation execution.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

\par<b>Assumptions and Limitations:</b>
- One and only one ExecutionConfigurationBase object should exist in an federation
execution.

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_EXECUTION_CONFIGURATION_BASE_HH
#define TRICKHLA_EXECUTION_CONFIGURATION_BASE_HH

// TrickHLA include files.
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"

namespace TrickHLA
{

// Forward declaration of the TrickHLA::Manager class.
// This is to resolve a circular reference.
class Manager;
class ExecutionControlBase;

class ExecutionConfigurationBase : public Object, public Packing
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ExecutionConfigurationBase();

  public:
   char const *S_define_name; /**< @trick_units{--}
      Full path name in the S_define for this ExecutionConfiguration instance. */

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the TrickHLA ExecutionConfigurationBase class. */
   ExecutionConfigurationBase();
   /*! @brief Initialization constructor for the TrickHLA ExecutionConfigurationBase class.
    *  @param s_define_name Full path name in the S_define for this ExecutionConfiguration instance. */
   explicit ExecutionConfigurationBase( char const *s_define_name );
   /*! @brief Pure virtual destructor for the TrickHLA ExecutionConfigurationBase class. */
   virtual ~ExecutionConfigurationBase() = 0;

   // Default data.
   /*! @brief Sets up the attributes for this Execution Configuration object
    *  using default values. These can be overridden in the input file.
    *  @param exec_control Reference to the associated TrickHLA::ExecutionControlBase object. */
   virtual void setup( TrickHLA::ExecutionControlBase &exec_control );

   /*! @brief Sets up the attributes for this Execution Configuration object
    *  using default values. These can be overridden in the input file. */
   virtual void configure_attributes() = 0;

   /*! @brief Configure the execution configuration object. */
   virtual void configure() = 0;

   /*! @brief Set the full path name in the S_define to the
    * ExecutionConfiguration object instance.
    *  @param new_name Full path name in the S_define for this ExecutionConfiguration instance. */
   virtual void set_S_define_name( char const *new_name );

   /*! @brief Get the full path name in the S_define to the
    * ExecutionConfiguration object instance.
    *  @return S_define_name Full path name in the S_define for this ExecutionConfiguration instance. */
   virtual char const *get_S_define_name()
   {
      return ( S_define_name );
   }

   // From the TrickHLA::Packing class.
   virtual void pack()   = 0;
   virtual void unpack() = 0;

   /*! @brief Resets the object and attribute preferred-order flags to Receive-Order. */
   virtual void reset_preferred_order();

   /*! @brief Resets the object and attribute ownership flags to locally owned
    * and enabled the CONFIG_TYPE_INITIALIZE flag for each attribute. */
   virtual void reset_ownership_states();

   /*! @brief The Execution Configuration is published by the master federate
    * and subscribed to by the non-master federates.
    *  @param is_master True if the master, false otherwise. */
   virtual void set_master( bool const is_master );

   // Execution configuration specific functions.
   /*! @brief Setup the Trick Ref Attributes for the ExecutionConfiguration object.
    *  @param packing_obj Associated packing object. */
   virtual void setup_ref_attributes( Packing *packing_obj ) = 0;

   /*! @brief Print the current Execution Configuration object to the console. */
   virtual void print_execution_configuration() const = 0;

   /*! @brief Waits for the registration of the ExecutionConfiguration
    * object instances with the RTI. */
   virtual void wait_for_registration();

   /*! @brief Wait for an Execution Configuration update.
    *  @return True for successful wait. */
   virtual bool wait_for_update();

   /*! @brief Check if an update is pending.
    *  @return True is an update is pending. */
   virtual bool update_pending()
   {
      return this->pending_update;
   }

   /*! @brief Clear the update pending flag. */
   virtual void clear_update_pending()
   {
      this->pending_update = false;
   }

   /*! @brief Get the reference to the associated TrickHLA::ExecutionControlBase object.
    *  @param exec_control Pointer to the associated TrickHLA::ExecutionControlBase object. */
   virtual void set_execution_control( ExecutionControlBase *exec_control )
   {
      execution_control = exec_control;
   }

   /*! @brief Get the reference to the associated TrickHLA::ExecutionControlBase object.
    *  @return Pointer to the associated TrickHLA::ExecutionControlBase object. */
   virtual ExecutionControlBase *get_execution_control()
   {
      return execution_control;
   }

  protected:
   bool pending_update; ///< @trick_units{--} Pending update flag.

   ExecutionControlBase *execution_control; /**< @trick_units{--}
      Associates TrickHLA::ExecutionConfigurationBase class object instance.
      Since this is an abstract class, the actual instance will be a concrete
      derived class instance (e.g. SRFOM:ExecutionControl). */

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ExecutionConfigurationBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ExecutionConfigurationBase( ExecutionConfigurationBase const &rhs );

   /*! @brief Assignment operator for ExecutionConfigurationBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ExecutionConfigurationBase &operator=( ExecutionConfigurationBase const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_EXECUTION_CONFIGURATION_BASE_HH: Do NOT put anything after this line!
