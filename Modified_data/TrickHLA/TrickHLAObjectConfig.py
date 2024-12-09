##############################################################################
# PURPOSE:
#    (This is a python class file used to by the top level TrickHLA input
#     file interface for specifying a federate object.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.))
#     ((Dan Dexter) (NASA/ER6) (Aug 2020) (--) (Added blocking_cyclic_read support.)))
##############################################################################
import sys
import trick

class TrickHLAObjectConfig( object ):

   # Ties to TrickHLA from simulation.
   hla_create               = False
   hla_instance_name        = None
   hla_FOM_name             = None
   hla_object               = None
   hla_packing_instance     = None
   hla_conditional_instance = None
   hla_lag_comp_instance    = None
   hla_lag_comp_type        = trick.TrickHLA.LAG_COMPENSATION_NONE
   hla_ownership_instance   = None
   hla_deleted_instance     = None
   hla_thread_IDs           = None
   hla_attribute_publish    = False
   hla_attribute_subscribe  = False
   hla_attribute_config     = None
   hla_blocking_cyclic_read = False

   # List of TrickHLA object attributes.
   attributes = None

   def __init__( self,
                 thla_create               = False,
                 thla_instance_name        = None,
                 thla_FOM_name             = None,
                 thla_lag_comp_instance    = None,
                 thla_lag_comp_type        = trick.TrickHLA.LAG_COMPENSATION_NONE,
                 thla_ownership_instance   = None,
                 thla_deleted_instance     = None,
                 thla_conditional_instance = None,
                 thla_packing_instance     = None,
                 thla_object               = None,
                 thla_thread_IDs           = None,
                 thla_attribute_publish    = None,
                 thla_attribute_subscribe  = None,
                 thla_attribute_config     = None,
                 thla_blocking_cyclic_read = False ):

      # Allocate and empty attribute list.
      self.attributes = []

      # Set the Trick HLA object reference here so the set() function calls will
      # work as expected. Normally this is postponed until initialization.
      if thla_object != None:
         self.hla_object = thla_object

      # Specify if this object instance created locally.
      self.set_create( thla_create )

      # Set packing if specified and not None.
      if thla_packing_instance != None:
         self.set_packing_instance( thla_packing_instance )

      # Set conditional if specified and not None.
      if thla_conditional_instance != None:
         self.set_conditional_instance( thla_conditional_instance )

      # Set lag compensation if specified and not None.
      if thla_lag_comp_instance != None:
         self.set_lag_comp_instance( thla_lag_comp_instance )

      # Set lag compensation type if different than default.
      if thla_lag_comp_type != self.hla_lag_comp_type:
         self.set_lag_comp_type( thla_lag_comp_type )

      # Set ownership if specified and not None.
      if thla_ownership_instance != None:
         self.set_ownership_instance( thla_ownership_instance )

      # Set deleted callback if specified and not None.
      if thla_deleted_instance != None:
         self.set_deleted_instance( thla_deleted_instance )

      # Set the object instance name if specified.
      if thla_instance_name != None:
         self.set_instance_name( thla_instance_name )

      # Set the object FOM name if specified.
      if thla_FOM_name != None:
         self.set_FOM_name( thla_FOM_name )

      # Set the Trick thread-IDs associated with this object.
      if thla_thread_IDs != None:
         self.set_thread_IDs( thla_thread_IDs )

      # By default, all attributes are published if the object instance is
      # created and it can be overridden if the attribute-publish flag is set.
      if thla_attribute_publish != None:
         self.set_attribute_publish( thla_attribute_publish )
      else:
         self.set_attribute_publish( thla_create )

      # By default, all attributes are subscribed if the object instance is Not
      # created and it can be overridden if the attribute-subscribe flag is set.
      if thla_attribute_subscribe != None:
         self.set_attribute_subscribe( thla_attribute_subscribe )
      else:
         self.set_attribute_subscribe( not thla_create )

      # Set the TrickHLA Attribute config.
      if thla_attribute_config != None:
         self.set_attribute_config( thla_attribute_config )

      # Specify if this object will block on cyclic reads.
      self.set_blocking_cyclic_read( thla_blocking_cyclic_read )

      # Still need to set the object attributes but this is left to the
      # specific implementation classes.

      return


   def initialize( self, thla_object = None ):

      # Assign the associated TrickHLA object if specified.
      if thla_object != None:
         self.hla_object = thla_object
      elif self.hla_object == None:
         trick.exec_terminate_with_return( -1,
                                           sys._getframe(0).f_code.co_filename,
                                           sys._getframe(0).f_lineno,
                                           'TrickHLA::Object reference not set.' )

      # Now make sure that the TrickHLAObject data is set.
      self.set_instance_name( self.hla_instance_name )
      self.set_FOM_name( self.hla_FOM_name )
      self.set_packing_instance( self.hla_packing_instance )
      self.set_create( self.hla_create )
      self.set_thread_IDs( self.hla_thread_IDs )
      self.set_blocking_cyclic_read( self.hla_blocking_cyclic_read )

      if self.hla_lag_comp_instance != None:
         self.set_lag_comp_instance( self.hla_lag_comp_instance )
         self.set_lag_comp_type( self.hla_lag_comp_type )

      if self.hla_ownership_instance != None:
         self.set_ownership_instance( self.hla_ownership_instance )

      # Allocate the federate object's attribute list.
      self.hla_object.attr_count = len( self.attributes )
      self.hla_object.attributes = trick.TMM_declare_var_1d( 'TrickHLA::Attribute', 
                                                             self.hla_object.attr_count )

      # Loop through the federation object attributes and initialize them.
      for indx in range( 0, self.hla_object.attr_count ):
         self.attributes[indx].initialize( self.hla_object.attributes[indx] )

      return


   def set_create( self, create_obj ):
 
      self.hla_create = create_obj
      if self.hla_object != None:
         self.hla_object.create_HLA_instance = self.hla_create

      return


   def get_create( self ):

      return self.hla_create


   def set_instance_name( self, name ):

      self.hla_instance_name = str(name)
      if self.hla_object != None:
         self.hla_object.name = str(name)

      return


   def get_instance_name( self ):

      return str( self.hla_instance_name )


   def set_FOM_name( self, name ):

      self.hla_FOM_name = str(name)
      if self.hla_object != None:
         self.hla_object.FOM_name = str(name)

      return


   def get_FOM_name( self ):

      return str( self.hla_FOM_name )


   def set_thread_IDs( self, threadIDs ):

      self.hla_thread_IDs = str(threadIDs)
      if self.hla_object != None:
         self.hla_object.thread_ids = str(threadIDs)

      return


   def get_thread_IDs( self ):

      return str( self.hla_thread_IDs )


   def set_lag_comp_instance( self, obj_lag_comp):
 
      self.hla_lag_comp_instance = obj_lag_comp
      if self.hla_object != None:
         self.hla_object.lag_comp = obj_lag_comp
         self.hla_object.lag_comp_type = trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE 

      return
   
   
   def set_lag_comp_type( self, lag_comp_type ):

      self.hla_lag_comp_type = lag_comp_type
      if self.hla_object != None:
         self.hla_object.lag_comp_type = lag_comp_type

      return


   def set_deleted_instance( self, obj_deleted):
 
      self.hla_deleted_instance = obj_deleted
      if self.hla_object != None:
         self.hla_object.deleted = obj_deleted

      return


   def set_ownership_instance( self, obj_ownership_handler ):
 
      self.hla_ownership_instance = obj_ownership_handler
      if self.hla_object != None:
         self.hla_object.ownership = obj_ownership_handler

      return


   def set_conditional_instance( self, obj_conditional ):
 
      self.hla_conditional_instance = obj_conditional
      if self.hla_object != None:
         self.hla_object.conditional = obj_conditional

      return


   def set_packing_instance( self, obj_packing ):
 
      self.hla_packing_instance = obj_packing
      if self.hla_object != None:
         self.hla_object.packing = obj_packing

      return


   def get_packing_instance( self ):

      return self.hla_packing_instance


   def add_attribute( self, attribute ):

      self.attributes.append( attribute )

      return


   def set_attribute_publish( self, publish ):

      self.hla_attribute_publish = publish

      return


   def get_attribute_publish( self ):

      return self.hla_attribute_publish


   def set_attribute_subscribe( self, subscribe ):

      self.hla_attribute_subscribe = subscribe

      return


   def get_attribute_subscribe( self ):

      return self.hla_attribute_subscribe


   def set_attribute_config( self, attribute_config ):

      self.hla_attribute_config = attribute_config

      return


   def get_attribute_config( self ):

      return self.hla_attribute_config


   def set_blocking_cyclic_read( self, blocking_cyclic_read ):

      self.hla_blocking_cyclic_read = blocking_cyclic_read
      if self.hla_object != None:
         self.hla_object.blocking_cyclic_read = self.hla_blocking_cyclic_read

      return


   def get_blocking_cyclic_read( self ):

      return self.hla_blocking_cyclic_read
