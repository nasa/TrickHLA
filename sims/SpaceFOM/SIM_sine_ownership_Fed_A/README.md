# Ownership transfers at initialization with split object attribute ownership and three or more federates.

This example simulation shows how to do ownership transfers at initialization and
how to perform multiphase initialization with mixed ownership because a federate
could see two or more data reflections.

When ownership of object attributes is mixed between several federates, a
subscribing federate could see multiple data reflections during initialization.
We will show how to layout multiphase initialization to support ownership transfer
at initialization and how to coordinate initialization data exchanges.

Run the following simulations to run the Ownership Transfers at Initialization
with split ownership multiphase initialization data updates.
1. cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine_ownership_Fed_A
2. ./S_main* RUN_a_side_mpr/input.py --verbose on
3. cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine_ownership_Fed_P
4. ./S_main* RUN_p_side/input.py --verbose on
5. cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine_ownership_Fed_other
6. ./S_main* RUN_other/input.py --verbose on

Multiphase Initialization Steps<br>
|Federate "A"                 |Federate "P"                 |Federate "Other"             |
|:---------------------------:|:---------------------------:|:---------------------------:|
|handle_pulled_ownership_at_init:<br>A-side-Federate.Sine|pull_ownership_at_init:<br>A-side-Federate.Sine:<br>Attributes: "Name, Tolerance"| Do-nothing |
|wait_for_init_sync_point:<br>"Ownership_transfer_init_phase"|wait_for_init_sync_point:<br>"Ownership_transfer_init_phase"|wait_for_init_sync_point:<br>"Ownership_transfer_init_phase"|
|receive_init_data:<br>A-side-Federate.Sine|send_init_data:<br>A-side-Federate.Sine|receive_init_data:<br>A-side-Federate.Sine|
|wait_for_init_sync_point:<br>"Analytic_init_phase1"|wait_for_init_sync_point:<br>"Analytic_init_phase1"|wait_for_init_sync_point:<br>"Analytic_init_phase1"|
|send_init_data:<br>A-side-Federate.Sine|receive_init_data:<br>A-side-Federate.Sine|receive_init_data:<br>A-side-Federate.Sine|
|wait_for_init_sync_point:<br>"Analytic_init_phase2"|wait_for_init_sync_point:<br>"Analytic_init_phase2"|wait_for_init_sync_point:<br>"Analytic_init_phase2"|
|receive_init_data:<br>P-side-Federate.Sine|send_init_data:<br>P-side-Federate.Sine|receive_init_data:<br>P-side-Federate.Sine|
|wait_for_init_sync_point:<br>"Propagated_init_phase"|wait_for_init_sync_point:<br>"Propagated_init_phase"|wait_for_init_sync_point:<br>"Propagated_init_phase"|
