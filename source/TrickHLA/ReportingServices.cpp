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
@trick_link_dependency{Types.cpp}
@trick_link_dependency{utils/SleepTimeout.cpp}
@trick_link_dependency{utils/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Nick Trevino, NASA ER7, TrickHLA, November 2026, --, Initial implementation of report generation.}
@revs_end

*/

// System include files.
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <regex>

// Trick includes.
// #include "trick/CheckPointRestart.hh"
// #include "trick/Memoryobject_service.hh"
#include "trick/command_line_protos.h"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/sim_mode.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh" // IWYU pragma: keep
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/InteractionServices.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/ObjectServices.hh"
#include "TrickHLA/ReportingServices.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/time/TimeManagementServices.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/StringUtilities.hh"
#include "TrickHLA/utils/Utilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/Exception.h"
#include "RTI/Handle.h"
#include "RTI/RTIambassador.h"
#include "RTI/Typedefs.h"
#include "RTI/time/HLAinteger64Time.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

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
   : federate( &fed ),
     object_service( fed.get_object_service() ),
     interaction_service( fed.get_interaction_service() ),
     time_management_service( fed.get_time_management_service() ),
     execution_control( nullptr ),
     HLA_save_directory( "" ),
     do_generate_report( true )
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
bool ReportingServices::set_HLA_save_directory( std::string const &path )
{
   // If the save directory is not specified, set it to the current RUN directory
   if ( path.empty() ) {

      // Get the Trick provided RUN directory path information.
      string const run_dir = command_line_args_get_output_dir();
      string const def_dir = command_line_args_get_default_dir();

      // Build an absolute path to the RUN directory by combining default_dir
      // and run_dir from the EXECUTIVE.
      this->HLA_save_directory = def_dir + "/" + run_dir;

   } else {
      this->HLA_save_directory = path;
   }

   // Check if path is valid and return status.
   return ( this->check_HLA_save_directory() );
}

/*!
 *  @job_class{scheduled}
 */
bool ReportingServices::check_HLA_save_directory()
{
   struct stat info;

   // Check for the existence of the path and that it is a directory.
   if ( stat( this->HLA_save_directory.c_str(), &info ) != 0 ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE_SERVICES ) ) {
         ostringstream errmsg;
         errmsg << "Save directory path '" << this->HLA_save_directory
                << "' does NOT exist!\n";
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, errmsg.str(), MSG_ERROR );
      }

      return ( false );

   } else if ( ( info.st_mode & S_IFDIR ) == 0 ) { // NOLINT(misc-include-cleaner)

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE_SERVICES ) ) {
         ostringstream errmsg;
         errmsg << "Save directory path '" << this->HLA_save_directory
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
void ReportingServices::generate_report()
{
   if ( do_generate_report ) {
      ostringstream msg;

      // Print Known Federates
      msg << endl << "Known Federates:" << endl;
      if ( federate->enable_known_feds ) {
         if ( federate->known_federates.size() == 0 ) {
               msg << "\tNo Known Federates" << endl;
         } else {
               for ( long unsigned int i = 0; i < federate->known_federates.size(); ++i ) {
                  msg << "\t" << federate->known_federates[i].name.c_str() << endl;
               }
         }
      } else {
         msg << "\tDisabled" << endl;
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
      msg.str("");
      msg.clear();

      // Print FOM Modules
      msg << endl << "FOM Modules:" << endl;
      if ( federate->FOM_modules.empty() ) {
         msg << "\tNo FOM Modules" << endl;
      } else {
         msg << "\t" << regex_replace(federate->FOM_modules, regex(", "), "\n\t") << endl;
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
      msg.str("");
      msg.clear();

      // Print HLA Objects
      msg << endl << "Objects:" << endl;
      if ( object_service->obj_count == 0 ) {
         msg << "\tNo Objects" << endl;
      } else {
         for ( int i = 0; i < object_service->obj_count; ++i ) {
               msg << "\t" << object_service->objects[i].get_FOM_name() << endl;
               msg << "\t" << object_service->objects[i].get_name() << endl;

               unsigned short int padding_width = 0;
               for ( int j = 0; j < object_service->objects[i].get_attribute_count(); ++j ) {
                  if ( object_service->objects[i].get_attributes()[j].get_FOM_name().length() > padding_width ) {
                     padding_width = object_service->objects[i].get_attributes()[j].get_FOM_name().length();
                  }
               }

               for ( int j = 0; j < object_service->objects[i].get_attribute_count(); ++j ) {
                  msg << "\t\t";
                  if ( object_service->objects[i].get_attributes()[j].is_publish()   ) { msg << "p"; }
                  if ( object_service->objects[i].get_attributes()[j].is_subscribe() ) { msg << "s"; }

                  string att_FOM_name = object_service->objects[i].get_attributes()[j].get_FOM_name();
                  msg << "\t" << att_FOM_name.append(padding_width - att_FOM_name.length(), ' ');
                  msg << "\t" << object_service->objects[i].get_attributes()[j].get_trick_name() << endl;
               }
         }
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
      msg.str("");
      msg.clear();

      // Print HLA Interactions
      msg << endl << "Interactions:" << endl;
      if ( interaction_service->inter_count == 0 ) {
         msg << "\tNo Interactions" << endl;
      } else {
         for ( int i = 0; i < interaction_service->inter_count; ++i ) {
            msg << "\t";
            if ( interaction_service->interactions[i].is_publish()  ) { msg << "p"; }
            if ( interaction_service->interactions[i].is_subscribe()) { msg << "s"; }
            msg << "\t" << interaction_service->interactions[i].get_FOM_name() << endl;

            msg << "\t" << "Parameters:" << endl;
            for ( int j = 0; j < interaction_service->interactions[i].get_parameter_count(); ++j ) {
               msg << "\t\t" << interaction_service->interactions[i].get_parameters()[j].get_FOM_name();
               msg << "\t"   << interaction_service->interactions[i].get_parameters()[j].get_trick_name();
            }
         }
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
      msg.str("");
      msg.clear();

      // Write HLA Objects to CSV file
      if ( object_service->obj_count > 0 ) {
         string file_name = get_HLA_save_directory() + "/" + federate->get_federation_name() + "__" + federate->get_federate_name() + "__objects.csv";
         ofstream file(file_name);

         file << "Object FOM Name,";
         file << "Object Name,";
         file << "Attribute FOM Name,";
         file << federate->get_federate_name() << " Publish"   << ",";
         file << federate->get_federate_name() << " Subscribe" << ",";
         file << federate->get_federate_name()                 << endl;

         for ( int i = 0; i < object_service->obj_count; ++i ) {
            for ( int j = 0; j < object_service->objects[i].get_attribute_count(); ++j ) {
                  file << object_service->objects[i].get_FOM_name() << "." << object_service->objects[i].get_name() << "." << object_service->objects[i].get_attributes()[j].get_FOM_name() << ",";
                  file << object_service->objects[i].get_FOM_name()                                          << ",";
                  file << object_service->objects[i].get_name()                                              << ",";
                  file << object_service->objects[i].get_attributes()[j].get_FOM_name()                      << ",";
                  file <<(object_service->objects[i].get_attributes()[j].is_publish()    ? "True" : "False") << ",";
                  file <<(object_service->objects[i].get_attributes()[j].is_subscribe()  ? "True" : "False") << ",";
                  file << object_service->objects[i].get_attributes()[j].get_trick_name()                    << endl;
            }
         }

         file.close();

         msg << "List of HLA Objects written to " + file_name << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
         msg.str("");
         msg.clear();
      }

      // Write HLA Interactions to CSV file
      if ( interaction_service->inter_count > 0 ) {
         string file_name = get_HLA_save_directory() + "/" + federate->get_federation_name() + "__" + federate->get_federate_name() + "__interactions.csv";
         ofstream file(file_name);

         // TODO
         file << "TO DO";
         file.close();

         msg << "List of HLA Interactions written to " + file_name << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }

   return;
}

