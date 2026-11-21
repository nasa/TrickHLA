/*!
@file TrickHLA/ReportFormatCSV.cpp
@ingroup TrickHLA
@brief A concrete report generation class for CSV formated reports.

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
@trick_link_dependency{ReportFormatCSV.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Interaction.cpp}
@trick_link_dependency{InteractionServices.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{ObjectServices.cpp}
@trick_link_dependency{Parameter.cpp}
@trick_link_dependency{ReportFormatBase.cpp}
@trick_link_dependency{ReportFormatCSV.cpp}
@trick_link_dependency{ReportingServices.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2026, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/InteractionServices.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectServices.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/ReportFormatBase.hh"
#include "TrickHLA/ReportFormatCSV.hh"
#include "TrickHLA/ReportingServices.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ReportFormatCSV::ReportFormatCSV( ReportingServices &service )
   : ReportFormatBase( service )
{
   format = THLAReportFormatEnum::REPORT_FORMAT_CSV;
   return;
}

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
ReportFormatCSV::~ReportFormatCSV()
{
   return;
}

//----------------------------------------------------------------------------
// General report support functions.
//----------------------------------------------------------------------------

/*!
 *  @job_class{initialization}
 */
void ReportFormatCSV::generate_report()
{
   ostringstream msg;

   Federate                  *federate            = reporting_service->federate;
   ObjectServices const      *object_service      = reporting_service->object_services;
   InteractionServices const *interaction_service = reporting_service->interaction_service;

   // Open the report files.
   open_report_files();

   //
   // Check for Federation level report topics.
   //
   if ( has_report_topic( THLAReportTopicsEnum::REPORT_TOPICS_FEDERATION ) ) {

      // Print Known Federates
      msg << "\nKnown Federates:\n";
      if ( federate->enable_known_feds ) {
         if ( federate->known_federates.size() == 0 ) {
            msg << "\tNo Known Federates\n";
         } else {
            for ( long unsigned int i = 0; i < federate->known_federates.size(); ++i ) {
               msg << "\t" << federate->known_federates[i].name.c_str() << '\n';
            }
         }
      } else {
         msg << "\tDisabled\n";
      }
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str(), MSG_NORMAL );
      }
      msg.str( "" );
      msg.clear();
   }

   //
   // Check for Federate level report topics.
   //
   if ( has_report_topic( THLAReportTopicsEnum::REPORT_TOPICS_FEDERATE ) ) {

      // Print FOM Modules
      msg << "\nFOM Modules:\n";
      if ( federate->FOM_modules.empty() ) {
         msg << "\tNo FOM Modules\n";
      } else {
         msg << "\t" << regex_replace( federate->FOM_modules, regex( ", " ), "\n\t" ) << '\n';
      }
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str(), MSG_NORMAL );
      }
      msg.str( "" );
      msg.clear();
   }

   //
   // Check for Object level report topics.
   //
   if ( has_report_topic( THLAReportTopicsEnum::REPORT_TOPICS_OBJECTS ) ) {

      // Print HLA Objects
      msg << "\nObjects:\n";
      if ( object_service->obj_count == 0 ) {
         msg << "\tNo Objects\n";
      } else {
         for ( int i = 0; i < object_service->obj_count; ++i ) {
            msg << "\t" << object_service->objects[i].get_FOM_name() << '\n';
            msg << "\t" << object_service->objects[i].get_name() << '\n';

            unsigned short int padding_width = 0;
            for ( int j = 0; j < object_service->objects[i].get_attribute_count(); ++j ) {
               if ( object_service->objects[i].get_attributes()[j].get_FOM_name().length() > padding_width ) {
                  padding_width = object_service->objects[i].get_attributes()[j].get_FOM_name().length();
               }
            }

            for ( int j = 0; j < object_service->objects[i].get_attribute_count(); ++j ) {
               msg << "\t\t";
               if ( object_service->objects[i].get_attributes()[j].is_publish() ) {
                  msg << "p";
               }
               if ( object_service->objects[i].get_attributes()[j].is_subscribe() ) {
                  msg << "s";
               }

               string att_FOM_name = object_service->objects[i].get_attributes()[j].get_FOM_name();
               msg << "\t" << att_FOM_name.append( padding_width - att_FOM_name.length(), ' ' );
               msg << "\t" << object_service->objects[i].get_attributes()[j].get_trick_name() << '\n';
            }
         }
      }
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str(), MSG_NORMAL );
      }
      msg.str( "" );
      msg.clear();
   }

   //
   // Check for Interaction level report topics.
   //
   if ( has_report_topic( THLAReportTopicsEnum::REPORT_TOPICS_INTERACTIONS ) ) {

      // Print HLA Interactions
      msg << '\n'
          << "Interactions:\n";
      if ( interaction_service->inter_count == 0 ) {
         msg << "\tNo Interactions\n";
      } else {
         for ( int i = 0; i < interaction_service->inter_count; ++i ) {
            msg << "\t";
            if ( interaction_service->interactions[i].is_publish() ) {
               msg << "p";
            }
            if ( interaction_service->interactions[i].is_subscribe() ) {
               msg << "s";
            }
            msg << "\t" << interaction_service->interactions[i].get_FOM_name() << '\n';

            msg << "\t" << "Parameters:\n";
            for ( int j = 0; j < interaction_service->interactions[i].get_parameter_count(); ++j ) {
               msg << "\t\t" << interaction_service->interactions[i].get_parameters()[j].get_FOM_name();
               msg << "\t" << interaction_service->interactions[i].get_parameters()[j].get_trick_name();
            }
         }
      }
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str(), MSG_NORMAL );
      }
      msg.str( "" );
      msg.clear();
   }

   //
   // Check for Object level report topics.
   //
   if ( has_report_topic( THLAReportTopicsEnum::REPORT_TOPICS_OBJECTS ) ) {

      // Write HLA Objects to CSV file
      if ( object_service->obj_count > 0 ) {

         objects_file << "Object FOM Name,";
         objects_file << "Object Name,";
         objects_file << "Attribute FOM Name,";
         objects_file << federate->get_federate_name() << " Publish" << ",";
         objects_file << federate->get_federate_name() << " Subscribe" << ",";
         objects_file << federate->get_federate_name() << '\n';

         for ( int i = 0; i < object_service->obj_count; ++i ) {
            for ( int j = 0; j < object_service->objects[i].get_attribute_count(); ++j ) {
               objects_file << object_service->objects[i].get_FOM_name() << "." << object_service->objects[i].get_name() << "." << object_service->objects[i].get_attributes()[j].get_FOM_name() << ",";
               objects_file << object_service->objects[i].get_FOM_name() << ",";
               objects_file << object_service->objects[i].get_name() << ",";
               objects_file << object_service->objects[i].get_attributes()[j].get_FOM_name() << ",";
               objects_file << ( object_service->objects[i].get_attributes()[j].is_publish() ? "True" : "False" ) << ",";
               objects_file << ( object_service->objects[i].get_attributes()[j].is_subscribe() ? "True" : "False" ) << ",";
               objects_file << object_service->objects[i].get_attributes()[j].get_trick_name() << '\n';
            }
         }

         msg << "List of HLA Objects written to " + objects_file_name << '\n';
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
            DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str(), MSG_NORMAL );
         }
         msg.str( "" );
         msg.clear();
      }
   }

   //
   // Check for Interaction level report topics.
   //
   if ( has_report_topic( THLAReportTopicsEnum::REPORT_TOPICS_INTERACTIONS ) ) {

      // Write HLA Interactions to CSV file
      if ( interaction_service->inter_count > 0 ) {

         // TODO
         interactions_file << "TO DO";

         msg << "List of HLA Interactions written to " + interactions_file_name << '\n';
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_REPORTING_SERVICES ) ) {
            DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str(), MSG_NORMAL );
         }
      }
   }

   // Close the report files.
   close_report_files();

   return;
}

/*!
 *  @job_class{initialization}
 */
void ReportFormatCSV::open_report_files()
{
   Federate const            *federate            = reporting_service->federate;
   ObjectServices const      *object_service      = reporting_service->object_services;
   InteractionServices const *interaction_service = reporting_service->interaction_service;

   if ( object_service->obj_count > 0 ) {
      string const obj_file_name = reporting_service->get_reports_directory() + "/"
                                   + federate->get_federation_name() + "__"
                                   + federate->get_federate_name() + "__objects.csv";
      objects_file.open( obj_file_name );
   }

   if ( interaction_service->inter_count > 0 ) {
      string const inter_file_name = reporting_service->get_reports_directory() + "/"
                                     + federate->get_federation_name() + "__"
                                     + federate->get_federate_name() + "__interactions.csv";
      interactions_file.open( inter_file_name );
   }
   return;
}

/*!
 *  @job_class{initialization}
 */
void ReportFormatCSV::close_report_files()
{
   objects_file.close();
   interactions_file.close();
   return;
}
