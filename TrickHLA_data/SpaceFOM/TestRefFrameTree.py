##############################################################################
#
# @file TestFrameTree.py
# @ingroup SpaceFOM
# @brief Simple default reference frame tree configuration.
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
# # 2101 NASA Parkway, Houston, TX  77058
# 
# @python_module{Trick}
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, March 2026, --, Initial version. }
# @revs_end
#
##############################################################################
import trick
from TrickHLA_data.SpaceFOM.SpaceFOMRefFrameObject import *


class TestFrameTree( TrickHLAObjectConfig ):

   # Class variables.
   federate_config = None
   test_tree       = None
   root_frame      = None
   leaf_frame      = None
   tree_debug      = False
   
   root_ref_frame = None
   leaf_ref_frame = None

   def __init__( self,
                 federate_config,
                 test_tree_sim_obj,
                 root_frame_sim_obj,
                 leaf_frame_sim_obj,
                 tree_debug = False ):
      
      # Set the class data.
      self.federate_config = federate_config
      self.test_tree       = test_tree_sim_obj
      self.root_ref_frame  = root_frame_sim_obj
      self.leaf_ref_frame  = leaf_frame_sim_obj
      self.tree_debug      = tree_debug
      
      # Set the Test Reference Frame Tree state.
      self.set_tree_default_state()

      #---------------------------------------------------------------------------
      # Set up the Root Reference Frame object for discovery.
      # If it is the RRFP, it will publish the frame.
      # If it is NOT the RRFP, it will subscribe to the frame.
      #---------------------------------------------------------------------------
      self.root_frame = SpaceFOMRefFrameObject( 
         create_frame_object          = self.federate_config.is_RRFP,
         frame_instance_name          = self.test_tree.root_frame_data.name,
         frame_S_define_instance      = self.root_ref_frame.frame_packing,
         frame_S_define_instance_name = 'root_ref_frame.frame_packing',
         frame_conditional            = self.root_ref_frame.conditional,
         frame_lag_comp               = self.root_ref_frame.lag_compensation,
         frame_lag_comp_type          = trick.TrickHLA.LAG_COMPENSATION_NONE,
         frame_ownership              = self.root_ref_frame.ownership_handler,
         frame_deleted                = self.root_ref_frame.deleted_callback )

      # Set the root frame for the federate.
      self.federate_config.set_root_frame( self.root_frame )

      #---------------------------------------------------------------------------
      # Set up an alternate leaf reference frame object for discovery.
      #---------------------------------------------------------------------------
      self.leaf_frame = SpaceFOMRefFrameObject( 
         create_frame_object          = self.federate_config.is_RRFP,
         frame_instance_name          = self.test_tree.leaf_frame_data.name,
         frame_S_define_instance      = self.leaf_ref_frame.frame_packing,
         frame_S_define_instance_name = 'leaf_ref_frame.frame_packing',
         parent_S_define_instance     = self.root_ref_frame.frame_packing,
         parent_name                  = 'RootFrame',
         frame_conditional            = self.leaf_ref_frame.conditional,
         frame_lag_comp               = self.leaf_ref_frame.lag_compensation,
         frame_lag_comp_type          = trick.TrickHLA.LAG_COMPENSATION_NONE,
         frame_ownership              = self.leaf_ref_frame.ownership_handler,
         frame_deleted                = self.leaf_ref_frame.deleted_callback )

      # Add this reference frame to the list of managed object.
      self.federate_config.add_fed_object( self.leaf_frame )

      # For this test case, the leaf frame is not compensated.
      self.set_leaf_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_NONE )

      # Set the debug flag for test tree.
      self.set_debug( self.tree_debug )

      return


   def set_debug( self, debug_state = True ):
      
      # Set the debug flag for the root reference frame.
      self.root_ref_frame.frame_packing.debug = self.tree_debug
      
      # Set the debug flag for the leaf reference frame.
      self.leaf_ref_frame.frame_packing.debug = self.tree_debug
      
      return


   def set_tree_default_state( self,
                               root_frame_name = 'RootFrame',
                               leaf_frame_name = 'FrameA'    ):
      
      # The Root Reference Frame is always a trivial zero state frame.
      self.test_tree.root_frame_data.name = root_frame_name
      self.test_tree.root_frame_data.parent_name = ''

      self.test_tree.root_frame_data.state.pos[0] = 0.0
      self.test_tree.root_frame_data.state.pos[1] = 0.0
      self.test_tree.root_frame_data.state.pos[2] = 0.0
      self.test_tree.root_frame_data.state.vel[0] = 0.0
      self.test_tree.root_frame_data.state.vel[1] = 0.0
      self.test_tree.root_frame_data.state.vel[2] = 0.0
      self.test_tree.root_frame_data.state.att.scalar = 1.0
      self.test_tree.root_frame_data.state.att.vector[0] = 0.0
      self.test_tree.root_frame_data.state.att.vector[1] = 0.0
      self.test_tree.root_frame_data.state.att.vector[2] = 0.0
      self.test_tree.root_frame_data.state.ang_vel[0] = 0.0
      self.test_tree.root_frame_data.state.ang_vel[1] = 0.0
      self.test_tree.root_frame_data.state.ang_vel[2] = 0.0
      self.test_tree.root_frame_data.state.time = 0.0

      # The default leaf frame is a static offset from the root frame.
      self.test_tree.leaf_frame_data.name = leaf_frame_name
      self.test_tree.leaf_frame_data.parent_name = root_frame_name

      self.test_tree.leaf_frame_data.state.pos[0] = 10.0
      self.test_tree.leaf_frame_data.state.pos[1] = 10.0
      self.test_tree.leaf_frame_data.state.pos[2] = 10.0
      self.test_tree.leaf_frame_data.state.vel[0] = 0.0
      self.test_tree.leaf_frame_data.state.vel[1] = 0.0
      self.test_tree.leaf_frame_data.state.vel[2] = 0.0
      self.test_tree.leaf_frame_data.state.att.scalar = 1.0
      self.test_tree.leaf_frame_data.state.att.vector[0] = 0.0
      self.test_tree.leaf_frame_data.state.att.vector[1] = 0.0
      self.test_tree.leaf_frame_data.state.att.vector[2] = 0.0
      self.test_tree.leaf_frame_data.state.ang_vel[0] = 0.0
      self.test_tree.leaf_frame_data.state.ang_vel[1] = 0.0
      self.test_tree.leaf_frame_data.state.ang_vel[2] = 0.0
      self.test_tree.leaf_frame_data.state.time = 0.0

      return


   def set_leaf_frame_state( self, state ):

      self.test_tree.leaf_frame_data.state.pos[0] = state.pos[0]
      self.test_tree.leaf_frame_data.state.pos[1] = state.pos[1]
      self.test_tree.leaf_frame_data.state.pos[2] = state.pos[2]
      self.test_tree.leaf_frame_data.state.vel[0] = state.vel[0]
      self.test_tree.leaf_frame_data.state.vel[1] = state.vel[1]
      self.test_tree.leaf_frame_data.state.vel[2] = state.vel[2]
      self.test_tree.leaf_frame_data.state.att.scalar = state.att.scalar
      self.test_tree.leaf_frame_data.state.att.vector[0] = state.att.vector[0]
      self.test_tree.leaf_frame_data.state.att.vector[1] = state.att.vector[1]
      self.test_tree.leaf_frame_data.state.att.vector[2] = state.att.vector[2]
      self.test_tree.leaf_frame_data.state.ang_vel[0] = state.ang_vel[0]
      self.test_tree.leaf_frame_data.state.ang_vel[1] = state.ang_vel[1]
      self.test_tree.leaf_frame_data.state.ang_vel[2] = state.ang_vel[2]
      self.test_tree.leaf_frame_data.state.time = state.time

      return


   def set_leaf_lag_comp_type( self, comp_type ):

      self.leaf_frame.set_lag_comp_type( comp_type )

      return
