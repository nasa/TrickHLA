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
   hla_lag_comp_instance    = None
   hla_ownership_instance   = None
   hla_packing_instance     = None
   hla_manager_object       = None
   hla_blocking_cyclic_read = False

   # List of TrickHLA object attributes.
   attributes = None

   def __init__( self,
                 thla_create               = False,
                 thla_instance_name        = None,
                 thla_FOM_name             = None,
                 thla_lag_comp_instance    = None,
                 thla_ownership_instance   = None,
                 thla_packing_instance     = None,
                 thla_manager_object       = None,
                 thla_blocking_cyclic_read = False ):

      # Allocate and empty attribute list.
      self.attributes = []

      # Set the Trick HLA object reference here so the set() function calls will
      # work as expected. Normally this is postponed until initialization.
      if thla_manager_object != None :
         self.hla_manager_object = thla_manager_object

      # Specify if this object instance created locally.
      self.set_create( thla_create )

      # Set lag compensation if specified and not None.
      if thla_lag_comp_instance != None :
         self.set_lag_comp_instance( thla_lag_comp_instance )

      # Set ownership if specified and not None.
      if thla_ownership_instance != None :
         self.set_ownership_instance( thla_ownership_instance )

      # Set packing if specified and not None.
      if thla_packing_instance != None :
         self.set_packing_instance( thla_packing_instance )

      # Set the object instance name if specified.
      if thla_instance_name != None :
         self.set_instance_name( thla_instance_name )

      # Set the object instance name if specified.
      if thla_FOM_name != None :
         self.set_FOM_name( thla_FOM_name )

      # Specify if this object will block on cyclic reads.
      self.set_blocking_cyclic_read( thla_blocking_cyclic_read )

      # Still need to set the object attributes but this is left to the
      # specific implementation classes.

      return


   def initialize( self, thla_manager_object = None ):

      # Assign the associated TrickHLA object if specified.
      if thla_manager_object != None :
         self.hla_manager_object = thla_manager_object
      elif self.hla_manager_object == None :
         trick.exec_terminate_with_return( -1,
                                           sys._getframe(0).f_code.co_filename,
                                           sys._getframe(0).f_lineno,
                                           'TrickHLA::Object reference not set.' )

      # Now make sure that the TrickHLAObject data is set.
      self.set_instance_name( self.hla_instance_name )
      self.set_FOM_name( self.hla_FOM_name )
      self.set_packing_instance( self.hla_packing_instance )
      self.set_create( self.hla_create )
      self.set_blocking_cyclic_read( self.hla_blocking_cyclic_read )

      if self.hla_lag_comp_instance != None :
         self.set_lag_comp_instance( self.hla_lag_comp_instance )

      if self.hla_ownership_instance != None :
         self.set_ownership_instance( self.hla_ownership_instance )

      # Allocate the federate object's attribute list.
      self.hla_manager_object.attr_count = len(self.attributes)
      self.hla_manager_object.attributes = trick.TMM_declare_var_1d( 'TrickHLA::Attribute', 
                                                                     self.hla_manager_object.attr_count )

      # Loop through the federation object attributes and initialize them.
      for indx in range( 0, self.hla_manager_object.attr_count ):
         self.attributes[indx].initialize( self.hla_manager_object.attributes[indx] )

      return


   def set_create( self, create_obj ):
 
      self.hla_create = create_obj
      if self.hla_manager_object != None :
         self.hla_manager_object.create_HLA_instance = self.hla_create

      return


   def get_create( self ):

      return self.hla_create


   def set_instance_name( self, name ):

      self.hla_instance_name = str(name)
      if self.hla_manager_object != None :
         self.hla_manager_object.name = str(name)

      return


   def get_instance_name( self ):

      return str( self.hla_instance_name )


   def set_FOM_name( self, name ):

      self.hla_FOM_name = str(name)
      if self.hla_manager_object != None :
         self.hla_manager_object.FOM_name = str(name)

      return


   def get_FOM_name( self ):

      return str( self.hla_FOM_name )


   def set_lag_comp_instance( self, obj_lag_comp):
 
      self.hla_lag_comp_instance = obj_lag_comp
      if self.hla_manager_object != None :
         self.hla_manager_object.lag_comp = obj_lag_comp
         self.hla_manager_object.lag_comp_type = trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE 

      return

   def set_ownership_instance( self, obj_ownership_handler):
 
      self.hla_ownership_instance = obj_ownership_handler
      if self.hla_manager_object != None :
         self.hla_manager_object.ownership = obj_ownership_handler

      return

   def set_packing_instance( self, obj_packing ):
 
      self.hla_packing_instance = obj_packing
      if self.hla_manager_object != None :
         self.hla_manager_object.packing = obj_packing

      return


   def get_packing_instance( self ):

      return self.hla_packing_instance


   def add_attribute( self, attribute ):

      self.attributes.append( attribute )

      return

   def set_blocking_cyclic_read( self, blocking_cyclic_read ):

      self.hla_blocking_cyclic_read = blocking_cyclic_read
      if self.hla_manager_object != None :
         self.hla_manager_object.blocking_cyclic_read = self.hla_blocking_cyclic_read

      return

   def get_blocking_cyclic_read( self ):

      return self.hla_blocking_cyclic_read
