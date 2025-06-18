/*!
@ingroup encoding
@file models/FixedRecord/include/FixedRecData.hh
@brief This is a container class for general encoder test data.

@copyright Copyright 2025 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{FixedRecord/src/FixedRecData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_FIXED_REC_DATA_HH
#define TRICKHLA_MODEL_FIXED_REC_DATA_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

namespace TrickHLAModel
{

class FixedRecData
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__FixedRecData();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel FixedRecData class. */
   FixedRecData();

   /*! @brief Destructor for the TrickHLAModel FixedRecData class. */
   virtual ~FixedRecData();

   bool compare( FixedRecData const &data, std::string &explanation );

   std::string to_string();

   // FixedRecordTest.xml:
   // MainFixedRecObject
   // - field_1_string:  HLAunicodeString
   // - field_2_float64: HLAfloat64LE
   // - field_3_rec:     MainFixedRecord
   //   + MainFixedRecord:  HLAfixedRecord
   //     - elem_1_string:  HLAunicodeString
   //     - elem_2_float64: HLAfloat64LE
   //     - elem_3_record:  SecondaryFixedRecord
   //       + SecondaryFixedRecord: HLAfixedRecord
   //         - element_1_count: HLAinteger32LE
   //         - element_2_name:  HLAunicodeString

  public:
   // MainFixedRecObject
   char  *field_1_string;
   double field_2_float64;

   // MainFixedRecord
   char  *elem_1_string;
   double elem_2_float64;

   // SecondaryFixedRecord
   int   element_1_count;
   char *element_2_name;
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_FIXED_REC_DATA_HH: Do NOT put anything after this line!
