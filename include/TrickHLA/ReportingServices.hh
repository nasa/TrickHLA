/*!
@file TrickHLA/ReportingServices.hh
@ingroup TrickHLA
@brief This class provides basic HLA save and restore services for a federate.

@copyright Copyright 2026 United States Government as represented by the
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
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/ReportingServices.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/time/TimeManagementServices.cpp}

@revs_title
@revs_begin
@rev_entry{Nick Trevino, NASA ER7, TrickHLA, November 2026, --, Initial implementation of report generation.}
@revs_end

*/

#ifndef TRICKHLA_REPORTING_SERVICES_HH
#define TRICKHLA_REPORTING_SERVICES_HH

// System includes.
#include <cstdint>

// TrickHLA includes.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/time/Int64Time.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/Handle.h"
#include "RTI/RTI1516.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class ObjectServices;
class InteractionServices;
class TimeManagementServices;
class ExecutionControlBase;

class ReportingServices : public CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ReportingServices();

   // Allow the TrickHLA core classes to have direct access to protected
   // and private data.
   friend class Federate;
   friend class ExecutionControlBase;
   friend class FedAmb;

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Federate class.
    *  @param fed Associated Federate instance. */
   explicit ReportingServices( Federate &fed );
   /*! @brief Destructor for the TrickHLA ReportingServices class. */
   virtual ~ReportingServices() override;

   //--------------------------------------------------------------------------
   // Principal public ReportingServices functions.
   //--------------------------------------------------------------------------

   //
   // General support functions.
   //
   /*! @brief Set the HLA save directory.
    * @return Success of setting the HLA Save directory path.
    * @detail If a path isn't provided, then a default path is constructed. */
   bool set_HLA_save_directory()
   {
      return set_HLA_save_directory( "" );
   }

   /*! @brief Set the HLA save directory.
    * @return Success of setting the HLA Save directory path.
    * @detail If a path isn't provided, then a default path is constructed.
    * @param  path Full path to the HLA Save directory.  */
   bool set_HLA_save_directory( std::string const &path );

   /*! @brief Get the HLA save directory.
    * @return HLA save directory. */
   std::string const &get_HLA_save_directory()
   {
      return HLA_save_directory;
   }

   /*! @brief Check if HLA_save_directory is empty. If so, ask the EXECUTIVE
    * for info and build the absolute path of the RUN directory.
    * @return Success of HLA Save directory path. */
   bool check_HLA_save_directory();

   //..........................................................................
   // Report Generation functions.
   //..........................................................................

   /*! @brief Function called to generate the report. */
   void generate_report();

   //-------------------------------------------------------------------------
   // CheckpointConversionBase Interface.
   //-------------------------------------------------------------------------

   /*! @brief Convert data to a form Trick can checkpoint. */
   virtual void convert_data_before_checkpoint() override
   {
      return;
   }

   /*! @brief Restore data structures after loading a Trick checkpoint. */
   virtual void restore_data_after_checkpoint() override
   {
      return;
   }

   /*! @brief Clear/release the memory used for the conversion data for the checkpoint. */
   virtual void free_converted_data_for_checkpoint() override
   {
      return;
   }

  protected:
   //
   // References to the Federate and associated services.
   //
   Federate               *federate;                ///< @trick_units{--} Associated TrickHLA::Federate.
   ObjectServices         *object_service;          ///< @trick_units{--} Associated ObjectServices.
   InteractionServices    *interaction_service;     ///< @trick_units{--} Associated InteractionServices.
   TimeManagementServices *time_management_service; ///< @trick_units{--} Associated TrickHLA::TimeManagementServices.
   ExecutionControlBase   *execution_control;       ///< @trick_units{--} Associated TrickHLA::ExecutionControlBase.

   // Reporting variables.
   std::string HLA_save_directory; ///< @trick_units{--} HLA Save directory.
   bool        do_generate_report; ///< @trick_units{--} Whether or not to generate the report.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ReportingServices class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ReportingServices( ReportingServices const &rhs );

   /*! @brief Assignment operator for ReportingServices class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ReportingServices &operator=( ReportingServices const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_REPORTING_SERVICES_HH
