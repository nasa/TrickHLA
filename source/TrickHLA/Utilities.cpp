/*!
@file TrickHLA/Utilities.cpp
@ingroup TrickHLA
@brief Implementation of the TrickHLA utilities.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, TrickHLA, Aug 2006, --, DSES TrickHLA Utilities.}
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, Sept 2010, --, Added Mac FPU control word support.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <string>
#include <time.h>

// Trick include files.
#include "trick/trick_byteswap.h"

// TrickHLA include files.
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/Version.hh"

// For a Mac, add support for the FPU control word value at program start.
#if defined( FPU_CW_PROTECTION ) && defined( __APPLE__ ) && ( defined( __i386__ ) || defined( __x86_64__ ) )
fpu_control_t __fpu_control;
#endif

using namespace std;
using namespace TrickHLA;

bool Utilities::is_transmission_byteswap(
   EncodingEnum const rti_encoding )
{
   char const endianness = Utilities::get_endianness();

   // Check encoding versus Endianness to determine if we need to byteswap.
   return ( ( ( rti_encoding == ENCODING_BIG_ENDIAN ) && ( endianness == TRICK_LITTLE_ENDIAN ) )
            || ( ( rti_encoding == ENCODING_LITTLE_ENDIAN ) && ( endianness == TRICK_BIG_ENDIAN ) ) );
}

short Utilities::byteswap_short(
   short const input )
{
   short                output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // sizeof(char) <= sizeof(short) <= sizeof(int)
   switch ( size ) {
      case 2: {
         out[0] = in[1];
         out[1] = in[0];
         break;
      }
      case 4: {
         out[0] = in[3];
         out[1] = in[2];
         out[2] = in[1];
         out[3] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

unsigned short Utilities::byteswap_unsigned_short(
   unsigned short const input )
{
   unsigned short       output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // sizeof(char) <= sizeof(short) <= sizeof(int)
   switch ( size ) {
      case 2: {
         out[0] = in[1];
         out[1] = in[0];
         break;
      }
      case 4: {
         out[0] = in[3];
         out[1] = in[2];
         out[2] = in[1];
         out[3] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

int Utilities::byteswap_int( // RETURN: -- Byteswap value.
   int const input )         // IN: -- The input value to byteswap.
{
   int                  output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // sizeof(short) <= sizeof(int) <= sizeof(long)
   switch ( size ) {
      case 4: {
         out[0] = in[3];
         out[1] = in[2];
         out[2] = in[1];
         out[3] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

unsigned int Utilities::byteswap_unsigned_int(
   unsigned int const input )
{
   unsigned int         output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // sizeof(short) <= sizeof(int) <= sizeof(long)
   switch ( size ) {
      case 4: {
         out[0] = in[3];
         out[1] = in[2];
         out[2] = in[1];
         out[3] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

long Utilities::byteswap_long(
   long const input )
{
   long                 output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // sizeof(long) >= sizeof(int)
   switch ( size ) {
      case 4: {
         out[0] = in[3];
         out[1] = in[2];
         out[2] = in[1];
         out[3] = in[0];
         break;
      }
      case 8: {
         out[0] = in[7];
         out[1] = in[6];
         out[2] = in[5];
         out[3] = in[4];
         out[4] = in[3];
         out[5] = in[2];
         out[6] = in[1];
         out[7] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

unsigned long Utilities::byteswap_unsigned_long(
   unsigned long const input )
{
   unsigned long        output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // sizeof(long) >= sizeof(int)
   switch ( size ) {
      case 4: {
         out[0] = in[3];
         out[1] = in[2];
         out[2] = in[1];
         out[3] = in[0];
         break;
      }
      case 8: {
         out[0] = in[7];
         out[1] = in[6];
         out[2] = in[5];
         out[3] = in[4];
         out[4] = in[3];
         out[5] = in[2];
         out[6] = in[1];
         out[7] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

long long Utilities::byteswap_long_long(
   long long const input )
{
   long long            output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // Specified in the C99 standard, a long long is at least 64 bits.
   // sizeof(long long) >= sizeof(long)
   switch ( size ) {
      case 8: {
         out[0] = in[7];
         out[1] = in[6];
         out[2] = in[5];
         out[3] = in[4];
         out[4] = in[3];
         out[5] = in[2];
         out[6] = in[1];
         out[7] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

unsigned long long Utilities::byteswap_unsigned_long_long(
   unsigned long long const input )
{
   unsigned long long   output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   // Specified in the C99 standard, a long long is at least 64 bits.
   // sizeof(long long) >= sizeof(long)
   switch ( size ) {
      case 8: {
         out[0] = in[7];
         out[1] = in[6];
         out[2] = in[5];
         out[3] = in[4];
         out[4] = in[3];
         out[5] = in[2];
         out[6] = in[1];
         out[7] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

float Utilities::byteswap_float(
   float const input )
{
   float                output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   switch ( size ) {
      case 4: {
         out[0] = in[3];
         out[1] = in[2];
         out[2] = in[1];
         out[3] = in[0];
         break;
      }
      case 8: {
         out[0] = in[7];
         out[1] = in[6];
         out[2] = in[5];
         out[3] = in[4];
         out[4] = in[3];
         out[5] = in[2];
         out[6] = in[1];
         out[7] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

double Utilities::byteswap_double(
   double const input )
{
   double               output;
   size_t const         size = sizeof( output );
   unsigned char       *out  = reinterpret_cast< unsigned char * >( &output );
   unsigned char const *in   = reinterpret_cast< unsigned char const * >( &input );

   switch ( size ) {
      case 8: {
         out[0] = in[7];
         out[1] = in[6];
         out[2] = in[5];
         out[3] = in[4];
         out[4] = in[3];
         out[5] = in[2];
         out[6] = in[1];
         out[7] = in[0];
         break;
      }
      default: {
         size_t i, k;
         for ( i = 0, k = size - 1; i < size; ++i, --k ) {
            out[i] = in[k];
         }
         break;
      }
   }
   return ( output );
}

size_t const Utilities::next_positive_multiple_of_8(
   size_t const value )
{
   // Round up to the next positive multiple of 8.
   return Utilities::next_positive_multiple_of_N( value, 8 );
}

size_t const Utilities::next_positive_multiple_of_N(
   size_t const       value,
   unsigned int const n )
{
   // Round up to the next positive multiple of N where the minimum result is N.
   // The result will always be greater than the positive value specified by
   // up to a count of N.
   return ( ( value >= n ) ? ( n * ( ( value / n ) + 1 ) ) : n );
}

int Utilities::micro_sleep(
   long const usec )
{
   struct timespec sleep_time;
   if ( usec >= 1000000 ) {
      sleep_time.tv_sec  = ( usec / 1000000 );
      sleep_time.tv_nsec = ( usec - ( sleep_time.tv_sec * 1000000 ) ) * 1000;
   } else if ( usec > 0 ) {
      sleep_time.tv_sec  = 0;
      sleep_time.tv_nsec = usec * 1000;
   } else {
      sleep_time.tv_sec  = 0;
      sleep_time.tv_nsec = 0;
   }
   return nanosleep( &sleep_time, NULL );
}

string const Utilities::get_version()
{
   // Version of the form: "vMajor.Minor.Patch" => v1.2.3
   return "v" + std::to_string( TRICKHLA_MAJOR_VERSION )
          + "." + std::to_string( TRICKHLA_MINOR_VERSION )
          + "." + std::to_string( TRICKHLA_PATCH_VERSION );
}

string const &Utilities::get_release_date()
{
   return TRICKHLA_RELEASE_DATE;
}
