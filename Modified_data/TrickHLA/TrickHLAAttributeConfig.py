##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level TrickHLA input
#     file interface for specifying a federate object attribute.)
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
import trick


class TrickHLAAttributeConfig( object ):

   FOM_name      = None
   trick_name    = None
   publish       = True
   subscribe     = True
   locally_owned = True
   config        = trick.TrickHLA.CONFIG_CYCLIC
   rti_encoding  = trick.TrickHLA.ENCODING_UNKNOWN

   def __init__( self,
                 FOM_name,
                 trick_name,
                 publish       = True,
                 subscribe     = True,
                 locally_owned = True,
                 config        = trick.TrickHLA.CONFIG_CYCLIC,
                 rti_encoding  = trick.TrickHLA.ENCODING_UNKNOWN ):

      self.FOM_name = FOM_name
      self.trick_name = trick_name
      self.publish = publish
      self.subscribe = subscribe
      self.locally_owned = locally_owned
      self.config = config
      self.rti_encoding = rti_encoding

      return

   def initialize( self, attribute ):

      attribute.FOM_name = self.FOM_name
      attribute.trick_name = self.trick_name
      attribute.publish = self.publish
      attribute.subscribe = self.subscribe
      attribute.locally_owned = self.locally_owned
      attribute.config = self.config
      attribute.rti_encoding = self.rti_encoding

      return

