##############################################################################
# PURPOSE:
#    (This is a python input file class to set up the general parameters that
#     describe a Space Reference FOM federate.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base SpaceFOMFederateConfig class.)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.)))
#    (((Dan Dexter) (NASA/ER6) (Oct 2025) (--) (Updated to add extended ExCO attributes.)))
##############################################################################
import trick
from trick.top import *
from ..SpaceFOM.SpaceFOMFederateConfig import *

class SpaceFOMFederateConfig2( SpaceFOMFederateConfig ):

   def __init__( self,
                 thla_federate,
                 thla_manager,
                 thla_control,
                 thla_config,
                 thla_federation_name,
                 thla_federate_name,
                 thla_enabled,
                 FOMs_path = None ):

      # Call the base class constructor.
      SpaceFOMFederateConfig.__init__( self,
                                       thla_federate,
                                       thla_manager,
                                       thla_control,
                                       thla_config,
                                       thla_federation_name,
                                       thla_federate_name,
                                       thla_enabled,
                                       FOMs_path )

      # Add in additional required Space FOM modules.
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_management2.xml' )

      return


   def set_FOMs_path( self, FOMs_path ):

      # You can only set SpaceFOM FOM module path before initialize method is called.
      if self.initialized:
         print( 'SpaceFOMFederateConfig2.set_FOMs_path(): Warning, already initialized! Ignoring!' )
         return

      # Call the parent function to ensure those paths are fixed.
      SpaceFOMFederateConfig.set_FOMs_path( FOMs_path )

      # Remove the FOM module with the old path.
      for item in self.fom_modules:
         if item.find( 'SISO_SpaceFOM_management2.xml' ) != -1:
            indx = self.fom_modules.index( item )
            self.fom_modules.pop( indx )

      # Add the module back in with the specified FOM module path prefix.
      self.add_FOM_module( self.SpaceFOMs_path + 'SISO_SpaceFOM_management2.xml' )

      return
