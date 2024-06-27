/*!
@file TrickHLA/CheckpointConversionBase.hh
@ingroup TrickHLA
@brief This class provides an interface for converting to and from Trick
checkpointable data structures.

@copyright Copyright 2024 United States Government as represented by the
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
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2024, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_CHECKPOINT_CONVERSION_BASE_HH
#define TRICKHLA_CHECKPOINT_CONVERSION_BASE_HH

// TrickHLA includes.
#include "TrickHLA/StandardsSupport.hh"

namespace TrickHLA
{

class CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__CheckpointConversionBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Don't allowdDefault constructor. */
   CheckpointConversionBase()
   {
      return;
   }

   /*! @brief Destructor for the TrickHLA CheckpointConversionBase class. */
   virtual ~CheckpointConversionBase()
   {
      return;
   }

   /*! @brief Convert the variables to a form Trick can checkpoint. */
   virtual void convert_to_checkpoint_data_structures() = 0;

   /*! @brief Restore the state of this class from the Trick checkpoint. */
   virtual void restore_from_checkpoint_data_structures() = 0;

   /*! @brief Clear/release the memory used for the checkpoint data structures. */
   virtual void clear_checkpoint_data_structures() = 0;
};

} // namespace TrickHLA

#endif /* TRICKHLA_CHECKPOINT_CONVERSION_BASE_HH */
