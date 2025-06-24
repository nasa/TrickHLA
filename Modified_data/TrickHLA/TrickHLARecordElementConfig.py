##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level TrickHLA input
#     file interface for specifying fixed records and fixed record elements.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA/ER6) (June 2025) (--) (Fixed record support.)))
##############################################################################
import trick


class TrickHLARecordElementConfig( object ):

   rti_encoding = trick.TrickHLA.ENCODING_UNKNOWN
   trick_name   = None

   # List of Record Elements.
   elements = None

   def __init__( self,
                 rti_encoding,
                 trick_name = None ):
      
      self.rti_encoding = rti_encoding
      self.trick_name   = trick_name

      return


   def add_record_element( self, element ):

      if ( self.elements == None ):
         self.elements = []

      self.elements.append( element )

      return


   def initialize_fixed_record( self, thla_elem, input_elem ):

      if ( input_elem.rti_encoding == trick.TrickHLA.ENCODING_FIXED_RECORD ):
         thla_elem.rti_encoding = trick.TrickHLA.ENCODING_FIXED_RECORD

         # Allocate the fixed record elements.
         thla_elem.element_count = len( input_elem.elements )
         if ( thla_elem.element_count > 0 ):
            thla_elem.elements = trick.TMM_declare_var_1d( 'TrickHLA::RecordElement',
                                                           thla_elem.element_count )

         # Loop through the fixed record elements and initialize them.
         for indx in range( 0, thla_elem.element_count ):
            if ( input_elem.elements[indx].rti_encoding == trick.TrickHLA.ENCODING_FIXED_RECORD ):
               self.initialize_fixed_record( thla_elem.elements[indx],
                                             input_elem.elements[indx] )
            else:
               thla_elem.elements[indx].rti_encoding = input_elem.elements[indx].rti_encoding

               if ( input_elem.elements[indx].trick_name != None ):
                  thla_elem.elements[indx].trick_name = str( input_elem.elements[indx].trick_name )
               else:
                  thla_elem.elements[indx].trick_name = str( '' )

      return
