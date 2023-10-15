##############################################################################
# PURPOSE:
#    (This is an input file python routine to set up the logging for either
#     a AnalyticSineSimObj or PropagatedSineSimObj simulation object
#     instance.)
#
# REFERENCE:
#    (Trick 10 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Assumes that trick and data_record are available globally.))
#
# PROGRAMMERS:
#    ((Dan Dexter)(NASA/ER7)(June 2013)(--)(Initial implementation)))
##############################################################################

def log_sine_states( sim_object_name, log_cycle ) :

   #####################################################
   # Set up the data recording group control parameters.
   #####################################################

   # Build a data recording group name.
   recording_group_name = sim_object_name + "_States"

   # Create a binary recording group.
   dr_group = trick.DRBinary( recording_group_name )
   dr_group.thisown = 0

   # Set the logging cycle frequency.
   dr_group.set_cycle( log_cycle )

   # Set up other logging parameters.
   dr_group.set_freq( trick.DR_Always )
   dr_group.enable()

   var = sim_object_name + ".truth_data.value"
   dr_group.add_variable(var)
   var = sim_object_name + ".truth_data.time"
   dr_group.add_variable(var)
   var = sim_object_name + ".truth_data.phase"
   dr_group.add_variable(var)
   var = sim_object_name + ".truth_data.amp"
   dr_group.add_variable(var)
   var = sim_object_name + ".truth_data.freq"
   dr_group.add_variable(var)
   var = sim_object_name + ".truth_data.dvdt"
   dr_group.add_variable(var)
   
   var = sim_object_name + ".sim_data.value"
   dr_group.add_variable(var)
   var = sim_object_name + ".sim_data.time"
   dr_group.add_variable(var)
   var = sim_object_name + ".sim_data.phase"
   dr_group.add_variable(var)
   var = sim_object_name + ".sim_data.amp"
   dr_group.add_variable(var)
   var = sim_object_name + ".sim_data.freq"
   dr_group.add_variable(var)
   var = sim_object_name + ".sim_data.dvdt"
   dr_group.add_variable(var)
   
   var = sim_object_name + ".lag_compensation.value"
   dr_group.add_variable(var)
   var = sim_object_name + ".lag_compensation.time"
   dr_group.add_variable(var)
   var = sim_object_name + ".lag_compensation.phase"
   dr_group.add_variable(var)
   var = sim_object_name + ".lag_compensation.amp"
   dr_group.add_variable(var)
   var = sim_object_name + ".lag_compensation.freq"
   dr_group.add_variable(var)
   var = sim_object_name + ".lag_compensation.dvdt"
   dr_group.add_variable(var)


   #########################################################
   # Add the data recording group to Trick's data recording.
   #########################################################
   trick.add_data_record_group( dr_group, trick.DR_Buffer )

   return

