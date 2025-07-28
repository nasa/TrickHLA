####################################################################################################
# PURPOSE:
#    (These are python input file classes to connect HLA FOM parameters to simulation model
#     variables for a FluidDistributed2WayBus object.  There are two HLA object messages connected
#     to the FluidDistributed2WayBus; one to the Bus's mInData and one to its mOutData.  These
#     messages connect two Buses in different Federates to each other.  Each Bus publishes one
#     message to send to its counterpart, and subscribes to the the other message that is published
#     by its counterpart.  Each message is written with the mOutData of the Bus that publishes it,
#     is read into the mInData of the subscribing Bus.  For the purpose of this configuration, we
#     call the two Bus objects 'A' and 'B'.
#
# REFERENCE:
#    ()
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAObjectConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Jason Harvey) (CACI) (October 2024) (--) (Initial)))
####################################################################################################
import trick
from TrickHLA_data.TrickHLA.TrickHLAObjectConfig import *
from TrickHLA_data.TrickHLA.TrickHLAAttributeConfig import *

# NOTE this object shouldn't be instantiated directly; use the derived types below.
class FluidDistIfObjectConfig( TrickHLAObjectConfig ):

   direction = None
   
   # Implemented in derived classes below
   def initDirection( self, isBusA ):
      pass
      
   def __init__( self,
                 thla_federate_name,
                 bus_name,                          # trick variable name of the FluidDistributed2WayBus object
                 isBusA,                            # True if this is the 'A' FluidDistributed2WayBus of the pair
                 FOM_type = 'FluidDistIfData_6_4',  # FOM objectClass type
                 threadId = None ):                 # Trick child thread

      self.initDirection( isBusA )

      # Call the base class constructor and tell it the FOM object class.
      TrickHLAObjectConfig.__init__( self,
                                     thla_create               = ( '.mOutData' == self.direction), # this locally owns
                                     thla_instance_name        = thla_federate_name,
                                     thla_FOM_name             = FOM_type,
                                     thla_lag_comp_instance    = None,
                                     thla_lag_comp_type        = None,
                                     thla_ownership_instance   = None,
                                     thla_deleted_instance     = None,
                                     thla_conditional_instance = None,
                                     thla_packing_instance     = None,
                                     thla_object               = None,
                                     thla_thread_IDs           = threadId )
      
      # Build the object attribute list.
      self.add_attributes(bus_name)

   def add_attributes( self, bus_name ):
      # Map terms of a FluidDistIfFOM message to FluidDistributed2WayBus attributes:
      #                 FOM attribute name,      model variable name, encoding type 
      Attribute_Map = [('DemandModeFlag',        '.mDemandMode',      trick.TrickHLA.ENCODING_BOOLEAN),
                       ('FrameCount',            '.mFrameCount',      trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                       ('FrameLoopback',         '.mFrameLoopback',   trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                       ('Capacitance',           '.mCapacitance',     trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                       ('Source',                '.mSource',          trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                       ('Energy',                '.mEnergy',          trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                       ('FluidMixture',          '.mMoleFractions',   trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                       ('TraceCompoundsMixture', '.mTcMoleFractions', trick.TrickHLA.ENCODING_LITTLE_ENDIAN)]

      for attr in Attribute_Map:
          attribute = TrickHLAAttributeConfig( FOM_name      = attr[0],
                                               trick_name    = bus_name + self.direction + attr[1],
                                               publish       = True,
                                               subscribe     = True,
                                               locally_owned = self.hla_create,
                                               config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                               rti_encoding  = attr[2] )
          self.add_attribute( attribute )

#################################
# A to B HLA Object Configuration
#################################
class FluidDistIfAToBObjectConfig(FluidDistIfObjectConfig):
   # If the FluidDistributed2WayBus object is the 'A' of the pair, then map the FOM to its mOutData, else its mInData.
   def initDirection( self, isBusA ):
      if ( isBusA ):
          self.direction = '.mOutData'
      else:
          self.direction = '.mInData'

#################################
# B to A HLA Object Configuration
#################################
class FluidDistIfBToAObjectConfig(FluidDistIfObjectConfig):
   # If the FluidDistributed2WayBus object is the 'A' of the pair, then map the FOM to its mInData, else its mOutData.
   def initDirection( self, isBusA ):
      if ( isBusA ):
          self.direction = '.mInData'
      else:
          self.direction = '.mOutData'
