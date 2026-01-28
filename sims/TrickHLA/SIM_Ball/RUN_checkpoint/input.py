
# Load in the data recording definition function.
exec(open("Modified_data/data_record.py").read())

# Load in the Trick realtime parameter setting.
exec(open("Modified_data/realtime.py").read())

# Load in the graphics definition and startup functions.
exec(open("Modified_data/graphics.py").read())

#---------------------------------------------------------------------------
# Set the Trick check point information.
#---------------------------------------------------------------------------
trick.checkpoint_pre_init(True)
trick.checkpoint_post_init(True)
trick.checkpoint_end(True)

trick.TMM_reduced_checkpoint(False)
trick.TMM_hexfloat_checkpoint(True)

#---------------------------------------------------------------------------
# Read in the 5 second checkpoint from the RUN_test directory.
#---------------------------------------------------------------------------
trick.exec_set_enable_freeze( False )
#trick.load_checkpoint( 'RUN_test/chkpnt_5.000000' )
trick.exec_set_freeze_command( False )
trick.add_read(0.0, 'trick.load_checkpoint("RUN_test/chkpnt_5.000000")')

#---------------------------------------------------------------------------
# Add the data recording groups.
#---------------------------------------------------------------------------
# Add Ball 1 to data recording.
add_dr_group( 'ball1', 'Ball1' )

# Add Ball 2 to data recording.
add_dr_group( 'ball2', 'Ball2' )

# Add Ball 3 to data recording.
add_dr_group( 'ball3', 'Ball3' )

#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
run_duration = 10.0

if run_duration != None:
   if run_duration == 0.0:
      trick.stop(0.0)
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )

