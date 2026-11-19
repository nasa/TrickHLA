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
   REPORT_FORMAT_LAST  = 0x0004, ///< Convenience marked for end of format types.
   REPORT_FORMAT_ALL   = 0x7FFF  ///< Gereat rports in all formats.
};

/*! @brief Convert a THLAReportFormatEnum value into a string representation.
 *  @param report_format Value to convert to a string. */
std::string to_string( THLAReportFormatEnum report_format );

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
   //----------------------------- USER VARIABLES -----------------------------
   // Variables below this point are for either use within a users simulation
   // or must be configured by the user.

   bool active; ///< @trick_units{1} Report generation activation flag.

  public:
   /*! @brief Default constructor for the TrickHLA ReportFormatBase class.
    *  @param services Reference to the reporting services. */
   ReportFormatBase( ReportingServices & service )
   : format( THLAReportFormatEnum::REPORT_FORMAT_NONE )
   {
      reporting_service = &service;
      return;
   };

   /*! @brief Destructor for the TrickHLA ReportFormatBase class. */
   virtual ~ReportFormatBase()
   {
      return;
   };

   /*! @brief Function called to generate requested reports. */
   virtual void generate_report() = 0;

   
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
   THLAReportFormatEnum format; ///< @trick_units{1} Format for this report.

   ReportingServices * reporting_service; ///< @trick_units{--} Reporting services.

};

} // namespace TrickHLA

#endif // TRICKHLA_REPORT_FORMAT_BASE_HH
