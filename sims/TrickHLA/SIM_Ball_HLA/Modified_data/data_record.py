
''' Ensure variable is defined '''
try:
    DR_GROUP_ID
except NameError:
    def initialize_globals():
        global DR_GROUP_ID  # Create global variable when not defined
        global drg          # Create a data recording group variable
        DR_GROUP_ID = 0
        drg = []            # Initialize variable as an empty array
    initialize_globals()

"""
Default Data Recording Template.
"""
def add_dr_group( ball_sim_obj_name,
                  ball_group_name,
                  ball_dr_cycle     = 0.1 ):
    global DR_GROUP_ID
    global drg
    group_name = ball_group_name
    drg.append( trick.DRAscii(group_name) )
    drg[DR_GROUP_ID].set_freq(trick.sim_services.DR_Always)
    drg[DR_GROUP_ID].enable()
    drg[DR_GROUP_ID].set_cycle(0.1)

    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.position[0]' )
    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.position[1]' )
    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.velocity[0]' )
    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.velocity[1]' )
    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.acceleration[0]' )
    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.acceleration[1]' )
    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.external_force[0]' )
    drg[DR_GROUP_ID].add_variable( ball_sim_obj_name + '.state.output.external_force[1]' )

    trick_data_record.drd.add_group( drg[DR_GROUP_ID], trick.DR_Buffer )
    
    # Add 1 to DR_GROUP_ID, this sets DR_GROUP_ID up for the next Data Recording file.
    DR_GROUP_ID += 1

