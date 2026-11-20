/*!
@file TrickHLA/ReportFormatCSV.hh
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

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/ReportFormatCSV.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2026, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_REPORT_FORMAT_CSV_HH
#define TRICKHLA_REPORT_FORMAT_CSV_HH

// System includes.

// TrickHLA includes.
#include "TrickHLA/ReportFormatBase.hh"

namespace TrickHLA
{

class ReportFormatCSV : public ReportFormatBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ReportFormatCSV();

  public:
   /*! @brief Constructor for the TrickHLA ReportFormatBase class.
    *  @param services Reference to the reporting services. */
   ReportFormatCSV( ReportingServices & service );

   /*! @brief Destructor for the TrickHLA ReportFormatCSV class. */
   virtual ~ReportFormatCSV();

   /*! @brief Function called to generate requested reports. */
   virtual void generate_report();

};

} // namespace TrickHLA

#endif // TRICKHLA_REPORT_FORMAT_CSV_HH
