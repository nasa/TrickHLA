# SIM_sine_zero_lookahead

SIM_sine_zero_lookahead is a simulation that solves a sine wave using 
both analytic and propagated solution. The distributed simulation is 
comprised of two simulations. The A-side federate owns the analytic sine 
state and the P-side federate owns the propagated sine state. The example 
shows how to use and configure TrickHLA to send data using Zero-
Lookahead.

Zero-Lookahead allows timestamp order data to be sent and received for 
the current granted HLA time. This does imply the moment when data is 
send and/or received is asynchronous and can happen any time within the 
granted HLA frame of time. To ensure a deterministic and repeatable 
simulation, TricHLA will use the zero-lookahead mechanism but will treat 
data exchanges as blocking I/O, but with timestamp ordered data.

---
### Configuring the Simulation

#### S_define configuration

You will need to place jobs in your simulation to explicitly send and 
receive the zero-lookahead data for a given object instance name. Because 
we use the same S_define file for both federates, is why you are seeing 
both the send and receive zero-lookahead jobs for both the Analytic and 
Propagated sine simulation objects.


```
class ZeroLookaheadAnalyticSineSimObj : public THLAFedSimObject {
 public:
   ZeroLookaheadAnalyticSineSimObj( TrickHLA::Federate & thla_fed )
      : THLAFedSimObject( thla_fed )
   {
      ...
      // HLA Data will only be received if it is remotely owned by another federate.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.wait_to_receive_zero_lookahead_data( "A-side-Federate.Sine.zero" );

      // Analytic sine calculations...

      // HLA Data will only be sent if this federate owns it.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.send_zero_lookahead_and_requested_data( "A-side-Federate.Sine.zero" );
   }
};
```

```
class ZeroLookaheadPropagatedSineSimObj : public THLAFedSimObject {
 public:
   ZeroLookaheadPropagatedSineSimObj( TrickHLA::Federate & thla_fed )
      : THLAFedSimObject( thla_fed )
   {
      ...
      // HLA Data will only be received if it is remotely owned by another federate.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.wait_to_receive_zero_lookahead_data( "P-side-Federate.Sine.zero" );

      // Propagated sine calculations...

      // HLA Data will only be sent if this federate owns it.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.send_zero_lookahead_and_requested_data( "P-side-Federate.Sine.zero" );
   }
};

```

#### A-side federate zero-lookahead input file configuration

**RUN_a_side/input.py:**

Configure all attributes of an object used for zero-lookahead data 
transfers as trick.CONFIG_ZERO_LOOKAHEAD.

```
THLA.manager.objects[0].FOM_name            = 'Test'
THLA.manager.objects[0].name                = 'A-side-Federate.Sine.zero'
THLA.manager.objects[1].create_HLA_instance = True
...
THLA.manager.objects[0].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[0].attributes[0].trick_name      = 'AZ.packing.time'
THLA.manager.objects[0].attributes[0].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[0].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[0].attributes[1].trick_name      = 'AZ.packing.value'
THLA.manager.objects[0].attributes[1].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[0].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[0].attributes[2].trick_name      = 'AZ.packing.dvdt'
THLA.manager.objects[0].attributes[2].config          = trick.CONFIG_ZERO_LOOKAHEAD
...

THLA.manager.objects[1].FOM_name            = 'Test'
THLA.manager.objects[1].name                = 'P-side-Federate.Sine.zero'
THLA.manager.objects[1].create_HLA_instance = False
...
THLA.manager.objects[1].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[1].attributes[0].trick_name      = 'PZ.packing.time'
THLA.manager.objects[1].attributes[0].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[1].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[1].attributes[1].trick_name      = 'PZ.packing.value'
THLA.manager.objects[1].attributes[1].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[1].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[1].attributes[2].trick_name      = 'PZ.packing.dvdt'
THLA.manager.objects[1].attributes[2].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
```

#### P-side federate zero-lookahead input file configuration
**RUN_p_side/input.py:**

Configure all attributes of an object used for zero-lookahead data 
transfers as trick.CONFIG_ZERO_LOOKAHEAD.

```
THLA.manager.objects[0].FOM_name            = 'Test'
THLA.manager.objects[0].name                = 'A-side-Federate.Sine.zero'
THLA.manager.objects[0].create_HLA_instance = False
...
THLA.manager.objects[0].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[0].attributes[0].trick_name      = 'AZ.packing.time'
THLA.manager.objects[0].attributes[0].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[0].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[0].attributes[1].trick_name      = 'AZ.packing.value'
THLA.manager.objects[0].attributes[1].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[0].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[0].attributes[2].trick_name      = 'AZ.packing.dvdt'
THLA.manager.objects[0].attributes[2].config          = trick.CONFIG_ZERO_LOOKAHEAD

...

THLA.manager.objects[1].FOM_name            = 'Test'
THLA.manager.objects[1].name                = 'P-side-Federate.Sine.zero'
THLA.manager.objects[1].create_HLA_instance = True
...
THLA.manager.objects[1].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[1].attributes[0].trick_name      = 'PZ.packing.time'
THLA.manager.objects[1].attributes[0].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[1].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[1].attributes[1].trick_name      = 'PZ.packing.value'
THLA.manager.objects[1].attributes[1].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
THLA.manager.objects[1].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[1].attributes[2].trick_name      = 'PZ.packing.dvdt'
THLA.manager.objects[1].attributes[2].config          = trick.CONFIG_ZERO_LOOKAHEAD
...
```

---
### Building the Simulation
In the SIM_sine_zero_lookahead directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine_zero_lookahead directory:

```
./S_main_*.exe RUN_a_side/input.py
```

From another terminal, in the SIM_sine_zero_lookahead directory:

```
./S_main_*.exe RUN_p_side/input.py
```
