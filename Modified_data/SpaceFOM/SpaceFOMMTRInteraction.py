##############################################################################
# PURPOSE:
#    (This is a python input file class to set up the general parameters that
#     describe an Space Reference FOM Mode Transition Request (MTR)
#     interaction.)
#
# REFERENCE:
#    (Trick 16 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAInteractionConfig class)
#     (Uses TrickHLAParameterConfig class.)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA) (June 2016) (--) (Space FOM development)))
##############################################################################
import trick
import sys
from ..TrickHLA.TrickHLAInteractionConfig import *
from ..TrickHLA.TrickHLAParameterConfig import *

class SpaceFOMMTRInteraction( TrickHLAInteractionConfig ):

   trick_sim_obj_name = None

   def __init__( self,
                 is_master,
                 S_define_instance,
                 S_define_instance_name,
                 manger_interaction = None ):
      
      # Set the name of the interaction.
      S_define_instance.name = 'ModeTransitionRequest'

      # Save the interaction name to use for trick_data_name generation.
      self.trick_sim_obj_name = str( S_define_instance_name )

      # Call the base class constructor.
      if ( is_master ):
         TrickHLAInteractionConfig.__init__( self,
                                             thla_FOM_name              = 'ModeTransitionRequest',
                                             thla_publish               = False,
                                             thla_subscribe             = True,
                                             thla_handler_instance      = S_define_instance,
                                             thla_handler_instance_name = S_define_instance_name,
                                             thla_manager_interaction   = manger_interaction )
      else:
         TrickHLAInteractionConfig.__init__( self,
                                             thla_FOM_name              = 'ModeTransitionRequest',
                                             thla_publish               = True,
                                             thla_subscribe             = False,
                                             thla_handler_instance      = S_define_instance,
                                             thla_handler_instance_name = S_define_instance_name,
                                             thla_manager_interaction   = manger_interaction )

      # Build the interaction parameters list.
      self.add_parameters()

      return


   def initialize( self, thla_interaction ):

      # Call the base class initialization utility function.
      TrickHLAInteractionConfig.initialize( self, thla_interaction )

      return


   def add_parameters( self ):

      # Set up the map to the MTR interaction execution mode.
      parameter = TrickHLAParameterConfig( FOM_name     = 'execution_mode',
                                           trick_name   = str(self.trick_sim_obj_name) + '.mtr_mode_int',
                                           rti_encoding = trick.TrickHLA.ENCODING_LITTLE_ENDIAN )
      self.add_parameter( parameter )

      return

