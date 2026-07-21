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
   /*! @brief Destructor for the TrickHLA CheckpointConversionBase class. */
   virtual ~CheckpointConversionBase()
   {
      return;
   }

   //-------------------------------------------------------------------------
   // Save and Restore functions.
   //
   // These functions support the HLA Save and Restore processes. These
   // functions are used in the SaveRestoreServices and ExecutionControl
   // classes to regulate the HLA Save and Restore processes.  Users will
   // typically NOT interact with these functions.
   //
   /*! @brief Convert data to a form Trick can checkpoint. */
   virtual void convert_data_before_checkpoint() = 0;

   /*! @brief Restore data structures after loading a Trick checkpoint. */
   virtual void restore_data_after_checkpoint() = 0;

   /*! @brief Clear/release the memory used for the conversion data for the checkpoint. */
   virtual void free_converted_data_for_checkpoint() = 0;

   //-------------------------------------------------------------------------
   // Checkpoint functions.
   //
   // These functions support the Trick checkpoint processes. These functions
   // are used in the THLABase.sm simulation module.  Users will typically NOT
   // interact with these functions.
   //
   /*! @brief Prepare for checkpointing.  Usually for an HLA Save. */
   virtual void checkpoint_before() { return; }

   /*! @brief Prepare to load a checkpoint file.  Usually as part of an HLA Restore. */
   virtual void checkpoint_preload() { return; }

   /*! @brief Federate tasks to perform after a checkpoint.  Usually for an HLA Save. */
   virtual void checkpoint_after() { return; }

   /*! @brief Tasks to perform after a checkpoint load.  Usually as part of an HLA Restore. */
   virtual void checkpoint_restart() { return; }
};

} // namespace TrickHLA

#endif /* TRICKHLA_CHECKPOINT_CONVERSION_BASE_HH */
