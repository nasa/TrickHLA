
def HLA_walls_config( THLA_object,
                      sim_obj_name,
                      obj_name,
                      obj_packing,
                      obj_create = True,
                      obj_publish = None,
                      obj_subscribe = None,
                      obj_owns = None ):
   
   # Perform some logical settings allowing for overrides.
   # If not overriding Publish, align with create flag.
   if ( obj_publish == None ):
      publish = obj_create
   else:
      publish = obj_publish
      
   # If not overriding Subscribe, align with negated create flag.
   if ( obj_subscribe == None ):
      subscribe = not obj_create
   else:
      subscribe = obj_subscribe
      
   # If not overriding Ownership, align with create flag.
   if ( obj_owns == None ):
      owns = obj_create
   else:
      owns = obj_owns
   
   # Configure the object this federate will create an HLA instance and
   # publish data for.
   THLA_object.FOM_name            = 'Walls'
   THLA_object.name                = obj_name
   THLA_object.create_HLA_instance = obj_create
   THLA_object.packing             = obj_packing
   THLA_object.lag_comp            = None
   THLA_object.lag_comp_type       = trick.LAG_COMPENSATION_NONE
   THLA_object.ownership           = None
   THLA_object.conditional         = None
   THLA_object.deleted             = None
   THLA_object.attr_count          = 4
   THLA_object.attributes          = trick.sim_services.alloc_type( THLA_object.attr_count, 'TrickHLA::Attribute' )
   
   THLA_object.attributes[0].FOM_name        = 'ceiling'
   THLA_object.attributes[0].trick_name      = sim_obj_name + '.packing.ceiling_y_pos'
   THLA_object.attributes[0].config          = trick.CONFIG_INITIALIZE
   THLA_object.attributes[0].publish         = publish
   THLA_object.attributes[0].subscribe       = subscribe
   THLA_object.attributes[0].locally_owned   = owns
   THLA_object.attributes[0].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

   THLA_object.attributes[1].FOM_name        = 'right'
   THLA_object.attributes[1].trick_name      = sim_obj_name + '.packing.right_wall_x_pos'
   THLA_object.attributes[1].config          = trick.CONFIG_INITIALIZE
   THLA_object.attributes[1].publish         = publish
   THLA_object.attributes[1].subscribe       = subscribe
   THLA_object.attributes[1].locally_owned   = owns
   THLA_object.attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   
   THLA_object.attributes[2].FOM_name        = 'left'
   THLA_object.attributes[2].trick_name      = sim_obj_name + '.packing.left_wall_x_pos'
   THLA_object.attributes[2].config          = trick.CONFIG_INITIALIZE
   THLA_object.attributes[2].publish         = publish
   THLA_object.attributes[2].subscribe       = subscribe
   THLA_object.attributes[2].locally_owned   = owns
   THLA_object.attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   
   THLA_object.attributes[3].FOM_name        = 'floor'
   THLA_object.attributes[3].trick_name      = sim_obj_name + '.packing.floor_y_pos'
   THLA_object.attributes[3].config          = trick.CONFIG_INITIALIZE
   THLA_object.attributes[3].publish         = publish
   THLA_object.attributes[3].subscribe       = subscribe
   THLA_object.attributes[3].locally_owned   = owns
   THLA_object.attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   