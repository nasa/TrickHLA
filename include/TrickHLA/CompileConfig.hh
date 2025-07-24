/*!
@file TrickHLA/CompileConfig.hh
@ingroup TrickHLA
@brief TrickHLA compile time configuration settings.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLA}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, February 2009, --, Consolidated config settings.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_COMPILE_CONFIG_HH
#define TRICKHLA_COMPILE_CONFIG_HH

// Trick does not support the std::wstring type in the Memory Manager for now.
// Default: NO_TRICK_WSTRING_MM_SUPPORT
#define NO_TRICK_WSTRING_MM_SUPPORT

// For blocking cyclic reads, do a check on the send and received data counts.
// Default: NO_THLA_CHECK_SEND_AND_RECEIVE_COUNTS
#define NO_THLA_CHECK_SEND_AND_RECEIVE_COUNTS

// Measure the elapsed time between cyclic data reads.
// Default: NO_THLA_CYCLIC_READ_TIME_STATS
#define NO_THLA_CYCLIC_READ_TIME_STATS

// Insert a compile time error if an unsupported version of Trick 19 is used.
// Minimum supported Trick 19 version: 19.0.0
#define MIN_TRICK_VER 19  // Set to the minimum supported Trick Major version.
#define MIN_TRICK_MINOR 0 // Set to the minimum supported Trick Minor version.
#define MIN_TRICK_PATCH 0 // Set to the minimum supported Trick Patch version.
// Make sure the TRICK_VER compiler define is set.
#if ( !defined( TRICK_VER ) )
#   error The Trick version variable TRICK_VER is not set!
#   error The minimum acceptable version of Trick is 19.0.0!
#endif
// Check for the minimum major version of Trick.
#if ( TRICK_VER < MIN_TRICK_VER )
#   error The minimum acceptable version of Trick is 19.0.0!
#else // Check the minor version if set.
#   if ( TRICK_VER == MIN_TRICK_VER )
#      if ( defined( TRICK_MINOR ) && ( TRICK_MINOR < MIN_TRICK_MINOR ) )
#         error The minimum acceptable version of Trick is 19.0.0!
#      else // Check the patch version if set.
#         if ( TRICK_MINOR == MIN_TRICK_MINOR ) && ( defined( TRICK_PATCH ) && ( TRICK_PATCH < MIN_TRICK_PATCH ) )
#            error The minimum acceptable version of Trick is 19.0.0!
#         endif
#      endif
#   endif
#endif
#undef MIN_TRICK_PATCH
#undef MIN_TRICK_MINOR
#undef MIN_TRICK_VER

// The Pitch Runtime Infrastructure (RTI) API calls will cause the
// floating-point control word register to change the Mantissa Precision Control
// Bits from 64-bit (extended double precision) to 53-bit (double precision)
// because the Pitch RTI runs Java under the hood which causes the problem.
// This is only supported for Intel CPU's.
#if ( !defined( FPU_CW_PROTECTION ) && ( defined( __i386__ ) || defined( __x86_64__ ) ) )
#   define FPU_CW_PROTECTION
#endif

// Define TRICKHLA_ENABLE_FPU_CONTROL_WORD_VALIDATION to enabled FPU control
// word tests that have been sprinkled throughout the TrickHLA code. This only
// has an effect if FPU_CW_PROTECTION is defined as well.
// Default: NO_TRICKHLA_ENABLE_FPU_CONTROL_WORD_VALIDATION
#define NO_TRICKHLA_ENABLE_FPU_CONTROL_WORD_VALIDATION

#endif // TRICKHLA_COMPILE_CONFIG_HH
