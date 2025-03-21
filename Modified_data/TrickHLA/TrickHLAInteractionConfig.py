##############################################################################
# PURPOSE:
#    (This is a python class file used to by the top level TrickHLA input
#     file interface for specifying a federate interaction.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.)))
##############################################################################
import sys
import trick

class TrickHLAInteractionConfig( object ):

   # Ties to TrickHLA from simulation.
   hla_create                = False
   hla_FOM_name              = None
   hla_publish               = True
   hla_subscribe             = False
   hla_handler_instance      = None
   hla_handler_instance_name = None
   hla_manager_interaction   = None
   

   # List of TrickHLA object attributes.
   parameters = None

   def __init__( self,
                 thla_FOM_name              = None,
                 thla_publish               = True,
                 thla_subscribe             = True,
                 thla_handler_instance      = None,
                 thla_handler_instance_name = None,
                 thla_manager_interaction   = None ):

      # Allocate and empty parameters list.
      self.parameters = []

      # Set the Trick HLA object reference here so that the set() function calls
      # will work as expected. Normally, this is postponed until initialization.
      if thla_manager_interaction != None:
         self.hla_manager_interaction = thla_manager_interaction

      # Set the interaction FOM name if specified.
      if thla_FOM_name != None:
         self.set_FOM_name( thla_FOM_name )

      # Specify if this interaction is published or subscribed.
      self.hla_publish   = thla_publish
      self.hla_subscribe = thla_subscribe

      # Set interaction handler if specified and not None.
      if thla_handler_instance != None:
         self.set_handler_instance( thla_handler_instance )

      # Still need to set the object attributes but this is left to the
      # specific implementation classes.

      return


   def initialize( self, thla_manager_interaction = None ):

      # Assign the associated TrickHLA object if specified.
      if thla_manager_interaction != None:
         self.hla_manager_interaction = thla_manager_interaction
      elif self.hla_manager_interaction == None:
         trick.exec_terminate_with_return( -1,
                                           sys._getframe(0).f_code.co_filename,
                                           sys._getframe(0).f_lineno,
                                           'TrickHLAInteraction reference not set.' )

      # Now make sure that the TrickHLAInteraction data is set.
      self.set_FOM_name( self.hla_FOM_name )
      self.set_publish( self.hla_publish )
      self.set_subscribe( self.hla_subscribe )
      self.set_handler_instance( self.hla_handler_instance )

      # Allocate the federate object's attribute list.
      self.hla_manager_interaction.param_count = len(self.parameters)
      self.hla_manager_interaction.parameters = \
         trick.alloc_type( self.hla_manager_interaction.param_count,
                           'TrickHLA::Parameter'                     )

      # Loop through the federation interaction parameters and initialize them.
      for indx in range( 0, self.hla_manager_interaction.param_count ):
         self.parameters[indx].initialize( self.hla_manager_interaction.parameters[indx] )

      return


   def set_FOM_name( self, name ):

      self.hla_FOM_name = str(name)
      if self.hla_manager_interaction != None:
         self.hla_manager_interaction.FOM_name = str(name)

      return


   def get_FOM_name( self ):

      return str( self.hla_FOM_name )


   def set_publish( self, publish ):
 
      self.hla_publish = publish
      if self.hla_manager_interaction != None:
         self.hla_manager_interaction.publish = self.hla_publish

      return


   def get_publish( self ):

      return hla_publish


   def set_subscribe( self, subscribe ):
 
      self.hla_subscribe = subscribe
      if self.hla_manager_interaction != None:
         self.hla_manager_interaction.subscribe = self.hla_subscribe

      return


   def get_subscribe( self ):

      return hla_subscribe


   def set_handler_instance( self, inter_hndlr ):
 
      self.hla_handler_instance = inter_hndlr
      if self.hla_manager_interaction != None:
         self.hla_manager_interaction.handler = inter_hndlr

      return


   def get_handler_instance( self ):

      return self.hla_handler_instance


   def add_parameter( self, param ):

      self.parameters.append( param )

      return


