# SIM_sine_threads

SIM_sine_threads is a simulation that solves a sine wave using both
analytic and propagated solution. The distributed simulation is comprised
of two simulations. The A-side federate owns the analytic sine state and
the P-side federate owns the propagated sine state. The example shows how
to use and configure Trick child threads and associating them with TrickHLA.

---
### Configuring the Simulation

#### S_define configuration

Ensure to include the THLAThread simulation module in the S_define file:

```
// TrickHLA support for Trick Child Threads.
#include "THLAThread.sm"
```

There are two approaches for associating Trick child threads to TrickHLA
in the S_define file.

The first approach adds an THLAThread sim-object instance to associate 
the Trick child thread (i.e. 1) used by the 'A' sim-object to TrickHLA 
with the specified HLA data cycle time. Please note for the for this code 
example, the child thread ID is hard coded and you must manually ensure 
the ID's match between the simulation object and the TrickHLA thread 
association. This is shown for the AnalyticeSineSimeObj.

```
class AnalyticSineSimObj : public Trick::SimObject {
 public:
   AnalyticSineSimObj()
   {
      ...
      C1 (DYN_RATE, "scheduled") truth_data.compute_value( THLA.execution_control.get_scenario_time() );
      C1 (DYN_RATE, "scheduled") truth_data.compute_derivative( THLA.execution_control.get_scenario_time() );

      C1 (DYN_RATE, "scheduled") sim_data.compute_value( THLA.execution_control.get_scenario_time() );
      C1 (DYN_RATE, "scheduled") sim_data.compute_derivative( THLA.execution_control.get_scenario_time() );

      C1 (THLA_DATA_CYCLE_TIME, "scheduled") sim_data.adjust_phase( THLA.execution_control.get_scenario_time() );
   }
};

AnalyticSineSimObj A;

// Example 1 showing a THLAThread sim-object created to associate the Trick child
// thread (i.e. 1) used by the 'A' sim-object to TrickHLA with the specified
// HLA data cycle time.
THLAThreadSimObject THLAThread1( THLA.federate,
                                 THLA_DATA_CYCLE_TIME, // Main thread data cycle time.
                                 THLA_DATA_CYCLE_TIME, // Child thread data cycle time.
                                 1 );                  // Trick child thread 1 (i.e. C1).
```

The second approach is to have the simulation object extend the 
TrickHLAThread sim-object to associate a Trick child thread (i.e. 2) to 
TrickHLA with the specified HLA data cycle time. The advantage of the 
code shown is the child thread ID and TrickHLA thread association are 
based on the same variable setting. This is shown for the 
PropagatedSineSimObj.

```
class PropagatedSineSimObj : public THLAThreadSimObject {
 public:
   PropagatedSineSimObj( TrickHLA::Federate & thla_fed,
                         double               main_thread_data_cycle,
                         double               child_thread_data_cycle,
                         unsigned short       _THREAD_ID )
      : THLAThreadSimObject( thla_fed,
                             main_thread_data_cycle,
                             child_thread_data_cycle,
                             _THREAD_ID )
   {
      ...
      C_THREAD_ID (DYN_RATE, "scheduled") truth_data.compute_value( THLA.execution_control.get_scenario_time() );
      C_THREAD_ID (DYN_RATE, "scheduled") truth_data.compute_derivative( THLA.execution_control.get_scenario_time() );

      C_THREAD_ID ("derivative") sim_data.compute_derivative( get_integ_time() );
      C_THREAD_ID ("integration") trick_ret = sim_data.integration();

      C_THREAD_ID ("post_integration") sim_data.set_time( THLA.execution_control.get_scenario_time() );
   }
};

...

// Example 2 showing a 'P' sim-object that extends the TrickHLAThread sim-object
// to associate a Trick child thread (i.e. 2) to TrickHLA with the specified
// HLA data cycle time.
PropagatedSineSimObj P( THLA.federate,
                        THLA_DATA_CYCLE_TIME, // Main thread data cycle time.
                        THLA_DATA_CYCLE_TIME, // Child thread data cycle time.
                        2 );                  // Trick child thread 2 (i.e. C2)
```

#### A-side federate threads input file configuration

**RUN_a_side/input.py:**

Configure the Trick child threads associated to TrickHLA to be Asynchronous Must Finish (AMF) and set the cycle time.

```
trick.exec_set_thread_process_type( 1, trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 1, 0.250 )

trick.exec_set_thread_process_type( 2, trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 2, 0.250 )
```

Associate the Trick child thread IDs with this HLA object instance, which
the thread_ids are a comma separated list if Trick child threads 
associated with the use of the data from the HLA object instance.

```
THLA.manager.objects[0].FOM_name            = 'Test'
THLA.manager.objects[0].name                = 'A-side-Federate.Sine'
THLA.manager.objects[0].create_HLA_instance = True
THLA.manager.objects[0].thread_ids          = "1"

THLA.manager.objects[1].FOM_name            = 'Test'
THLA.manager.objects[1].name                = 'P-side-Federate.Sine'
THLA.manager.objects[1].create_HLA_instance = False
THLA.manager.objects[1].thread_ids          = "2"
```

#### P-side federate threads input file configuration
**RUN_p_side/input.py:**

Configure the Trick child threads associated to TrickHLA to be Asynchronous Must Finish (AMF) and set the cycle time.

```
trick.exec_set_thread_process_type( 1, trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 1, 0.250 )

trick.exec_set_thread_process_type( 2, trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 2, 0.250 )
```

Associate the Trick child thread IDs with this HLA object instance, which
the thread_ids are a comma separated list if Trick child threads 
associated with the use of the data from the HLA object instance.

```
THLA.manager.objects[0].FOM_name            = 'Test'
THLA.manager.objects[0].name                = 'A-side-Federate.Sine'
THLA.manager.objects[0].create_HLA_instance = False
THLA.manager.objects[0].thread_ids          = "1"

THLA.manager.objects[1].FOM_name            = 'Test'
THLA.manager.objects[1].name                = 'P-side-Federate.Sine'
THLA.manager.objects[1].create_HLA_instance = True
THLA.manager.objects[1].thread_ids          = "2"
```

---
### Building the Simulation
In the SIM_sine_threads directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine_threads directory:

```
./S_main_*.exe RUN_a_side/input.py
```

From another terminal, in the SIM_sine_threads directory:

```
./S_main_*.exe RUN_p_side/input.py
```
