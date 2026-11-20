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
@trick_link_dependency{../../source/TrickHLA/ReportingServices.cpp}

@revs_title
@revs_begin
@rev_entry{Nick Trevino, NASA ER7, TrickHLA, November 2026, --, Initial implementation of report generation.}
@revs_end

*/

#ifndef TRICKHLA_REPORTING_SERVICES_HH
#define TRICKHLA_REPORTING_SERVICES_HH

// System includes.
#include <string>
#include <cstdint>

// Trick includes.

// TrickHLA includes.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/ReportFormatCSV.hh"

namespace TrickHLA
{

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class TimeManagementServices;
class ObjectServices;
class InteractionServices;
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

   friend class ReportFormatCSV;
   friend class ReportFormatMD;
   friend class ReportFormatLaTeX;

  public:

   //--------------------------------------------------------------------------
   // User accessible data.
   //--------------------------------------------------------------------------
   //
   // Known report formats.
   //
   ReportFormatCSV   format_csv; ///< @trick_units{--} CSV report generation instance.
   //ReportFormatMD    format_md; ///< @trick_units{--} Markdown report generation instance.
   //ReportFormatLaTeX format_latex; ///< @trick_units{--} LaTeX report generation instance.

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
   /*! @brief Set the directory in which to generate the reports.
    * @return Success of setting the reports directory path.
    * @detail If a path isn't provided, then a default path is constructed. */
   bool set_reports_directory()
   {
      return set_reports_directory( "" );
   }

   /*! @brief Set the directory in which to generate the reports.
    * @return Success of setting the reports directory path.
    * @detail If a path isn't provided, then a default path is constructed.
    * @param  path Full path to the reports directory.  */
   bool set_reports_directory( std::string const &path );

   /*! @brief Get the path to the reports directory.
    * @return Path to the reports directory. */
   std::string const &get_reports_directory()
   {
      return reports_directory;
   }

   /*! @brief Check if reports_directory is empty. If so, ask the EXECUTIVE
    * for info and build the absolute path of the RUN directory.
    * @return Success of HLA Save directory path. */
   bool check_reports_directory();
   
   /*! @brief Add a report format to generate.
    *  @param format Rreport format to add. */
   void add_report( THLAReportFormatEnum format );

   /*! @brief Remove a report format to generate.
    *  @param format Report format to remove. */
   void remove_report( THLAReportFormatEnum format );

   /*! @brief Check for a report format.
    *  @param format Report format to check for.
    *  @return Returns true if report format is set in the specified report formats. */
   bool check_for_report( THLAReportFormatEnum format );

   //..........................................................................
   // Report Generation functions.
   //..........................................................................

   /*! @brief Function called to generate all requested reports. */
   void generate_reports();


   //-------------------------------------------------------------------------
   // CheckpointConversionBase Interface.
   //-------------------------------------------------------------------------

   /*! @brief Convert data to a form Trick can checkpoint. */
   virtual void convert_data_before_checkpoint() override;

   /*! @brief Restore data structures after loading a Trick checkpoint. */
   virtual void restore_data_after_checkpoint() override;

   /*! @brief Clear/release the memory used for the conversion data for the checkpoint. */
   virtual void free_converted_data_for_checkpoint() override;

  protected:

   std::string reports_directory; ///< @trick_units{--} Path to the reports directory.
   uint16_t    report_formats;    ///< @trick_units{1} Report formats to generate.

   //
   // References to the Federate and associated services.
   //
   Federate            *federate;            ///< @trick_units{--} Associated TrickHLA::Federate.
   ObjectServices      *object_services;     ///< @trick_units{--} Object services associated with the federate.
   InteractionServices *interaction_service; ///< @trick_units{--} Interaction services associated with the federate.

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
