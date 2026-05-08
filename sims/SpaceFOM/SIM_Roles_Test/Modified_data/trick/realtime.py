trick.real_time_enable()
#trick.itimer_enable()
trick.exec_set_enable_freeze(True)
trick.exec_set_freeze_command(True)

enable_sim_control_panel = True
if ( enable_sim_control_panel ):
   trick.var_allow_connections()
   trick.sim_control_panel_set_enabled( enable_sim_control_panel )
   trick.var_server_set_port( 7000 )

trick.exec_set_software_frame( 0.250 )
trick.exec_set_freeze_frame( 0.250 )
