/*!
@ingroup Sine
@file models/sine/include/FixedRecPacking.hh
@brief This class provides data packing.

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
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{FixedRecord/src/FixedRecData.cpp}
@trick_link_dependency{FixedRecord/src/FixedRecPacking.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation}
@revs_end

*/

#ifndef TRICKHLA_MODEL_FIXED_REC_PACKING_HH
#define TRICKHLA_MODEL_FIXED_REC_PACKING_HH

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"

// Model include files.
#include "FixedRecData.hh"

namespace TrickHLAModel
{

class FixedRecPacking : public FixedRecData, public TrickHLA::Packing
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__FixedRecPacking();

  public:
   FixedRecData *sim_data; ///< @trick_units{--} Simulation data.

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel FixedRecPacking class. */
   FixedRecPacking();
   /*! @brief Destructor for the TrickHLAModel FixedRecPacking class. */
   virtual ~FixedRecPacking();

   /*! @brief Configure the packing object.
    *  @param sim_data The sine wave data object for packing and unpacking. */
   void configure( FixedRecData *sim_data );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for FixedRecPacking class.
    *  @details This constructor is private to prevent inadvertent copies. */
   FixedRecPacking( FixedRecPacking const &rhs );
   /*! @brief Assignment operator for FixedRecPacking class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   FixedRecPacking &operator=( FixedRecPacking const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_FIXED_REC_PACKING_HH: Do NOT put anything after this line!
