# SIM_sine_ownership_init

SIM_sine_ownership_init is a simulation that solves a sine wave
using both analytic and propagated solution. The distributed simulation 
is comprised of two simulations. The A-side federate owns the analytic
sine state and the P-side federate owns the propagated sine state.
The example shows how to perform an ownership transfer during initialization.

---
### Configuring the Simulation

#### S_define configuration

For this example, the P-side federate will pull ownership of the Name
attribute of the A-side-Federate.Sine object instance during initialization.
Ownership transfers during initialization requires coordination to ensure
the transfer completes. For every push or pull request there needs to be a
corresponding handle_pushed_ownership_at_init or
handle_pulled_ownership_at_init by the other federate. The list of attributes
in the push or pull request is a comma separated list so you can specify more
than one.


```
class THLAInitAnalyticSimObject : public Trick::SimObject {
 public:
   THLAInitAnalyticSimObject( TrickHLA::Manager  & thla_mngr,
                              TrickHLA::Federate & thla_fed,
                              unsigned int         _MPI_1   = 100,
                              unsigned int         _MPI_END = P_SF_INIT_POST_MPI )
      : sim_timeline(),
        scenario_timeline( sim_timeline, 0.0, 0.0 ),
        thla_manager( thla_mngr ),
        thla_federate( thla_fed )
   {
      // Ownership transfer at initialization.
      P_MPI_1 ("initialization") thla_manager.handle_pulled_ownership_at_init( "A-side-Federate.Sine" );
      
      ...
   }
};

class THLAInitPropagatedSimObject : public Trick::SimObject {
 public:
   THLAInitPropagatedSimObject( TrickHLA::Manager  & thla_mngr,
                                TrickHLA::Federate & thla_fed,
                                unsigned int         _MPI_1   = 100,
                                unsigned int         _MPI_END = P_SF_INIT_POST_MPI )
      : scenario_timeline( sim_timeline, 0.0, 0.0 ),
        thla_manager( thla_mngr ),
        thla_federate( thla_fed )
   {
      // Ownership transfer at initialization.
      P_MPI_1 ("initialization") thla_manager.pull_ownership_at_init( "A-side-Federate.Sine", "Name" );

      ...
   }
};
```

---
### Building the Simulation
In the SIM_sine_ownership_init directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine_ownership_init directory:

```
./S_main_*.exe RUN_a_side_mpr/input.py --verbose on
```

From a another console, in the SIM_sine_ownership_init directory:

```
./S_main_*.exe RUN_p_side/input.py --verbose on
```
