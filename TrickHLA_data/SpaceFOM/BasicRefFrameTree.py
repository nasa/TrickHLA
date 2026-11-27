##############################################################################
#
# @file BasicRefFrameTree.py
# @ingroup SpaceFOM
# @brief Basic default reference frame tree configuration with just a root
#        reerence frame.
#
# This Python class sets up the configuration parameters for a simple default
# reference frame tree.  This is intended for use only as a placeholder in a
# SpaceFOM-based federate that does not really need a tree.  This Python
# configuration file depends on the Trick Sim Object that are instantiated
# in the SpaceFOM TestRefFrameTree.sm file!
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
# 2101 NASA Parkway, Houston, TX  77058
#
# @python_module{Trick}
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, March 2026, --, Initial version. }
# @rev_entry{ Dan Dexter, NASA JSC, TrickHLA, November 2026, --, Refactored from TestRefFrameTree.py. }
# @revs_end
#
##############################################################################
import trick

from TrickHLA_data.SpaceFOM.SpaceFOMRefFrameObject import *
from TrickHLA_data.TrickHLA.TrickHLAObjectConfig import *


class BasicRefFrameTree(TrickHLAObjectConfig):
   # Class variables.
   federate_config = None
   ref_tree = None
   root_frame = None
   tree_debug = False
   root_ref_frame = None

   def __init__(
      self,
      federate_config,
      ref_tree_sim_obj,
      root_frame_sim_obj,
      tree_debug=False,
   ):
      # Set the class data.
      self.federate_config = federate_config
      self.ref_tree = ref_tree_sim_obj
      self.root_ref_frame = root_frame_sim_obj
      self.tree_debug = tree_debug

      # Set the Test Reference Frame Tree state.
      self.set_tree_default_state()

      # ---------------------------------------------------------------------------
      # Set up the Root Reference Frame object for discovery.
      # If it is the RRFP, it will publish the frame.
      # If it is NOT the RRFP, it will subscribe to the frame.
      # ---------------------------------------------------------------------------
      self.root_frame = SpaceFOMRefFrameObject(
         create_frame_object=self.federate_config.is_RRFP,
         frame_instance_name=self.ref_tree.root_frame_data.name,
         frame_S_define_instance=self.root_ref_frame.frame_packing,
         frame_S_define_instance_name="root_ref_frame.frame_packing",
         frame_conditional=self.root_ref_frame.conditional,
         frame_lag_comp=self.root_ref_frame.lag_compensation,
         frame_lag_comp_type=trick.TrickHLA.LAG_COMPENSATION_NONE,
         frame_ownership=self.root_ref_frame.ownership_handler,
         frame_deleted=self.root_ref_frame.deleted_callback,
      )

      # Set the root frame for the federate.
      self.federate_config.set_root_frame(self.root_frame)

      # Set the debug flag for test tree.
      self.set_debug(self.tree_debug)

   def set_debug(self, debug_state=True):
      # Set the debug flag for the root reference frame.
      self.root_ref_frame.frame_packing.debug = self.tree_debug

   def set_tree_default_state(self, root_frame_name="RootFrame"):
      # The Root Reference Frame is always a trivial zero state frame.
      self.ref_tree.root_frame_data.name = root_frame_name
      self.ref_tree.root_frame_data.parent_name = ""

      self.ref_tree.root_frame_data.state.pos[0] = 0.0
      self.ref_tree.root_frame_data.state.pos[1] = 0.0
      self.ref_tree.root_frame_data.state.pos[2] = 0.0
      self.ref_tree.root_frame_data.state.vel[0] = 0.0
      self.ref_tree.root_frame_data.state.vel[1] = 0.0
      self.ref_tree.root_frame_data.state.vel[2] = 0.0
      self.ref_tree.root_frame_data.state.att.scalar = 1.0
      self.ref_tree.root_frame_data.state.att.vector[0] = 0.0
      self.ref_tree.root_frame_data.state.att.vector[1] = 0.0
      self.ref_tree.root_frame_data.state.att.vector[2] = 0.0
      self.ref_tree.root_frame_data.state.ang_vel[0] = 0.0
      self.ref_tree.root_frame_data.state.ang_vel[1] = 0.0
      self.ref_tree.root_frame_data.state.ang_vel[2] = 0.0
      self.ref_tree.root_frame_data.state.time = 0.0
