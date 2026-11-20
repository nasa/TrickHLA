/*!
@file TrickHLA/ReportFormatBase.hh
@ingroup TrickHLA
@brief A base class representation for formatted report generation.

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
@trick_link_dependency{../../source/TrickHLA/ReportFormatBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2026, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_REPORT_FORMAT_BASE_HH
#define TRICKHLA_REPORT_FORMAT_BASE_HH

// System includes.
#include <cstdint>

// TrickHLA includes.
#include "TrickHLA/CheckpointConversionBase.hh"

namespace TrickHLA
{

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class ReportingServices;

/*
 * Enumerated type used to identify a report format.
 */
enum class THLAReportFormatEnum : uint16_t {
   REPORT_FORMAT_NONE  = 0x0000, ///< No report format selected.
   REPORT_FORMAT_CSV   = 0x0001, ///< Comma separated value format.
   REPORT_FORMAT_MD    = 0x0002, ///< Markdown format.
   REPORT_FORMAT_LATEX = 0x0004, ///< LaTeX format.
   REPORT_FORMAT_ALL   = 0xFFFF  ///< Gereat rports in all formats.
};

/*! @brief Convert a THLAReportFormatEnum value into a string representation.
 *  @param report_format Value to convert to a string. */
std::string to_string( THLAReportFormatEnum report_format );

/*
 * Enumerated type used limit report generation detail.
 */
enum class THLAReportDetailEnum : uint8_t {
   REPORT_DETAIL_NONE    = 0, ///< Effectively deactivates the report.
   REPORT_DETAIL_MINIMAL = 1, ///< Minimal reporting detail, basically names and counts.
   REPORT_DETAIL_NOMINAL = 2, ///< Typical reporting detail, includes more details associated with Objects, Interaction, etc.
   REPORT_DETAIL_VERBOSE = 3, ///< Verbose reporting detail, includes all available reporting data.
   REPORT_DETAIL_LAST    = 3  ///< Marker for the last eumeration value.
};

/*
 * Enumerated type used to filter out specific information by topic.
 */
enum class THLAReportTopicsEnum : uint16_t {
   REPORT_TOPICS_NONE         = 0x0000, ///< Reports only the highest level information.
   REPORT_TOPICS_FEDERATION   = 0x0001, ///< Reports Federation Execution related information.
   REPORT_TOPICS_FEDERATE     = 0x0002, ///< Reports Federate related information.
   REPORT_TOPICS_OBJECTS      = 0x0004, ///< Reports information on object instances.
   REPORT_TOPICS_ATTRIBUTES   = 0x0008, ///< Reports information on object attributes.
   REPORT_TOPICS_INTERACTIONS = 0x0010, ///< Reports information on interactions.
   REPORT_TOPICS_PARAMETERS   = 0x0020, ///< Reports information on interaction parameters.
   REPORT_TOPICS_SYNC_POINTS  = 0x0040, ///< Reports information on synchronization points.
   REPORT_TOPICS_ALL          = 0xFFFF  ///< Generate report information for all topics.
};

class ReportFormatBase : public CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ReportFormatBase();

  public:
   /*! @brief Default constructor for the TrickHLA ReportFormatBase class.
    *  @param services Reference to the reporting services. */
   ReportFormatBase( ReportingServices & service );

   /*! @brief Destructor for the TrickHLA ReportFormatBase class. */
   virtual ~ReportFormatBase() = 0;

   /*! @brief Function called to generate requested reports. */
   virtual void generate_report() = 0;

   /*! @brief Set the report detail level.
    *  @param report_detail Detail level for this report. */
   virtual void set_detail( THLAReportDetailEnum report_detail );

   /*! @brief Get the report detail level.
    *  @return Detail level for this report. */
   virtual THLAReportDetailEnum get_detail()
   {
      return( detail );
   }

   /*! @brief Check the report detail level.
    *  @param report_detail Detail level for this report. */
   virtual bool is_detail_level( THLAReportDetailEnum report_detail )
   {
      return( (report_detail == detail) ? true : false );
   }

   /*! @brief Set the report topics filter.
    *  @param report_topics Topic selections for this report. */
   virtual void set_topics( uint16_t report_topics );
   
   /*! @brief Add a report topic.
    *  @param topic Report topic to add. */
   virtual void add_topic( THLAReportTopicsEnum topic );

   /*! @brief Remove a report topic.
    *  @param topic Report topic to remove. */
   virtual void remove_topic( THLAReportTopicsEnum topic );

   /*! @brief Check for a topic.
    *  @param topic Report topic to check for.
    *  @return Returns true if report topic is set in the specified topics. */
   bool has_report_topic( THLAReportTopicsEnum topic );

   
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
   THLAReportFormatEnum format; ///< @trick_units{1} Format for this report.
   THLAReportDetailEnum detail; ///< @trick_units{1} Detail for this report.
   uint16_t             topics; ///< @trick_units{1} Topics filter for this report.

   ReportingServices * reporting_service; ///< @trick_units{--} Reporting services.

};

} // namespace TrickHLA

#endif // TRICKHLA_REPORT_FORMAT_BASE_HH
