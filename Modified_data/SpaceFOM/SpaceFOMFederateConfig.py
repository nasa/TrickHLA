##############################################################################
# PURPOSE:
#    (This is a python input file class to set up the general parameters that
#     describe a Space Reference FOM federate.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAFederateConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.)))
##############################################################################
import trick
from trick.top import *
from ..TrickHLA.TrickHLAFederateConfig import *

class SpaceFOMFederateConfig(TrickHLAFederateConfig):
   
   # Reference the SpaceFOM Execution Control.
   #execution_control = None
   
   # Reference the Execution Configuration Object (ExCO).
   #ExCO = None
   #config_S_define_name = None
   
   # SpaceFOM FOM modules path.  The default is a path relative to the
   # simulation directory in the FOMs/SpaceFOM directory.  In most cases, the
   # FOMs/SpaceFOM directory is a link wherever the SpaceFOM FOM modules
   # actually reside.
   SpaceFOMs_path = 'FOMs/SpaceFOM/'
   
   # SpaceFOM role designations.
   is_master = False
   is_pacing = False
   is_RRFP   = False
   
   # Federation execution scenario time epoch (TT in TJD reference in seconds).
   scenario_time_epoch = 0.0
   

   def __init__( self,
                 thla_federate,
                 thla_manager,
                 thla_control,
                 thla_config,
                 thla_federation_name,
                 thla_federate_name,
                 thla_enabled,
                 FOMs_path = None ):
      
      # Copy a FOM path if provided.
      if FOMs_path :
         self.SpaceFOMs_path = FOMs_path

      # Call the base class constructor.
      TrickHLAFederateConfig.__init__( self,
                                       thla_federate,
                                       thla_manager,
                                       thla_control,
                                       thla_config,
                                       thla_federation_name,
                                       thla_federate_name,
                                       thla_enabled )

      # Add in required Space FOM modules.
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_switches.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_datatypes.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_management.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_environment.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_entity.xml' )
      
      # Add any multi-phase initialization synchronization points.
      # These will be federation execution specific. These sync points
      # should be set in the federation execution specific extension to
      # this class.
      # self.add_multiphase_init_sync_point( 'sync_point_name' )

      return
   
   
   def set_master_role( self, master_state = True ):
      
      # You can only change master state before initialize method is called.
      if self.initialized :
         print('SpaceFOMFederateConfig.set_master_role(): Warning, already initialized, function ignored!')
      else:
         self.is_master = master_state
         self.control.master = master_state

      return
   
   
   def set_pacing_role( self, pacing_state = True ):
      
      # You can only change pacing state before initialize method is called.
      if self.initialized :
         print('SpaceFOMFederateConfig.set_pacing_role(): Warning, already initialized, function ignored!')
      else:
         self.is_pacing = pacing_state
         self.control.pacing = pacing_state

      return
   
   
   def set_RRFP_role( self, RRFP_state = True ):
      
      # You can only change RRFP state before initialize method is called.
      if self.initialized :
         print('SpaceFOMFederateConfig.set_RRFP_role(): Warning, already initialized, function ignored!')
      else:
         self.is_RRFP = RRFP_state
         self.control.root_frame_pub = RRFP_state

      return
   
   
   def set_config_S_define_name( self, config_S_define_name ):
      
      # You can only change RRFP state before initialize method is called.
      if self.initialized :
         print('SpaceFOMFederateConfig.set_config_S_define_name(): Warning, already initialized, function ignored!')
      else:
         self.config_S_define_name = config_S_define_name

      return


   def set_scenario_timeline_epoch( self, epoch ):
      
      # You can only set sim_config path before initialize method is called.
      if self.initialized :
         print('SpaceFOMFederateConfig.set_scenario_timeline_epoch(): Warning, already initialized, function ignored!')
         return

      # Set the new Least Common Time Step value.
      self.scenario_time_epoch = float( epoch )
      self.control.scenario_timeline.set_epoch( float( epoch ) )

      return


   def set_root_frame( self, root_frame ):
      
      # You can only set root frame before initialize method is called.
      if self.initialized :
         print('SpaceFOMFederateConfig.set_root_frame: Warning, already initialized, function ignored!')
         return
      
      # Check to see if an ExCO is already been set. If so, warn the user.
      if self.control.root_ref_frame:
         print('SpaceFOMFederateConfig.set_root_frame(): Warning, root reference frame is already set; this will override the existing settings!')
         return
      
      #
      # Assign the reference in the manager to the root reference frame object.
      #
      self.control.root_ref_frame = root_frame.get_packing_instance()

      #
      # If this is the Root Reference Frame Publisher (RRFP) then set the
      # root reference frame instance name as the Execution Configuration
      # Object (ExCO) attribute name.
      #
      # If the is not the RRFP, then the ExCO root reference frame name
      # attribute can be set to any empty string. It will be overwritten
      # on Root Reference Frame Discovery.
      #
      if ( self.is_RRFP ) :
         self.config.set_root_frame_name( root_frame.get_instance_name() )
      else:
         self.config.set_root_frame_name( '' )
      
      # Add the root frame to the list of federate objects to manage.
      self.add_fed_object( root_frame )

      return


   def set_FOMs_path( self, FOMs_path ):
      
      # You can only set SpaceFOM fom module pate before initialize method is called.
      if self.initialized :
         print('SpaceFOMFederateConfig.set_FOMs_path(): Warning, already initialized! Ignoring!')
         return
      
      for item in self.fom_modules:
         if item.find('SISO_SpaceFOM_switches.xml') != -1:
            indx = self.fom_modules.index( item )
            self.fom_modules.pop(indx)
      for item in self.fom_modules:
         if item.find('SISO_SpaceFOM_datatypes.xml') != -1:
            indx = self.fom_modules.index( item )
            self.fom_modules.pop(indx)
      for item in self.fom_modules:
         if item.find('SISO_SpaceFOM_management.xml') != -1:
            indx = self.fom_modules.index( item )
            self.fom_modules.pop(indx)
      for item in self.fom_modules:
         if item.find('SISO_SpaceFOM_environment.xml') != -1:
            indx = self.fom_modules.index( item )
            self.fom_modules.pop(indx)
      for item in self.fom_modules:
         if item.find('SISO_SpaceFOM_entity.xml') != -1:
            indx = self.fom_modules.index( item )
            self.fom_modules.pop(indx)
      
      # Change the SpaceFOM FOM module path prefix.
      self.SpaceFOMs_path = FOMs_path

      # Add the module back in with the specified FOM module path prefix.
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_switches.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_datatypes.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_management.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_environment.xml' )
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_entity.xml' )
      
      return


   def initialize( self ):
      
      # You can only initialize once.
      if self.initialized :
         print('SpaceFOMFederateConfig.initialize(): Warning, already initialized! Ignoring!')
         return
      
      # Call the base class initialization utility function.
      TrickHLAFederateConfig.initialize( self )

      return
