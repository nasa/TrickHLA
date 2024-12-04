##############################################################################
# PURPOSE:
#    (This is an input file python routine to setup the initial values of the
#     sine state.)
#
# REFERENCE:
#    (Trick 10 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA) (May 2013) (--) (Initial implementation.)))
##############################################################################

# Analytic sine data
A.sim_data.time  = 0.0
A.sim_data.value = 0.0
A.sim_data.dvdt  = 0.392699
A.sim_data.amp   = 2.0
A.sim_data.phase = 0.0
A.sim_data.freq  = 0.1963495

# Analytic sine truth state
A.truth_data.time  = 0.0
A.truth_data.value = 0.0
A.truth_data.dvdt  = 0.392699
A.truth_data.amp   = 2.0
A.truth_data.phase = 0.0
A.truth_data.freq  = 0.1963495

# Propagated sine data
P.sim_data.time  = 0.0
P.sim_data.value = 0.0
P.sim_data.dvdt  = 0.392699
P.sim_data.amp   = 1.0
P.sim_data.phase = 0.0
P.sim_data.freq  = 0.392699

# Propagated sine truth state
P.truth_data.time  = 0.0
P.truth_data.value = 0.0
P.truth_data.dvdt  = 0.392699
P.truth_data.amp   = 1.0
P.truth_data.phase = 0.0
P.truth_data.freq  = 0.392699
