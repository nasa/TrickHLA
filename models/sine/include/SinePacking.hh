/*!
@ingroup Sine
@file models/sine/include/SinePacking.hh
@brief This class provides data packing.

@copyright Copyright 2020 United States Government as represented by the
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
@trick_link_dependency{sine/src/SineData.cpp}
@trick_link_dependency{sine/src/SinePacking.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, Sept 2009, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_SINE_PACKING_HH
#define TRICKHLA_MODEL_SINE_PACKING_HH

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"

// Model include files.
#include "SineData.hh"

namespace TrickHLAModel
{

class SinePacking : public SineData, public TrickHLA::Packing
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SinePacking();

  public:
   SineData *sim_data; ///< @trick_units{--} Simulation data.

   double phase_deg; ///< @trick_units{degree} Phase offset in degrees.

   unsigned int pack_count; ///< @trick_units{count} The number of times the pack routine has been called.

   int buff_size; ///< @trick_units{--} Size of the byte buffer.

   unsigned char *buff; ///< @trick_units{--} Byte buffer.

   TrickHLA::Attribute *time_attr;  ///< @trick_io{**} Reference to the "Time" TrickHLA::Attribute.
   TrickHLA::Attribute *value_attr; ///< @trick_io{**} Reference to the "Value" TrickHLA::Attribute.
   TrickHLA::Attribute *dvdt_attr;  ///< @trick_io{**} Reference to the "dvdt" TrickHLA::Attribute.
   TrickHLA::Attribute *phase_attr; ///< @trick_io{**} Reference to the "Phase" TrickHLA::Attribute.
   TrickHLA::Attribute *freq_attr;  ///< @trick_io{**} Reference to the "Frequency" TrickHLA::Attribute.
   TrickHLA::Attribute *amp_attr;   ///< @trick_io{**} Reference to the "Amplitude" TrickHLA::Attribute.
   TrickHLA::Attribute *tol_attr;   ///< @trick_io{**} Reference to the "Tolerance" TrickHLA::Attribute.
   TrickHLA::Attribute *name_attr;  ///< @trick_io{**} Reference to the "Name" TrickHLA::Attribute.

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SinePacking class. */
   SinePacking();
   /*! @brief Destructor for the TrickHLAModel SinePacking class. */
   virtual ~SinePacking();

   /*! @brief Set the packing object working data.
    *  @param data The sine wave data object for packing and unpacking. */
   void set_data( SineData *data );

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SinePacking class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SinePacking( SinePacking const &rhs );
   /*! @brief Assignment operator for SinePacking class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SinePacking &operator=( SinePacking const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_SINE_PACKING_HH: Do NOT put anything after this line!
