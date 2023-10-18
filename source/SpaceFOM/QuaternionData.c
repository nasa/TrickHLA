/*!
@file SpaceFOM/QuaternionData.c
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
@trick_link_dependency{QuaternionData.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, October 2023, --, Initial version }
@revs_end

*/

// C includes.
#include "math.h"

// Trick includes.
#include "trick/trick_math.h"

// SpaceFOM includes.
#include "SpaceFOM/QuaternionData.h"


/*!
 * @job_class{scheduled}
 */
void normalize_quaternion (
   double * qs,
   double   qv[3])
{
   double q_mag_sq;
   double qv_mag_sq;
   double diff1;
   double norm_fact;

   // Compute and compare the magnitude of the quaternion wrt. one.
   if ( fabs (*qs) > GSL_SQRT_DBL_MIN ) {
      qv_mag_sq = qv[0] * qv[0] + qv[1] * qv[1] + qv[2] * qv[2];
      q_mag_sq = (*qs * *qs) + qv_mag_sq;
      diff1 = 1.0 - q_mag_sq;
   }
   else {
      q_mag_sq = 0.0;
      diff1 = 1.0;
   }

   // Compute the normalization factor, nominally 1/sqrt(qmagsq).
   // Computational shortcut: Approximate as 2/(1+qmagsq)
   // To second order, the error in the approximation is diff1^2/8.
   // The approximation is valid if this error is smaller than numerical
   // precision. A double IEEE floating point number has a 53 bit mantissa plus
   // an implied 1 to the left of the binary point. The validity limit is thus
   // sqrt(8*2^(-54)) = 2.107342e-08, to the accuracy of the appoximation.
   if ((diff1 > -2.107342e-08) && (diff1 < 2.107342e-08)) {
      norm_fact = 2.0 / (1.0 + q_mag_sq);
   } else {
      norm_fact = 1.0 / sqrt(q_mag_sq);
   }

   // Scale the quaternion by the above normalization factor.
   *qs *= norm_fact;
   for ( int iinc = 0; iinc < 4; ++iinc ) {
      qv[iinc] *= norm_fact;
   }

   // Return to calling routine.
   return;
}


/*!
 * @job_class{scheduled}
 */
void compute_quat_dot (
   double const   quat_scalar,
   double const   quat_vector[3],
   double const   omega[3],
   double       * qdot_scalar,
   double         qdot_vector[3] )
{
   double minus_half_omega[3];

   V_SCALE( minus_half_omega, omega, -0.5 );

   *(qdot_scalar) = - (V_DOT( quat_vector, minus_half_omega ));

   V_SCALE( qdot_vector, minus_half_omega, quat_scalar );
   qdot_vector[0] += (minus_half_omega[1] * quat_vector[2]) - (minus_half_omega[2] * quat_vector[1]);
   qdot_vector[1] += (minus_half_omega[2] * quat_vector[0]) - (minus_half_omega[0] * quat_vector[2]);
   qdot_vector[2] += (minus_half_omega[0] * quat_vector[1]) - (minus_half_omega[1] * quat_vector[0]);

   // Return to calling routine.
   return;
}


/*!
 * @job_class{scheduled}
 */
void compute_Q_dot(
   QuaternionData const * q,
   double const           omega[3],
   QuaternionData       * q_dot )
{
   compute_quat_dot( q->scalar, q->vector, omega, &(q_dot->scalar), q_dot->vector );
   return;
}


/*!
 * @job_class{scheduled}
 */
void compute_omega (
   double const qdot_scalar,
   double const qdot_vector[3],
   double const quat_scalar,
   double const quat_vector[3],
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

void compute_omega_Q (
   QuaternionData const * q_dot,
   QuaternionData const * q,
   double                 omega[3] )
{
   compute_omega( q_dot->scalar,
                  q_dot->vector,
                  q->scalar,
                  q->vector,
                  omega );
   return;
}


/*!
 * @job_class{scheduled}
 */
void compute_quat_dotdot (
   double const   quat_scalar,
   double const   quat_vector[3],
   double const   omega[3],
   double const   omega_dot[3],
   double       * qdotdot_scalar,
   double         qdotdot_vector[3] )
{
   double half_omega_mag = 0.5 * V_MAG( omega );
   double minus_hom_sq = -half_omega_mag * half_omega_mag;
   double minus_half_alpha[3];
   double mhaXqv[3];

   V_SCALE( minus_half_alpha, omega_dot, -0.5 );
   V_CROSS( mhaXqv, minus_half_alpha, quat_vector );

   // Compute the scalar psrt of second derivative of Q.
   *qdotdot_scalar = (minus_hom_sq * quat_scalar) - (V_DOT( minus_half_alpha, quat_vector ));

   // Compute the vector part of second derivative of Q.
   V_SCALE( qdotdot_vector, quat_vector, minus_hom_sq );
   VxS_ADD( qdotdot_vector, minus_half_alpha, quat_scalar );
   V_DECR( qdotdot_vector, mhaXqv );

   // Return to calling routine.
   return;
}


/*!
 * @job_class{scheduled}
 */
void compute_Q_dotdot(
   QuaternionData const * q,
   double const           omega[3],
   double const           omega_dot[3],
   QuaternionData       * q_dotdot )
{
   compute_quat_dotdot( q->scalar, q->vector, omega, omega_dot, &(q_dotdot->scalar), q_dotdot->vector );
   return;
}


/*!
 * @job_class{scheduled}
 * @brief Pre-multiply a quaternion by another's conjugate: prod = conj(lq) * rq.
 */
void conjugate_multiply(
   double const   lq_scalar,
   double const   lq_vector[3],
   double const   rq_scalar,
   double const   rq_vector[3],
   double       * scalar,
   double         vector[3] )
{
   double qv_cross_qv[3];

   //
   // Compute the scalar part of the resulting quaternion.
   //
   // Finish the scalar computation.
   *scalar = (rq_scalar * lq_scalar) + (V_DOT( lq_vector, rq_vector ));

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
void multiply_conjugate(
   double const   lq_scalar,
   double const   lq_vector[3],
   double const   rq_scalar,
   double const   rq_vector[3],
   double       * scalar,
   double         vector[3] )
{
   double qv_cross_qv[3];

   //
   // Compute the scalar part of the resulting quaternion.
   //
   // Finish the scalar computation.
   *scalar = (rq_scalar * lq_scalar) + (V_DOT( lq_vector, rq_vector ));

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
