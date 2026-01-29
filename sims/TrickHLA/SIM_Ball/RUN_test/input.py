##############################################################################
#
# @file RUN_test/input.py
# @ingroup TrickHLA
# @brief A configuration input class for SIM_Ball.
# 
# This is a Trick Python input file class using the TrickHLA base
# for configuring the Ball test simulation.
# 
# @copyright Copyright 2026 United States Government as represented by the
# Administrator of the National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Other Rights Reserved.
# 
# \par<b>Responsible Organization</b>
# Simulation and Graphics Branch, Mail Code ER7\n
# Software, Robotics & Simulation Division\n
# NASA, Johnson Space Center\n
# # 2101 NASA Parkway, Houston, TX  77058
# 
# @python_module{ArtemisFOM}
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, January 2026, --, Initial version. }
# @revs_end
#
##############################################################################

# Find the TrickHLA home location and append the path.
trickhla_home = os.environ.get( "TRICKHLA_HOME" )
if trickhla_home is None:
   sys.exit( '\033[91m'\
            +'Environment variable TRICKHLA_HOME is not defined!'\
            +'\033[0m\n' )
else:
   if os.path.isdir( trickhla_home ) is False:
      sys.exit( '\033[91m'\
               +'TRICKHLA_HOME not found: '\
               +trickhla_home\
               +'\033[0m\n' )

# Append the path to the top level of the top level TrickHLA directory.
# We need this to locate the TrickHLA_data Python data directory.
if trickhla_home not in sys.path :
   sys.path.append( trickhla_home )

# Load in the data recording definition function.
#exec(open("Modified_data/data_record.py").read())

# Load in the Trick realtime parameter setting.
exec(open("Modified_data/realtime.py").read())

# Load in the graphics definition and startup functions.
exec(open("Modified_data/graphics.py").read())

# Load the Ball State Data Recording Group class.
#from TrickHLA_data.Trick.TrickDataRecordingGroup import *
from BallStateDRG import *

#---------------------------------------------------------------------------
# Set the Trick check point information.
#---------------------------------------------------------------------------
trick.checkpoint_pre_init(True)
trick.checkpoint_post_init(True)
trick.checkpoint_end(True)

trick.TMM_reduced_checkpoint(False)
trick.TMM_hexfloat_checkpoint(True)

# Check point at 5 seconds.
trick.checkpoint(5.0)

#---------------------------------------------------------------------------
# Set the Walls information.
#---------------------------------------------------------------------------
ensemble.walls.print_contact    = False
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
#add_dr_group( 'ball1', 'Ball1' )

#
# Ball #2
#
ball2.state.name = "ball2"
ball2.state.id   = 1

ball2.state.input.print_state = False
ball2.state.input.speed = 5.0
ball2.state.input.elevation = trick.sim_services.attach_units("degree", -45.0)
# Add Ball 2 to data recording.
#add_dr_group( 'ball2', 'Ball2' )

#
# Ball #3
#
ball3.state.name = "ball3"
ball3.state.id   = 2

ball3.state.input.print_state = False
ball3.state.input.speed = 7.5
ball3.state.input.elevation = trick.sim_services.attach_units("degree", 30.0)
# Add Ball 3 to data recording.
#add_dr_group( 'ball3', 'Ball3' )


#---------------------------------------------------------------------------
# Setup the Data Recording Groups.
#---------------------------------------------------------------------------
ball1_drg = BallStateDRG( 'ball1', 'Ball1', 0.1 )
ball2_drg = BallStateDRG( 'ball2', 'Ball2', 0.1 )
ball3_drg = BallStateDRG( 'ball3', 'Ball3', 0.1 )

# Initialize all the Data Recording Groups.
TrickDataRecordingGroup.initialize_groups()


#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
run_duration = 10.0

if run_duration != None:
   if run_duration == 0.0:
      trick.stop(0.0)
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )

