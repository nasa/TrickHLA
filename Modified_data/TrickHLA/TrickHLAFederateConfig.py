##############################################################################
# PURPOSE:
#    (This is a python class file used to create the top level TrickHLA input
#     file interface for specifying a federate.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA) (Jan 2019) (--) (SpaceFOM support and testing.))
#     ((Dan Dexter) (NASA) (June 2021) (--) (Added set_wait_status_time.))
#     ((Dan Dexter) (NASA) (July 2023) (--) (Added support for HLA base time units.)))
##############################################################################
import socket
import subprocess
import sys
import trick


class TrickHLAFederateConfig( object ):

   # Ties to TrickHLA from simulation.
   federate    = None
   manager     = None
   control     = None
   config      = None
   enabled     = True
   sim_config  = None
   initialized = False

   # Least Common Time Step, in seconds.
   lcts = 1.0

   # Time padding for mode transitions, in seconds.
   time_padding = 0.0

   # S_define name for the ExecutionConfiguration object.
   config_S_define_name = None

   # List of known federates.
   known_federates = []

   # List of FOM modules used by this federate.
   fom_modules = []

   # List of multi-phase initialization sync points.
   init_sync_points = []

   # List of TrickHLA simulation objects associated with this federate.
   sim_objects = []

   # List of federation objects this federate recognizes.
   fed_objects = []

   # List of federation interactions this federate recognizes.
   fed_interactions = []

   def __init__( self,
                 thla_federate,
                 thla_manager,
                 thla_control,
                 thla_config,
                 thla_federation_name,
                 thla_federate_name,
                 thla_enabled = True ):

      self.federate = thla_federate
      self.manager = thla_manager
      self.enabled = thla_enabled
      self.control = thla_control
      self.config = thla_config

      self.set_federation_name( thla_federation_name )
      self.set_federate_name( thla_federate_name )

      # Set up the default HLA time management policy.
      self.setup_HLA_time_management()

      return


   def initialize( self ):

      # You can only initialize once.
      if self.initialized:
         print( 'TrickHLAFederateConfig.initialize(): Warning: Already initialized! Ignoring!' )
         return

      # Loop through the FOM modules appending them as a single comma
      # separated string.
      foms_string = ''
      for i in range( len( self.fom_modules ) ):
         if i == 0:
            foms_string = str( self.fom_modules[0] )
         else:
            foms_string += ', ' + str( self.fom_modules[i] )
      self.federate.FOM_modules = foms_string

      # Check to see if the objects are pre-configured in default data jobs
      # or if they are being allocated and configured in the input file.
      # This is an all-or-nothing choice.  All objects are either configured
      # in default data or here in the input file.
      if self.manager.obj_count == 0:

         # Note: The federate objects are added to a list in a simulation
         # specific routine. This code assumes that everything has been set
         # in the TrickHLAObjectConfig based objects except the TrickHLAObject
         # references. This code will create the appropriate number of
         # TrickHLAObject and then use the TrickHLAObjectConfig instances to
         # initialize them. This will allow users to add or delete objects
         # without having to manage array size.

         # Allocate the federate's federation object list.
         self.manager.obj_count = len( self.fed_objects )
         if self.manager.obj_count:
            self.manager.objects = trick.alloc_type( self.manager.obj_count,
                                                     'TrickHLA::Object' )

         # Loop through the federation objects and initialize them.
         for indx in range( 0, self.manager.obj_count ):
            self.fed_objects[indx].initialize( self.manager.objects[indx] )

      # Check to see if the interactions are pre-configured in default data jobs
      # or if they are being allocated and configured in the input file.
      # This is an all-or-nothing choice.  All interaction are either configured
      # in default data or here in the input file.
      if self.manager.inter_count == 0:

         # Note: The federate interactions are added to a list in a simulation
         # specific routine. This code assumes that everything has been set
         # in the TrickHLAInteractionConfig based interactions except the
         # TrickHLAInteraction references. This code will create the
         # appropriate number of TrickHLAInteraction and then use the
         # TrickHLAInitializationConfig instances to initialize them. This
         # will allow users to add or delete objects without having to manage
         # array size.

         # Allocate the federate's federation interactions list.
         self.manager.inter_count = len( self.fed_interactions )
         if self.manager.inter_count:
            self.manager.interactions = trick.alloc_type( self.manager.inter_count,
                                                          'TrickHLA::Interaction' )

         # Loop through the federation interactions and initialize them.
         for indx in range( 0, self.manager.inter_count ):
            self.fed_interactions[indx].initialize( self.manager.interactions[indx] )

      # Loop through the known federates and add them.
      if len( self.known_federates ):
         self.federate.enable_known_feds = True
         self.federate.known_feds_count = len( self.known_federates )
         self.federate.known_feds = trick.sim_services.alloc_type( self.federate.known_feds_count,
                                                                   "TrickHLA::KnownFederate" )
         indx = 0
         for known_federate in self.known_federates:
            self.federate.known_feds[indx].name = str( known_federate[1] )
            self.federate.known_feds[indx].required = known_federate[0]
            indx += 1

      else:
         # Disable known federates if none have been added.
         self.federate.enable_known_feds = False

      # Loop through the multi-phase initialization sync points appending them
      # as a single comma separated string.
      sync_point_string = ''
      for i in range( len( self.init_sync_points ) ):
         if i == 0:
            sync_point_string = str( self.init_sync_points[0] )
         else:
            sync_point_string += ', ' + str( self.init_sync_points[i] )
      self.control.multiphase_init_sync_points = sync_point_string

      # Marks this as initialized.
      self.initialized = True

      # Check if HLA interfaces are enabled or not.
      if not self.enabled:
         self.disable()

      return
   
   
   def fix_var_server_source_address( self ):
      # The Trick variable server uses the local host name without verifying the
      # IP address it resolves to is actually used by the local host computer.
      # Verify the IP address and fallback to 127.0.0.1 if we find a discrepancy.
      # Otherwise the simulation control panel will not successfully connect.
      try:
         if ( trick.var_server_get_hostname() == socket.gethostname() ):
            host_ip_addr = socket.gethostbyname( socket.gethostname() )
            try:
               ifconfig_out = subprocess.check_output( ['ifconfig'] ).decode()
               if ( ifconfig_out.find( host_ip_addr ) < 0 ):
                  print( 'WARNING: Invalid IP address ' + host_ip_addr
                         + ' resolved for host \'' + trick.var_server_get_hostname()
                         + '\', setting the variable server source address to 127.0.0.1!' )
                  trick.var_server_set_source_address( '127.0.0.1' )
            except:
               return  # Use host source address as is.
      except ( socket.error, socket.gaierror, socket.herror, socket.timeout ):
         print( 'WARNING: Problem resolving \'' + trick.var_server_get_hostname()
                + '\' host name to an address, setting the variable server source address to 127.0.0.1!' )
         trick.var_server_set_source_address( '127.0.0.1' )
         
      return


   def disable_trick_child_thread_associations( self, disable_thread_ids ):

      # You can only disable thread IDS before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.disable_trick_child_thread_associations(): Warning, already initialized, function ignored!' )
      else:
         self.federate.disable_trick_child_thread_associations( str( disable_thread_ids ) )

      return


   def set_federation_name( self, name ):

      # You can only set federation name before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_federation_name(): Warning, already initialized, function ignored!' )
      else:
         self.federate.federation_name = str( name )

      return


   def set_federate_name( self, name ):

      # You can only set federate name before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_federate_name(): Warning, already initialized, function ignored!' )
      else:
         self.federate.name = str( name )

      return


   def set_HLA_base_time_units( self, new_base_time_units ):

      # You can only set HLA base time units before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_HLA_base_time_units(): Warning, already initialized, function ignored!' )
      else:
         self.federate.set_HLA_base_time_units( new_base_time_units )

      return


   def scale_trick_tics_to_base_time_units( self ):

      # You can only adjust the Trick Tick value before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.scale_trick_tics_to_base_time_units(): Warning, already initialized, function ignored!' )
      else:
         self.federate.scale_trick_tics_to_base_time_units()

      return


   def set_lookahead_time( self, lookahead_time: float ):

      # You can only set lookahead time before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_lookahead_time(): Warning, already initialized, function ignored!' )
      else:
         if not isinstance( lookahead_time, float ):
            sys.exit( 'TrickHLAFederateConfig.set_lookahead_time(): ERROR: The lookahead time specified (lookahead_time:' + str( lookahead_time ) + ' ) must be a float representing seconds! ' )

         self.federate.set_lookahead( lookahead_time )

      return


   def set_least_common_time_step( self, new_lcts: float ):

      # You can only set LCTS before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_least_common_time_step(): Warning, already initialized, function ignored!' )
         return

      # To protect against the previous API that took time in microseconds, make
      # sure the user specifies floating-point time in seconds.
      if not isinstance( new_lcts, float ):
         sys.exit( 'TrickHLAFederateConfig.set_least_common_time_step(): ERROR: The Least Common Time Step specified (new_lcts:' + str( new_lcts ) + ' ) must be a float representing seconds! ' )

      # Set the new Least Common Time Step value.
      self.lcts = new_lcts
      self.control.set_least_common_time_step( self.lcts )

      return


   def set_time_padding( self, new_padding: float ):

      # You can only set passing before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_time_padding(): Warning, already initialized, function ignored!' )
         return

      if not isinstance( new_padding, float ):
         sys.exit( 'TrickHLAFederateConfig.set_time_padding(): ERROR: The time padding specified (new_padding:' + str( new_padding ) + ' ) must be a float representing seconds! ' )

      # Set the new time padding value.
      self.time_padding = new_padding
      self.control.set_time_padding( self.time_padding )

      return


   def set_time_constrained( self, time_constrained ):

      # You can only set time constrained before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_time_constrained(): Warning, already initialized, function ignored!' )
      else:
         self.federate.time_constrained = time_constrained

      return


   def set_time_regulating( self, time_regulating ):

      # You can only set time regulating before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_time_regulating(): Warning, already initialized, function ignored!' )
      else:
         self.federate.time_regulating = time_regulating

      return


   def set_time_management( self, time_management ):

      # You can only set time management before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.set_time_management(): Warning, already initialized, function ignored!' )
      else:
         self.federate.time_management = time_management

      return


   def setup_HLA_time_management( self,
                                  lookahead       = 1.0,
                                  constrained     = True,
                                  regulating      = True,
                                  time_managed    = True,
                                  base_time_units = trick.HLA_BASE_TIME_MICROSECONDS ):

      # You can only setup parameters before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.setup_HLA_time_management(): Warning, already initialized, function ignored!' )
      else:
         self.set_HLA_base_time_units( base_time_units )
         self.set_lookahead_time( lookahead )
         self.set_time_constrained( constrained )
         self.set_time_regulating( regulating )
         self.set_time_management( time_managed )

      return


   def add_sim_object( self, sim_object ):

      # You can only add simulation objects before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.add_sim_object(): Warning, already initialized, function ignored!' )
      else:
         self.sim_objects.append( sim_object )

      return


   def add_fed_object( self, fed_object ):

      # You can only add federation objects before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.add_fed_object(): Warning, already initialized, function ignored!' )
      else:
         self.fed_objects.append( fed_object )

      return


   def add_fed_interaction( self, fed_interaction ):

      # You can only add federation interactions before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.add_fed_interaction(): Warning, already initialized, function ignored!' )
      else:
         self.fed_interactions.append( fed_interaction )

      return


   def enable( self ):

      self.enabled = True

      # Turn on all the HLA sim objects associated with this HLA federate.
      for sim_object in self.sim_objects:
         trick.exec_set_sim_object_onoff( sim_object, True )

      return


   def disable( self ):

      self.enabled = False

      # Turn off all the HLA sim objects associated with this HLA federate.
      for sim_object in self.sim_objects:
         trick.exec_set_sim_object_onoff( sim_object, False )

      return


   def set_debug_level( self, debug_level ):

      self.federate.debug_level = debug_level

      return


   def set_debug_source( self, debug_source ):

      self.federate.code_section = debug_source

      return


   def set_wait_status_time( self, wait_status_time: float ):

      # Set the time in seconds TrickHLA will wait in a any spin-lock
      # before it will print a status message to the console.
      self.federate.wait_status_time = wait_status_time

      return


   def add_known_federate( self, is_required, name ):

      # You can only add known federates before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.add_known_federate(): Warning, already initialized, function ignored!' )
      else:
         self.known_federates.append( ( is_required, str( name ) ) )

      return


   def add_FOM_module( self, name ):

      # You can only add FOM modules before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.add_FOM_module(): Warning, already initialized, function ignored!' )
      else:
         self.fom_modules.append( str( name ) )

      return


   def add_multiphase_init_sync_point( self, sync_point ):

      # You can only add sync points before initialize method is called.
      if self.initialized:
         print( 'TrickHLAFederateConfig.add_multiphase_init_sync_point(): Warning, already initialized, function ignored!' )
      else:
         self.init_sync_points.append( str( sync_point ) )

      return
