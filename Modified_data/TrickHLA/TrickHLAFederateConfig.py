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
#    (((Edwin Z. Crues) (NASA) (Jan 2019) (--) (SpaceFOM support and testing.)))
##############################################################################
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
   
   # Least Common Time Step
   lcts = 1.0
   
   # Time padding for mode transitions.
   time_padding = 3.0
   
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
      self.manager  = thla_manager
      self.endabled = thla_enabled
      self.control  = thla_control
      self.config   = thla_config

      self.set_federation_name(thla_federation_name)
      self.set_federate_name(thla_federate_name)

      # Set up the default time management policy.
      self.set_time_management( )

      return


   def initialize( self ):
      
      # You can only initialize once.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized!  Ignored!'
         return

      # Loop through the FOM modules appending them as a single comma
      # separated string.
      foms_string = ''
      for i in range(len(self.fom_modules)):
         if i == 0:
            foms_string = str(self.fom_modules[0])
         else:
            foms_string += ', ' + str(self.fom_modules[i])
      self.federate.FOM_modules = foms_string

      # Note: The federate objects are added to a list in a simulation
      # specific routine. This code assumes that everything has been set
      # in the TrickHLAObjectConfig based objects except the TrickHLAObject
      # references. This code will create the appropriate number of
      # TrickHLAObject and then use the TrickHLAObjectConfig instances to
      # initialize them. This will allow users to add or delete objects
      # without having to manage array size.

      # Allocate the federate's federation object list.
      self.manager.obj_count = len(self.fed_objects)
      if self.manager.obj_count:
         self.manager.objects = trick.alloc_type( self.manager.obj_count,
                                                  'TrickHLA::Object'        )

      # Loop through the federation objects and initialize them.
      for indx in range( 0, self.manager.obj_count ):
         self.fed_objects[indx].initialize( self.manager.objects[indx] )

      # Note: The federate interactions are added to a list in a simulation
      # specific routine. This code assumes that everything has been set
      # in the TrickHLAInteractionConfig based interactions except the
      # TrickHLAInteraction references. This code will create the
      # appropriate number of TrickHLAInteraction and then use the
      # TrickHLAInitializationConfig instances to initialize them. This
      # will allow users to add or delete objects without having to manage
      # array size.

      # Allocate the federate's federation interactions list.
      self.manager.inter_count = len(self.fed_interactions)
      if self.manager.inter_count:
         self.manager.interactions = trick.alloc_type( self.manager.inter_count,
                                                       'TrickHLA::Interaction'    )

      # Loop through the federation interactions and initialize them.
      for indx in range( 0, self.manager.inter_count ):
         self.fed_interactions[indx].initialize( self.manager.interactions[indx] )

      # Loop through the known federates and add them.
      if len(self.known_federates) :
         self.federate.enable_known_feds = True
         self.federate.known_feds_count  = len(self.known_federates)
         self.federate.known_feds = trick.sim_services.alloc_type( self.federate.known_feds_count,
                                                                   "TrickHLA::KnownFederate" )
         indx = 0
         for known_federate in self.known_federates:
            self.federate.known_feds[indx].name     = str(known_federate[1])
            self.federate.known_feds[indx].required = known_federate[0]
            indx += 1
            
      else:
         # Disable know federates if none have been added.
         self.federate.enable_known_feds = False

      # Loop through the multi-phase initialization sync points appending them
      # as a single comma separated string.
      sync_point_string = ''
      for i in range(len(self.init_sync_points)):
         if i == 0:
            sync_point_string = str(self.init_sync_points[0])
         else:
            sync_point_string += ', ' + str(self.init_sync_points[i])
      self.control.multiphase_init_sync_points = sync_point_string

      # Marks this as initialized.
      self.initialized = True

      # Check if HLA interfaces are enabled or not.
      if not self.enabled:
         self.disable()

      return


   def set_federation_name( self, name ):

      # You can only set federation name before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set_federation_name ignored!'
      else:
         self.federate.federation_name = str(name)

      return


   def set_federate_name( self, name ):

      # You can only set federate name before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set_federate_name ignored!'
      else:
         self.federate.name = str(name)

      return


   def set_lookahead_time( self, lookahead_time ) :
      
      # You can only set lookahead time before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set_lookahead_time ignored!'
      else:
         self.federate.set_lookahead( lookahead_time )
         
      return


   def set_least_common_time_step( self, new_lcts ):
      
      # You can only set sim_config path before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set least common time step ignored!'
         return

      # Set the new Least Common Time Step value.
      self.lcts = int(new_lcts)
      self.control.set_least_common_time_step( self.lcts )

      return


   def set_time_padding( self, new_padding ):
      
      # You can only set sim_config path before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set time padding ignored!'
         return
      
      # Set the new time padding value.
      self.time_padding = float(new_padding)
      self.control.set_time_padding( self.time_padding )

      return


   def set_time_constrained( self, time_constrained ) :
      
      # You can only set time constrained before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set_time_constrained ignored!'
      else:
         self.federate.time_constrained = time_constrained
         
      return


   def set_time_regulating( self, time_regulating ) :
      
      # You can only set time regulating before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set_time_regulating ignored!'
      else:
         self.federate.time_regulating = time_regulating
         
      return


   def set_time_management( self,
      lookahead   = 1.0,
      constrained = True,
      regulating  = True ):

      # You can only set time management before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, set_time_management ignored!'
      else:
         self.set_lookahead_time( lookahead )
         self.set_time_constrained( constrained )
         self.set_time_regulating( regulating )

      return


   def add_sim_object( self, sim_object ):

      # You can only add simulation objects before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, add_sim_object ignored!'
      else:
         self.sim_objects.append( sim_object )

      return


   def add_fed_object( self, fed_object ):

      # You can only add federation objects before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, add_fed_object ignored!'
      else:
         self.fed_objects.append( fed_object )

      return


   def add_fed_interaction( self, fed_interaction ):

      # You can only add federation interactions before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, add_fed_interaction ignored!'
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


   def add_known_fededrate( self, is_required, name ):

      # You can only add known federates before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, add_known_fededrate ignored!'
      else:
         self.known_federates.append( (is_required, str(name)) )

      return


   def add_FOM_module( self, name ):

      # You can only add FOM modules before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, add_FOM_module ignored!'
      else:
         self.fom_modules.append( str(name) )

      return


   def add_multiphase_init_sync_point( self, sync_point ):

      # You can only add sync points before initialize method is called.
      if self.initialized :
         print 'Warning: TrickHLAFederateConfig already initialized, add_multiphase_init_sync_point ignored!'
      else:
         self.init_sync_points( str(sync_point) )

      return

