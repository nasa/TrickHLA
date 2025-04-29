
# Load in the data recording definition function.
exec(open("Modified_data/data_record.py").read())

# Load in the Trick realtime parameter setting.
exec(open("Modified_data/realtime.py").read())
trick.exec_set_freeze_command(False)
trick.sim_control_panel_set_enabled( False )

# Load in the graphics definition and startup functions.
#exec(open("Modified_data/graphics.py").read())

#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
run_duration = 10.0

#---------------------------------------------------------------------------
# Set the Walls information.
#---------------------------------------------------------------------------
ensemble.walls.print_contact    = True
ensemble.walls.floor_y_pos      = -10.0;
ensemble.walls.right_wall_x_pos =  10.0;
ensemble.walls.ceiling_y_pos    =  10.0;
ensemble.walls.left_wall_x_pos  = -10.0;

#---------------------------------------------------------------------------
# Set the Ball information.
#---------------------------------------------------------------------------
# Ball #1
#
ball1.state.name = "ball1"
ball1.state.id   = 0

ball1.state.input.print_state = False
ball1.state.input.speed = 10.0
ball1.state.input.elevation = trick.sim_services.attach_units("degree", 45.0)
# Add Ball 1 to data recording.
add_dr_group( 'ball1', 'Ball1' )

#
# Ball #2
#
ball2.state.name = "ball2"
ball2.state.id   = 1

ball2.state.input.print_state = False
ball2.state.input.speed = 5.0
ball2.state.input.elevation = trick.sim_services.attach_units("degree", -45.0)
# Add Ball 2 to data recording.
add_dr_group( 'ball2', 'Ball2' )

#
# Ball #3
#
ball3.state.name = "ball3"
ball3.state.id   = 2

ball3.state.input.print_state = False
ball3.state.input.speed = 7.5
ball3.state.input.elevation = trick.sim_services.attach_units("degree", 30.0)
# Add Ball 3 to data recording.
add_dr_group( 'ball3', 'Ball3' )

#---------------------------------------------------------------------------
# Set the HLA information.
#---------------------------------------------------------------------------
# Load in the HLA configuration
exec(open("Modified_data/HLA_config3.py").read())
THLA.federate.name = 'Ball2-Federate'

# Allocate the federate HLA objects: Walls + number of Balls.
THLA.manager.obj_count = ensemble.num_balls + 1
THLA.manager.objects   = trick.sim_services.alloc_type( THLA.manager.obj_count, 'TrickHLA::Object' )

#
# Read in the Walls HLA Object configuration function.
#
exec(open("Modified_data/HLA_walls_config.py").read())
HLA_walls_config( THLA.manager.objects[0], 'walls_hla', 'walls', walls_hla.packing, False )

#
# Read in the Ball HLA Object configuration function.
#
exec(open("Modified_data/HLA_ball_config.py").read())

#
# Create the Ball <-> HLA connections.
#

HLA_ball_config( THLA.manager.objects[1], 'ball1_hla', ball1.state.name, ball1_hla.packing, False )
HLA_ball_config( THLA.manager.objects[2], 'ball2_hla', ball2.state.name, ball2_hla.packing, True )
HLA_ball_config( THLA.manager.objects[3], 'ball3_hla', ball3.state.name, ball3_hla.packing, False )


#---------------------------------------------------------------------------
# Manage simulation termination time.
#---------------------------------------------------------------------------
if run_duration != None:
   if run_duration == 0.0:
      trick.stop(0.0)
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )

