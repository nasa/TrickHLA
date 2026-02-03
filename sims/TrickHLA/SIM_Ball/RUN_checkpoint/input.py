##############################################################################
#
# @file RUN_test/input.py
# @ingroup TrickHLA
# @brief A configuration input class for SIM_Ball.
# 
# This is a Trick Python input file class using the TrickHLA base
# for configuring the Ball test simulation checkpoint run.
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

#---------------------------------------------------------------------------
# Read in the 5 second checkpoint from the RUN_test directory.
#---------------------------------------------------------------------------
trick.exec_set_enable_freeze( True)
trick.exec_set_freeze_command( True)
# Start up in Freeze and manually load the modified checkpoint.
# Eventually, the Trick team will have an ability to restart from a
# checkpoint without entering into Freeze and manually loading the
# desired checkpoint.  You will be able to do it from the input file
# as part of initialization.
# The following don't currently work.
#trick.load_checkpoint( 'RUN_test/chkpnt_5.000000' )
#trick.add_read(0.0, 'trick.load_checkpoint("RUN_checkpoint/chkpnt_5.000000")')


#---------------------------------------------------------------------------
# Setup the Data Recording Groups.
#---------------------------------------------------------------------------
# Since we are restoring from a checkpoint, these will get overridden anyway.
# So, don't bother specifying them here.
#ball1_drg = BallStateDRG( 'ball1', 'Ball1', 0.1 )
#ball2_drg = BallStateDRG( 'ball2', 'Ball2', 0.1 )
#ball3_drg = BallStateDRG( 'ball3', 'Ball3', 0.1 )

# Initialize all the Data Recording Groups.
#TrickDataRecordingGroup.initialize_groups()


#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
run_duration = 10.0

if run_duration != None:
   if run_duration == 0.0:
      trick.stop(0.0)
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )

