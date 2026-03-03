##############################################################################
#
# @file RUN_ball1/input.py
# @ingroup SpaceFOM
# @brief A configuration input class for SpaceFOM SIM_Ball example.
# 
# This is a Trick Python input file class using the TrickHLA base
# for configuring the Ball simulation with HLA.
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
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, March 2026, --, Initial version. }
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

# Import the Ball Simulation Configuration class.
from Modified_data.BallSimConfig import TrickSimConfig, BallSimConfig

# Import the Ball State Data Recording Group class.
from Modified_data.BallStateDRG import TrickDataRecordingGroup, BallStateDRG

# Import the Ball HLA Federate Configuration class.
from Modified_data.BallFederateConfig import BallFederateConfig

# Import the Ball HLA Object Configuration function.
#from Modified_data.HLA_ball_config import *
from TrickHLA_data.SpaceFOM.TestRefFrameTree import *

# Import the Ball HLA Object Configuration function.
#from Modified_data.HLA_ball_config import *
from Modified_data.BallObject import *

# Import the Wall HLA Object Configuration function.
#from Modified_data.HLA_walls_config import *
from Modified_data.WallObject import *


#---------------------------------------------------------------------------
# Set the simulation run duration.  Zero (0) means unlimited.
#---------------------------------------------------------------------------
run_duration = 10.0


#---------------------------------------------------------------------------
# Configure the Ball simulation.
#---------------------------------------------------------------------------
ball_sim_config = BallSimConfig( 'Ball 1 Sim' )
ball_sim_config.realtime()
ball_sim_config.fix_var_server_source_address()
ball_sim_config.sim_control_panel()


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

#
# Ball #2
#
ball2.state.name = "ball2"
ball2.state.id   = 1

ball2.state.input.print_state = False
ball2.state.input.speed = 5.0
ball2.state.input.elevation = trick.sim_services.attach_units("degree", -45.0)

#
# Ball #3
#
ball3.state.name = "ball3"
ball3.state.id   = 2

ball3.state.input.print_state = False
ball3.state.input.speed = 7.5
ball3.state.input.elevation = trick.sim_services.attach_units("degree", 30.0)


#---------------------------------------------------------------------------
# Setup the Data Recording Groups.
#---------------------------------------------------------------------------
ball1_drg = BallStateDRG( 'ball1', 'Ball1', 0.1 )
ball2_drg = BallStateDRG( 'ball2', 'Ball2', 0.1 )
ball3_drg = BallStateDRG( 'ball3', 'Ball3', 0.1 )

# Initialize all the Data Recording Groups.
TrickDataRecordingGroup.initialize_groups()


#---------------------------------------------------------------------------
# Setup Ball graphics and add to the Trick external applications list.
#---------------------------------------------------------------------------
ball_sim_config.ball_graphics()


#---------------------------------------------------------------------------
# Set the HLA information.
#---------------------------------------------------------------------------
# Configure the Ball HLA data
ball_fed_config = BallFederateConfig( 
   thla            = THLA,
   thla_init       = THLA_INIT,
   federate_name   = 'Ball 1 Sim',
   federation_name = 'BallFederation',
   enabled         = True )

# Check to see if we need to fix the Trick variable server address.
ball_fed_config.fix_var_server_source_address()

# Set the TrickHLA debug reporting level.
#federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )
ball_fed_config.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_2_TRACE )

# Configure this federate SpaceFOM roles for this federate.
ball_fed_config.set_master_role( True )  # This is the Master federate.
ball_fed_config.set_pacing_role( True )  # This is the Pacing federate.
ball_fed_config.set_RRFP_role( True )    # This is the Root Reference Frame Publisher.

# Only the Master federate can set the LCTS in a SpaceFOM federate!
# This call only sticks after the Master role is set!
if ball_fed_config.is_master:
   ball_fed_config.set_least_common_time_step( 0.10 )

#
# Add in known required federates.
#
ball_fed_config.add_known_federate( True, str( ball_fed_config.federate.name ) )
ball_fed_config.add_known_federate( True, 'Ball 2 Sim' )
ball_fed_config.add_known_federate( True, 'Ball 3 Sim' )

#...........................................................................
# Configure the Wall HLA data.
#...........................................................................
walls_config = WallsObject( 
   create_walls_object          = True,
   walls_instance_name          = 'walls',
   walls_S_define_instance      = walls_hla.packing,
   walls_S_define_instance_name = 'walls_hla.packing' )
ball_fed_config.add_fed_object( walls_config )

#...........................................................................
# Configure the HLA data for each Ball and add them to the list of managed
# objects.
#...........................................................................
ball1_config = BallObject( 
   create_ball_object          = True,
   ball_instance_name          = str(ball1.state.name),
   ball_S_define_instance      = ball1_hla.packing,
   ball_S_define_instance_name = 'ball1_hla.packing' )
ball_fed_config.add_fed_object( ball1_config )

ball2_config = BallObject( 
   create_ball_object          = False,
   ball_instance_name          = str(ball2.state.name),
   ball_S_define_instance      = ball2_hla.packing,
   ball_S_define_instance_name = 'ball2_hla.packing' )
ball_fed_config.add_fed_object( ball2_config )

ball3_config = BallObject( 
   create_ball_object          = False,
   ball_instance_name          = str(ball3.state.name),
   ball_S_define_instance      = ball3_hla.packing,
   ball_S_define_instance_name = 'ball3_hla.packing' )
ball_fed_config.add_fed_object( ball3_config )


#...........................................................................
# Set up a trivial test reference frame tree.
#...........................................................................
test_frame_tree = TestFrameTree(
   federate_config    = ball_fed_config,
   test_tree_sim_obj  = ref_frame_tree,
   root_frame_sim_obj = root_ref_frame,
   leaf_frame_sim_obj = leaf_ref_frame,
   tree_debug         = False )


#...........................................................................
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
#...........................................................................
ball_fed_config.add_sim_object( THLA )
ball_fed_config.add_sim_object( THLA_INIT )
ball_fed_config.add_sim_object( ref_frame_tree )
ball_fed_config.add_sim_object( root_ref_frame )
ball_fed_config.add_sim_object( leaf_ref_frame )
ball_fed_config.add_sim_object( ball1_hla )
ball_fed_config.add_sim_object( ball2_hla )
ball_fed_config.add_sim_object( ball3_hla )


#...........................................................................
# Make sure that the federate configuration object is initialized.
#...........................................................................
ball_fed_config.initialize()


#---------------------------------------------------------------------------
# Manage simulation termination time.
#---------------------------------------------------------------------------
if run_duration != None:
   if run_duration == 0.0:
      trick.stop(0.0)
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )

