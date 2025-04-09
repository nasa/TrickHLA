/*!
@file SpaceFOM/QuaternionData.cpp
@ingroup SpaceFOM
@brief Simple functions that operate on quaternion data.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{QuaternionData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, October 2023, --, Initial version }
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Made into full class.}
@revs_end

*/

// C includes.
#include <math.h>

// Trick includes.
#include "trick/constant.h"
#include "trick/trick_math.h"

// SpaceFOM includes.
#include "SpaceFOM/QuaternionData.hh"

using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
QuaternionData::QuaternionData()
{
   initialize();
}

/*!
 * @job_class{initialization}
 */
QuaternionData::QuaternionData( QuaternionData const &source )
{
   copy( source );
}

/*!
 * @job_class{initialization}
 */
QuaternionData::QuaternionData(
   Euler_Seq    sequence,
   double const angles[3] )
{
   set_from_Euler( sequence, angles );
}

/*!
 * @job_class{initialization}
 */
QuaternionData::QuaternionData(
   double const T[3][3] )
{
   set_from_transfrom( T );
}

/***********************************************************************
 * QuaternionData methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
QuaternionData &QuaternionData::operator=(
   QuaternionData const &rhs )
{
   this->scalar    = rhs.scalar;
   this->vector[0] = rhs.vector[0];
   this->vector[1] = rhs.vector[1];
   this->vector[2] = rhs.vector[2];
   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
bool QuaternionData::operator==(
   QuaternionData const &rhs )
{
   return ( is_equal( rhs ) );
}

/*!
 * @job_class{scheduled}
 */
bool QuaternionData::operator!=(
   QuaternionData const &rhs )
{
   return ( !( is_equal( rhs ) ) );
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::print_data( std::ostream &stream ) const
{
   double euler_angles[3];

   // Compute the attitude Euler angles.
   get_Euler_deg( Roll_Pitch_Yaw, euler_angles );

   // Set the print precision.
   stream.precision( 15 );

   stream << "\tattitude (s,v): "
          << "\t\t" << scalar << "; "
          << "\t\t" << vector[0] << ", "
          << "\t\t" << vector[1] << ", "
          << "\t\t" << vector[2] << '\n';
   stream << "\tattitude (RPY){deg}: "
          << "\t\t" << euler_angles[0] << ", "
          << "\t\t" << euler_angles[1] << ", "
          << "\t\t" << euler_angles[2] << '\n';

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::initialize()
{
   // Initialize to a unit quaternion with no rotation.
   this->scalar    = 1.0;
   this->vector[0] = 0.0;
   this->vector[1] = 0.0;
   this->vector[2] = 0.0;
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::set_from_Euler(
   Euler_Seq    sequence,
   double const angles[3] )
{
   euler_quat( const_cast< double * >( angles ), &( this->scalar ), 0, sequence );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::set_from_Euler_deg(
   Euler_Seq    sequence,
   double const angles_deg[3] )
{
   double angles[3];
   angles[0] = angles_deg[0] * DTR;
   angles[1] = angles_deg[1] * DTR;
   angles[2] = angles_deg[2] * DTR;
   euler_quat( angles, &( this->scalar ), 0, sequence );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::get_Euler(
   Euler_Seq sequence,
   double    angles[3] ) const
{
   double work[4];
   work[0] = this->scalar;
   work[1] = this->vector[0];
   work[2] = this->vector[1];
   work[3] = this->vector[2];
   euler_quat( angles, work, 1, sequence );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::get_Euler_deg(
   Euler_Seq sequence,
   double    angles_deg[3] ) const
{
   double angles[3];
   double work[4];
   work[0] = this->scalar;
   work[1] = this->vector[0];
   work[2] = this->vector[1];
   work[3] = this->vector[2];
   euler_quat( angles, work, 1, sequence );
   angles_deg[0] = angles[0] * RTD;
   angles_deg[1] = angles[1] * RTD;
   angles_deg[2] = angles[2] * RTD;
   return;
}

/*!
 * @job_class{initialization}
 */
void QuaternionData::set_from_transfrom(
   double const T[3][3] )
{
   // NOTE: This code is from JEOD Quaternion::left_quat_from_transformation.

   /*-------------------------------------------------------------------------
    * Overview
    * The goal is to find a unitary quaternion 'q' such that 'q' yields the
    * same vector transformations as does the transformation matrix 'T'. The
    * quaternion is unique save for an ambiguity in the sign of all the
    * elements. The sign will be chosen such that the scalar part of the
    * quaternion is non-negative.
    * There are multiple methods to deriving the quaternion based on the
    * matrix elements.  However, the base algorithm is susceptible to numerical
    * inaccuracies.  To avoid precision loss, the forms best suited to the
    * given matrix will be selected.  Method -1 is selected if the trace
    * dominates the diagonal elements of the matrix. Method 0 is selected if
    * the T_00 element dominates the trace and the other two elements. Methods
    * 1 and 2 are selected when T_11 or T_22 dominates.
    *------------------------------------------------------------------------*/

   // Method identification parameters.
   double tr;     /* Trace of input transformation matrix. */
   double tr_max; /* Max of tr, diagonal elements */
   int    method; /* Index of tr_max in tr (-1 if trace dominates) */

   // Working parameters.
   double qix2;     /* sqrt(1+max(tr,t_i)) */
   double qix4_inv; /* 1/(4 * qs) */
   int    ii;

   /* Compute the trace of the matrix. */
   tr = T[0][0] + T[1][1] + T[2][2];

   /* Find the largest of the trace (method = -1) and the three diagonal
    * elements of 'T' (method = 0, 1, or 2). */
   method = -1;
   tr_max = tr;
   for ( ii = 0; ii < 3; ii++ ) {
      if ( T[ii][ii] > tr_max ) {
         method = ii;
         tr_max = T[ii][ii];
      }
   }

   /* Use method -1 when no diagonal element dominates the trace. */
   if ( method == -1 ) {
      qix2      = std::sqrt( 1.0 + tr );
      qix4_inv  = 0.5 / qix2;
      scalar    = 0.5 * qix2;
      vector[0] = qix4_inv * ( T[2][1] - T[1][2] );
      vector[1] = qix4_inv * ( T[0][2] - T[2][0] );
      vector[2] = qix4_inv * ( T[1][0] - T[0][1] );

   } /* Use method 0,1, or 2 based on the dominant diagonal element. */
   else {
      ii     = method;
      int jj = ( ii + 1 ) % 3;
      int kk = ( jj + 1 ) % 3;

      double di = T[kk][jj] - T[jj][kk]; /* T_kj - T_jk */
      qix2      = sqrt( 1.0 + T[ii][ii] - ( T[jj][jj] + T[kk][kk] ) );
      if ( di < 0.0 ) {
         qix2 = -qix2;
      }
      qix4_inv   = 0.5 / qix2;
      vector[ii] = 0.5 * qix2;
      vector[jj] = qix4_inv * ( T[ii][jj] + T[jj][ii] );
      vector[kk] = qix4_inv * ( T[ii][kk] + T[kk][ii] );
      scalar     = qix4_inv * di;
   }
   return;
}

/*!
 * @job_class{initialization}
 */
void QuaternionData::get_transfrom(
   double T[3][3] ) const
{
   double qsx2_2 = 2.0 * scalar * scalar;

   T[0][0] = qsx2_2 + ( 2.0 * vector[0] * vector[0] ) - 1.0;
   T[0][1] = 2.0 * ( ( vector[0] * vector[1] ) - ( scalar * vector[2] ) );
   T[0][2] = 2.0 * ( ( vector[0] * vector[2] ) + ( scalar * vector[1] ) );
   T[1][0] = 2.0 * ( ( vector[0] * vector[1] ) + ( scalar * vector[2] ) );
   T[1][1] = qsx2_2 + ( 2.0 * vector[1] * vector[1] ) - 1.0;
   T[1][2] = 2.0 * ( ( vector[1] * vector[2] ) - ( scalar * vector[0] ) );
   T[2][0] = 2.0 * ( ( vector[0] * vector[2] ) - ( scalar * vector[1] ) );
   T[2][1] = 2.0 * ( ( vector[1] * vector[2] ) + ( scalar * vector[0] ) );
   T[2][2] = qsx2_2 + ( 2.0 * vector[2] * vector[2] ) - 1.0;

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::scale( double factor )
{
   this->scalar *= factor;
   this->vector[0] *= factor;
   this->vector[1] *= factor;
   this->vector[2] *= factor;
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::copy( QuaternionData const &source )
{
   this->scalar    = source.scalar;
   this->vector[0] = source.vector[0];
   this->vector[1] = source.vector[1];
   this->vector[2] = source.vector[2];
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::conjugate()
{
   this->vector[0] = -this->vector[0];
   this->vector[1] = -this->vector[1];
   this->vector[2] = -this->vector[2];
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::conjugate(
   QuaternionData const &source )
{
   this->scalar    = source.scalar;
   this->vector[0] = -source.vector[0];
   this->vector[1] = -source.vector[1];
   this->vector[2] = -source.vector[2];
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::normalize()
{
   normalize( &scalar, vector );
   return;
}

/*!
 * @job_class{scheduled}
 */
bool QuaternionData::is_equal(
   QuaternionData const &source )
{
   return ( is_equal( *this, source ) );
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::multiply(
   QuaternionData const &left,
   QuaternionData const &right )
{
   multiply_sv( left.scalar, left.vector, right.scalar, right.vector, &( this->scalar ), this->vector );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::multiply(
   QuaternionData const &left,
   double                right[3] )
{
   left_multiply_v( left.scalar, left.vector, right, &( this->scalar ), this->vector );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::multiply(
   double                left[3],
   QuaternionData const &right )
{
   right_multiply_v( left, right.scalar, right.vector, &( this->scalar ), this->vector );
   return;
}

/*!
 * @job_class{derivative}
 */
void QuaternionData::derivative_first(
   QuaternionData const &quat,
   double const          omega[3] )
{
   compute_derivative( quat.scalar, quat.vector, omega, &( this->scalar ), this->vector );
   return;
}

/*!
 * @job_class{derivative}
 */
void QuaternionData::derivative_first(
   double const quat_scalar,
   double const quat_vector[3],
   double const omega[3] )
{
   compute_derivative( quat_scalar, quat_vector, omega, &( this->scalar ), this->vector );
   return;
}

/*!
 * @job_class{derivative}
 */
void QuaternionData::derivative_second(
   QuaternionData const &quat,
   double const          omega[3],
   double const          omega_dot[3] )
{
   compute_2nd_derivative( quat.scalar, quat.vector, omega, omega_dot, &( this->scalar ), this->vector );
   return;
}

/*!
 * @job_class{derivative}
 */
void QuaternionData::derivative_second(
   double const quat_scalar,
   double const quat_vector[3],
   double const omega[3],
   double const omega_dot[3] )
{
   compute_2nd_derivative( quat_scalar, quat_vector, omega, omega_dot, &( this->scalar ), this->vector );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::compute_omega(
   QuaternionData const &att_quat,
   double                omega[3] )
{
   compute_omega( this->scalar, this->vector, att_quat.scalar, att_quat.vector, omega );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::transform_vector(
   double const v_in[3],
   double       v_out[3] ) const
{
   double qv_cross_v[3];
   double qv_cross_qv_cross_v[3];
   double v_dot;

   v_dot = V_DOT( vector, v_in );
   V_CROSS( qv_cross_v, vector, v_in );
   V_CROSS( qv_cross_qv_cross_v, vector, qv_cross_v );

   v_out[0] = qv_cross_v[0] * 2.0;
   v_out[1] = qv_cross_v[1] * 2.0;
   v_out[2] = qv_cross_v[2] * 2.0;

   v_out[0] += v_in[0] * scalar;
   v_out[1] += v_in[1] * scalar;
   v_out[2] += v_in[2] * scalar;

   v_out[0] *= scalar;
   v_out[1] *= scalar;
   v_out[2] *= scalar;

   v_out[0] += vector[0] * v_dot;
   v_out[1] += vector[1] * v_dot;
   v_out[2] += vector[2] * v_dot;

   v_out[0] += qv_cross_qv_cross_v[0];
   v_out[1] += qv_cross_qv_cross_v[1];
   v_out[2] += qv_cross_qv_cross_v[2];

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::conjugate_transform_vector(
   double const v_in[3],
   double       v_out[3] ) const
{
   QuaternionData q_star( *this );
   q_star.conjugate();
   q_star.transform_vector( v_in, v_out );
   return;
}

/***********************************************************************
 * Static methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
void QuaternionData::multiply_sv(
   double const ls,
   double const lv[3],
   double const rs,
   double const rv[3],
   double      *ps,
   double       pv[3] )
{
   // We use a working area because we do not know if the product happens to
   // refer to either the left or right operands.
   double ws;
   double wv[3];

   // Quaternion multiplication:
   // p[s:v] = l[s:v] * r[s:v]
   // ps = (ls*rs) - |lv,rv|
   // pv = (lv X rv) + (ls * rv) + (lv * rs)

   // Compute the scalar component:
   // ws = (ls*rs) - |lv,rv|
   ws = ( ls * rs ) - ( ( lv[0] * rv[0] ) + ( lv[1] * rv[1] ) + ( lv[2] * rv[2] ) );

   // Compute the vector component:
   // wv = (lv X rv) + (ls * rv) + (lv * rs)
   wv[0] = ( ( lv[1] * rv[2] ) - ( lv[2] * rv[1] ) ) + ( ls * rv[0] ) + ( lv[0] * rs );
   wv[1] = ( ( lv[2] * rv[0] ) - ( lv[0] * rv[2] ) ) + ( ls * rv[1] ) + ( lv[1] * rs );
   wv[2] = ( ( lv[0] * rv[1] ) - ( lv[1] * rv[0] ) ) + ( ls * rv[2] ) + ( lv[2] * rs );

   // Place results in the product quaternion.
   *ps   = ws;
   pv[0] = wv[0];
   pv[1] = wv[1];
   pv[2] = wv[2];

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::left_multiply_v(
   double const ls,
   double const lv[3],
   double const rv[3],
   double      *ps,
   double       pv[3] )
{
   // We use a working area because we do not know if the product happens to
   // refer to either the left or right operands.
   double ws;
   double wv[3];

   // Quaternion multiplication:
   // p[s:v] = l[s:v] * r[0:v]
   // ps = - |lv,rv|
   // pv = (lv X rv) + (ls * rv)

   // Compute the scalar component:
   // ws = - |lv,rv|
   ws = -( ( lv[0] * rv[0] ) + ( lv[1] * rv[1] ) + ( lv[2] * rv[2] ) );

   // Compute the vector component:
   // wv = (lv X rv) + (ls * rv) + (lv * rs)
   wv[0] = ( ( lv[1] * rv[2] ) - ( lv[2] * rv[1] ) ) + ( ls * rv[0] );
   wv[1] = ( ( lv[2] * rv[0] ) - ( lv[0] * rv[2] ) ) + ( ls * rv[1] );
   wv[2] = ( ( lv[0] * rv[1] ) - ( lv[1] * rv[0] ) ) + ( ls * rv[2] );

   // Place results in the product quaternion.
   *ps   = ws;
   pv[0] = wv[0];
   pv[1] = wv[1];
   pv[2] = wv[2];

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::right_multiply_v(
   double const lv[3],
   double const rs,
   double const rv[3],
   double      *ps,
   double       pv[3] )
{
   // We use a working area because we do not know if the product happens to
   // refer to either the left or right operands.
   double ws;
   double wv[3];

   // Quaternion multiplication:
   // p[s:v] = l[0:v] * r[s:v]
   // ps = - |lv,rv|
   // pv = (lv X rv) + (ls * rv) + (lv * rs)

   // Compute the scalar component:
   // ws = (ls*rs) - |lv,rv|
   ws = -( ( lv[0] * rv[0] ) + ( lv[1] * rv[1] ) + ( lv[2] * rv[2] ) );

   // Compute the vector component:
   // wv = (lv X rv) + (ls * rv) + (lv * rs)
   wv[0] = ( ( lv[1] * rv[2] ) - ( lv[2] * rv[1] ) ) + ( lv[0] * rs );
   wv[1] = ( ( lv[2] * rv[0] ) - ( lv[0] * rv[2] ) ) + ( lv[1] * rs );
   wv[2] = ( ( lv[0] * rv[1] ) - ( lv[1] * rv[0] ) ) + ( lv[2] * rs );

   // Place results in the product quaternion.
   *ps   = ws;
   pv[0] = wv[0];
   pv[1] = wv[1];
   pv[2] = wv[2];

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::normalize(
   double *qs,
   double  qv[3] )
{
   double q_mag_sq;
   double diff1;
   double norm_fact;

   // Compute and compare the magnitude of the quaternion wrt. one.
   if ( fabs( *qs ) > GSL_SQRT_DBL_MIN ) {
      double qv_mag_sq = qv[0] * qv[0] + qv[1] * qv[1] + qv[2] * qv[2];
      q_mag_sq         = ( *qs * *qs ) + qv_mag_sq;
      diff1            = 1.0 - q_mag_sq;
   } else {
      q_mag_sq = 0.0;
      diff1    = 1.0;
   }

   // Compute the normalization factor, nominally 1/sqrt(qmagsq).
   // Computational shortcut: Approximate as 2/(1+qmagsq)
   // To second order, the error in the approximation is diff1^2/8.
   // The approximation is valid if this error is smaller than numerical
   // precision. A double IEEE floating point number has a 53 bit mantissa plus
   // an implied 1 to the left of the binary point. The validity limit is thus
   // sqrt(8*2^(-54)) = 2.107342e-08, to the accuracy of the approximation.
   if ( ( diff1 > -2.107342e-08 ) && ( diff1 < 2.107342e-08 ) ) {
      norm_fact = 2.0 / ( 1.0 + q_mag_sq );
   } else {
      norm_fact = 1.0 / sqrt( q_mag_sq );
   }

   // Scale the quaternion by the above normalization factor.
   *qs *= norm_fact;
   qv[0] *= norm_fact;
   qv[1] *= norm_fact;
   qv[2] *= norm_fact;

   // Return to calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
bool QuaternionData::is_equal(
   QuaternionData const &lhs,
   QuaternionData const &rhs )
{
   return ( is_equal( lhs.scalar, lhs.vector, rhs.scalar, rhs.vector ) );
}

/*!
 * @job_class{scheduled}
 */
bool QuaternionData::is_equal(
   double const lhs_scalar,
   double const lhs_vector[3],
   double const rhs_scalar,
   double const rhs_vector[3] )
{
   if ( ( lhs_scalar != rhs_scalar )
        || ( lhs_vector[0] != rhs_vector[0] )
        || ( lhs_vector[1] != rhs_vector[1] )
        || ( lhs_vector[2] != rhs_vector[2] ) ) {
      return ( false );
   }
   return ( true );
}

/*!
 * @job_class{derivative}
 */
void QuaternionData::compute_derivative(
   double const quat_scalar,
   double const quat_vector[3],
   double const omega[3],
   double      *qdot_scalar,
   double       qdot_vector[3] )
{

   *qdot_scalar   = ( ( quat_vector[0] * omega[0] ) + ( quat_vector[1] * omega[1] ) + ( quat_vector[2] * omega[2] ) ) * 0.5;
   qdot_vector[0] = ( ( -quat_scalar * omega[0] ) + ( -quat_vector[2] * omega[1] ) + ( quat_vector[1] * omega[2] ) ) * 0.5;
   qdot_vector[1] = ( ( quat_vector[2] * omega[0] ) + ( -quat_scalar * omega[1] ) + ( -quat_vector[0] * omega[2] ) ) * 0.5;
   qdot_vector[2] = ( ( -quat_vector[1] * omega[0] ) + ( quat_vector[0] * omega[1] ) + ( -quat_scalar * omega[2] ) ) * 0.5;

   // Return to calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::compute_derivative(
   QuaternionData const *q,
   double const          omega[3],
   QuaternionData       *q_dot )
{
   compute_derivative( q->scalar, q->vector, omega, &( q_dot->scalar ), q_dot->vector );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::compute_omega(
   double const quat_scalar,
   double const quat_vector[3],
   double const qdot_scalar,
   double const qdot_vector[3],
   double       omega[3] )
{
   double scalar;
   double vector[4];

   // Compute the quaternion angular rate vector.
   // Note: This is the solution for a left (conjugate) quaternion.
   multiply_conjugate( qdot_scalar,
                       qdot_vector,
                       quat_scalar,
                       quat_vector,
                       &scalar,
                       vector );

   // Compute the angluar velocity vector from the angular rate vector.
   // Note that physical rotation angle is -1/2 the the left quaternion
   // rotation angle.
   omega[0] = -2.0 * vector[0];
   omega[1] = -2.0 * vector[1];
   omega[2] = -2.0 * vector[2];

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::compute_omega(
   QuaternionData const *q,
   QuaternionData const *q_dot,
   double                omega[3] )
{
   compute_omega( q->scalar,
                  q->vector,
                  q_dot->scalar,
                  q_dot->vector,
                  omega );
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::compute_2nd_derivative(
   double const quat_scalar,
   double const quat_vector[3],
   double const omega[3],
   double const omega_dot[3],
   double      *qdotdot_scalar,
   double       qdotdot_vector[3] )
{
   double omega_mag_sq;
   double omega_quat_scalar;
   double omega_quat_vector[3];

   omega_mag_sq = ( omega[0] * omega[0] ) + ( omega[1] * omega[1] ) + ( omega[2] * omega[2] );

   // Compute the special omega/omega_dot quaternion.
   omega_quat_scalar    = -0.25 * omega_mag_sq;
   omega_quat_vector[0] = -0.5 * omega_dot[0];
   omega_quat_vector[1] = -0.5 * omega_dot[1];
   omega_quat_vector[2] = -0.5 * omega_dot[2];

   // Multiply the special omega/omega_dot quaternion by the attitude quaternion
   // to compute the second time derivative of the attitude quaternion.
   multiply_sv( omega_quat_scalar, omega_quat_vector,
                quat_scalar, quat_vector,
                qdotdot_scalar, qdotdot_vector );

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionData::compute_2nd_derivative(
   QuaternionData const *q,
   double const          omega[3],
   double const          omega_dot[3],
   QuaternionData       *q_dotdot )
{
   QuaternionData::compute_2nd_derivative( q->scalar, q->vector, omega, omega_dot, &( q_dotdot->scalar ), q_dotdot->vector );
   return;
}

/*!
 * @job_class{scheduled}
 * @brief Pre-multiply a quaternion by another's conjugate: prod = conj(lq) * rq.
 */
void QuaternionData::conjugate_multiply(
   double const lq_scalar,
   double const lq_vector[3],
   double const rq_scalar,
   double const rq_vector[3],
   double      *scalar,
   double       vector[3] )
{
   double qv_cross_qv[3];

   //
   // Compute the scalar part of the resulting quaternion.
   //
   // Finish the scalar computation.
   *scalar = ( rq_scalar * lq_scalar ) + ( V_DOT( lq_vector, rq_vector ) );

   //
   // Compute the vector part of the resulting quaternion.
   //
   // Start with the scaled value the right quaternion vector by the left
   // quaternion scalar.
   V_SCALE( vector, rq_vector, lq_scalar );
   // Increment the vector by the scaled value of the left quaternion by the
   // right quaternion scalar.
   VxS_SUB( vector, lq_vector, rq_scalar );
   // Decrement the vector by the cross product of the quaternion vectors.
   V_CROSS( qv_cross_qv, lq_vector, rq_vector );
   V_DECR( vector, qv_cross_qv );

   return;
}

/*!
 * @job_class{scheduled}
 * @brief Post-multiply a quaternion by another's conjugate: prod = lq * conj(rq).
 */
void QuaternionData::multiply_conjugate(
   double const lq_scalar,
   double const lq_vector[3],
   double const rq_scalar,
   double const rq_vector[3],
   double      *scalar,
   double       vector[3] )
{
   double qv_cross_qv[3];

   //
   // Compute the scalar part of the resulting quaternion.
   //
   // Finish the scalar computation.
   *scalar = ( rq_scalar * lq_scalar ) + ( V_DOT( lq_vector, rq_vector ) );

   //
   // Compute the vector part of the resulting quaternion.
   //
   // Start with the scaled value the right quaternion vector by the left
   // quaternion scalar.
   V_SCALE( vector, lq_vector, rq_scalar );
   // Increment the vector by the scaled value of the left quaternion by the
   // right quaternion scalar.
   VxS_SUB( vector, rq_vector, lq_scalar );
   // Decrement the vector by the cross product of the quaternion vectors.
   V_CROSS( qv_cross_qv, lq_vector, rq_vector );
   V_DECR( vector, qv_cross_qv );

   return;
}
