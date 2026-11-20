/*!
@file TrickHLA/ReportFormatBase.cpp
@ingroup TrickHLA
@brief A base report generation class for formated reports.

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
@trick_link_dependency{ReportFormatBase.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{ObjectServices.cpp}
@trick_link_dependency{InteractionServices.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2026, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <regex>

// Trick includes.
#include "trick/message_proto.h"

// TrickHLA includes.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/ReportFormatBase.hh"

using namespace std;
using namespace TrickHLA;

//-----------------------------------------------------------
// ReportFormat enumeration support functions.
//-----------------------------------------------------------
std::string TrickHLA::to_string( THLAReportFormatEnum report_format )
{
   switch ( report_format ) {
      case THLAReportFormatEnum::REPORT_FORMAT_NONE:
         return ( "REPORT_FORMAT_UNKNOWN" );
         break;
      case THLAReportFormatEnum::REPORT_FORMAT_CSV:
         return ( "REPORT_FORMAT_CSV" );
         break;
      case THLAReportFormatEnum::REPORT_FORMAT_MD:
         return ( "REPORT_FORMAT_MD" );
         break;
      case THLAReportFormatEnum::REPORT_FORMAT_LATEX:
         return ( "REPORT_FORMAT_LATEX" );
         break;
      default:
         return ( "REPORT_FORMAT_UNKNOWN" );
   }
   return ( "REPORT_FORMAT_UNKNOWN" );
}

/*!
 * @job_class{initialization}
 */
ReportFormatBase::ReportFormatBase( ReportingServices & service )
: format( THLAReportFormatEnum::REPORT_FORMAT_NONE ),
  detail( THLAReportDetailEnum::REPORT_DETAIL_NOMINAL ),
  topics( static_cast<uint16_t>(THLAReportTopicsEnum::REPORT_TOPICS_ALL) )
{
    reporting_service = &service;
    return;
};

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
ReportFormatBase::~ReportFormatBase()
{
   return;
}

//----------------------------------------------------------------------------
// General report support functions.
//----------------------------------------------------------------------------

/*!
 *  @job_class{initialization}
 */



/*! @brief Set the report detail level.
*  @param report_detail Detail level for this report. */
void ReportFormatBase::set_detail( THLAReportDetailEnum report_detail )
{
    detail = report_detail;
    return;
}

/*! @brief Set the report topics filter.
*  @param report_topics Topic selections for this report. */
void ReportFormatBase::set_topics( uint16_t report_topics )
{
    topics = report_topics;
    return;
}

/*! @brief Add a report topic.
*  @param topic Report topic to add. */
void ReportFormatBase::add_topic( THLAReportTopicsEnum topic )
{
    // Add the topic to the reporting topics bit field.
    topics |= static_cast<uint16_t>(topic);
    return;
}

/*! @brief Remove a report topic.
*  @param topic Report topic to remove. */
void ReportFormatBase::remove_topic( THLAReportTopicsEnum topic )
{
    // Remove the topic to the reporting topics bit field.
    topics &= ~(static_cast<uint16_t>(topic));
    return;
}

   
//-------------------------------------------------------------------------
// CheckpointConversionBase Interface.
//-------------------------------------------------------------------------

/*! @brief Convert data to a form Trick can checkpoint. */
void ReportFormatBase::convert_data_before_checkpoint()
{
    return;
}

/*! @brief Restore data structures after loading a Trick checkpoint. */
void ReportFormatBase::restore_data_after_checkpoint()
{
    return;
}

/*! @brief Clear/release the memory used for the conversion data for the checkpoint. */
void ReportFormatBase::free_converted_data_for_checkpoint()
{
    return;
}
