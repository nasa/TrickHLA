/*!
@file TrickHLA/ReportingServices.cpp
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

@tldh
@trick_link_dependency{ReportingServices.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{ObjectServices.cpp}
@trick_link_dependency{InteractionServices.cpp}

@revs_title
@revs_begin
@rev_entry{Nick Trevino, NASA ER7, TrickHLA, November 2026, --, Initial implementation of report generation.}
@revs_end

*/

// System include files.
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <regex>

// Trick includes.
#include "trick/command_line_protos.h"
#include "trick/message_proto.h"

// TrickHLA includes.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/ReportingServices.hh"

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details NOTE: In most cases, we would allocate and set default names in
 * the constructor. However, since we want this class to be Input Processor
 * friendly, we cannot do that here since the Input Processor may not have
 * been initialized yet. So, we have to set the name information to nullptr and
 * then allocate and set the defaults in the initialization job if not
 * already set in the input stream.
 *
 * @job_class{initialization}
 */
ReportingServices::ReportingServices( Federate &fed )
   : format_csv( *this ),
     //format_md( *this ),
     //format_latex( *this ),
     reports_directory( "" ),
     report_formats( static_cast<uint16_t>(THLAReportFormatEnum::REPORT_FORMAT_NONE) ),
     federate( &fed ),
     object_services( &(fed.object_service) ),
     interaction_service( &(fed.interaction_service) )
     
{
   return;
}

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
ReportingServices::~ReportingServices()
{
   return;
}

//----------------------------------------------------------------------------
// General ReportingService support functions.
//----------------------------------------------------------------------------

/*!
 *  @job_class{initialization}
 */
bool ReportingServices::set_reports_directory( std::string const &path )
{
   // If the save directory is not specified, set it to the current RUN directory
   if ( path.empty() ) {

      // Get the Trick provided RUN directory path information.
      string const run_dir = command_line_args_get_output_dir();
      string const def_dir = command_line_args_get_default_dir();

      // Build an absolute path to the RUN directory by combining default_dir
      // and run_dir from the EXECUTIVE.
      this->reports_directory = def_dir + "/" + run_dir;

   } else {
      this->reports_directory = path;
   }

   // Check if path is valid and return status.
   return ( this->check_reports_directory() );
}

/*!
 *  @job_class{scheduled}
 */
bool ReportingServices::check_reports_directory()
{
   struct stat info;

   // Check for the existence of the path and that it is a directory.
   if ( stat( this->reports_directory.c_str(), &info ) != 0 ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
         ostringstream errmsg;
         errmsg << "Save directory path '" << this->reports_directory
                << "' does NOT exist!\n";
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, errmsg.str(), MSG_ERROR );
      }

      return ( false );

   } else if ( ( info.st_mode & S_IFDIR ) == 0 ) { // NOLINT(misc-include-cleaner)

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
         ostringstream errmsg;
         errmsg << "Save directory path '" << this->reports_directory
                << "' exists but is NOT a directory!\n";
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, errmsg.str(), MSG_ERROR );
      }

      return ( false );
   }

   return ( true );
}

/*!
 *  @job_class{initialization}
 */
void ReportingServices::add_report( THLAReportFormatEnum format )
{
   // Add the format to the reporting formats bit field.
   report_formats |= static_cast<uint16_t>(format);

   return;
}

/*!
 *  @job_class{initialization}
 */
void ReportingServices::remove_report( THLAReportFormatEnum format )
{
   // Remove the format to the reporting formats bit field.
   report_formats &= ~(static_cast<uint16_t>(format));

   return;
}

/*!
 *  @job_class{initialization}
 */
bool ReportingServices::check_for_report( THLAReportFormatEnum format )
{
   // Check for special cases.
   if ( format == THLAReportFormatEnum::REPORT_FORMAT_NONE ){
      return( (report_formats == 0) ? true : false );
   }
   else if ( format == THLAReportFormatEnum::REPORT_FORMAT_ALL ){
      return(    (check_for_report( THLAReportFormatEnum::REPORT_FORMAT_CSV ))
              && (check_for_report( THLAReportFormatEnum::REPORT_FORMAT_MD ))
              && (check_for_report( THLAReportFormatEnum::REPORT_FORMAT_LATEX )) );
   }

   // Return true is format is found in the report formats bit field.
   return( ((report_formats & static_cast<uint16_t>(format)) ? true : false ) );
}

/*!
 *  @job_class{initialization}
 */
void ReportingServices::generate_reports()
{
   // Check to see if any reports are being generated.
   if ( check_for_report( THLAReportFormatEnum::REPORT_FORMAT_NONE) ) {
      return;
   }

   // Check for CSV format report generation.
   if ( check_for_report( THLAReportFormatEnum::REPORT_FORMAT_CSV ) ) {
      format_csv.generate_report();
   }

   // Check for Markdown format report generation.
   if ( check_for_report( THLAReportFormatEnum::REPORT_FORMAT_MD ) ) {
      //format_md.generate_report();
      ostringstream errmsg;
      errmsg << "Markdown format not yet implemented!\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, errmsg.str(), MSG_ERROR );
   }

   //// Check for LaTeX format report generation.
   if ( check_for_report( THLAReportFormatEnum::REPORT_FORMAT_LATEX ) ) {
      //format_latex.generate_report();
      ostringstream errmsg;
      errmsg << "LaTeX format not yet implemented!\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, errmsg.str(), MSG_ERROR );
   }

   return;
}


//-------------------------------------------------------------------------
// CheckpointConversionBase Interface.
//-------------------------------------------------------------------------

/*!
 *  @job_class{freeze}
 */
void ReportingServices::convert_data_before_checkpoint()
{
   format_csv.convert_data_before_checkpoint();
   //format_md.convert_data_before_checkpoint();
   //format_latex.convert_data_before_checkpoint();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ReportingServices::restore_data_after_checkpoint()
{
   format_csv.restore_data_after_checkpoint();
   //format_md.restore_data_after_checkpoint();
   //format_latex.restore_data_after_checkpoint();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ReportingServices::free_converted_data_for_checkpoint()
{
   format_csv.free_converted_data_for_checkpoint();
   //format_md.free_converted_data_for_checkpoint();
   //format_latex.free_converted_data_for_checkpoint();
   return;
}


