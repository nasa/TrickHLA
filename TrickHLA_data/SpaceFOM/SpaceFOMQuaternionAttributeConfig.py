##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level SpaceFOM input
#     file interface for specifying a Quaternion object attribute.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA/ER6) (May 2026) (--) (Initial implementation.)))
##############################################################################
import trick
from ..TrickHLA.TrickHLAAttributeConfig import *
from ..SpaceFOM.SpaceFOMQuaternionFixedRecordConfig import *


class SpaceFOMQuaternionAttributeConfig( TrickHLAAttributeConfig, SpaceFOMQuaternionFixedRecordConfig ):

   def __init__( self,
                 quat_FOM_name,
                 quat_trick_name      = None,
                 quat_publish         = True,
                 quat_subscribe       = True,
                 quat_locally_owned   = True,
                 quat_config          = trick.TrickHLA.CONFIG_CYCLIC,
                 quat_preferred_order = trick.TrickHLA.TRANSPORT_SPECIFIED_IN_FOM ):

      TrickHLAAttributeConfig.__init__(
                                 self,
                                 FOM_name        = quat_FOM_name,
                                 trick_name      = quat_trick_name,
                                 publish         = quat_publish,
                                 subscribe       = quat_subscribe,
                                 locally_owned   = quat_locally_owned,
                                 config          = quat_config,
                                 rti_encoding    = trick.TrickHLA.ENCODING_FIXED_RECORD,
                                 preferred_order = quat_preferred_order )

      SpaceFOMQuaternionFixedRecordConfig.__init__(
                                 self,
                                 quat_rec_trick_name = quat_trick_name )

      return
