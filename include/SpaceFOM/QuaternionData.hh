/*!
@file SpaceFOM/QuaternionData.h
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
and decode a SISO Space Reference FOM Quaternion data type.

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

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/SpaceFOM/QuaternionData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, July 2018, --, Initial version }
@revs_end

*/

#ifndef SPACEFOM_QUATERNION_DATA_HH
#define SPACEFOM_QUATERNION_DATA_HH

namespace SpaceFOM {

class QuaternionData{

  public:
   double scalar;    ///< @trick_units{--} Attitude quaternion scalar.
   double vector[3]; ///< @trick_units{--} Attitude quaternion vector.

   // Default constructor.
   QuaternionData(){ this->initialize(); }

   /*! @brief Initialize the attitude quaternion. */
   void initialize ();

   /*! @brief Normalize the attitude quaternion.
    *  @param quat_scalar Pointer to the attitude quaternion scalar.
    *  @param quat_vector Attitude quaternion vector. */
   void normalize ();

   /*! @brief Normalize the attitude quaternion.
    *  @param quat_scalar Pointer to the attitude quaternion scalar.
    *  @param quat_vector Attitude quaternion vector. */
   static void normalize_quaternion (
      double * quat_scalar,
      double   quat_vector[3]);

   /*! @brief Compute the rate of the attitude quaternion.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param omega Angular velocity vector.
    *  @param qdot_scalar Reference to the scalar part of the quaternion rate.
    *  @param qdot_vector Reference to the vector part of the quaternion rate. */
   static void compute_quat_dot (
      double const   quat_scalar,
      double const   quat_vector[3],
      double const   omega[3],
      double       * qdot_scalar,
      double         qdot_vector[3] );

   /*! @brief Compute the rate of the attitude quaternion.
    *  @param q     Attitude quaternion.
    *  @param omega Angular velocity vector.
    *  @param qdot  Attitude quaternion rate. */
   static void compute_Q_dot(
      QuaternionData const * q,
      double const           omega[3],
      QuaternionData       * q_dot );

   /*! @brief Compute the acceleration of the attitude quaternion.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param omega       Angular velocity vector.
    *  @param omega_dot   Angular accelaration vector.
    *  @param qdotdot_scalar Reference to the scalar part of the quaternion acceleration.
    *  @param qdotdot_vector Vector part of the quaternion acceleration. */
   static void compute_quat_dotdot (
      double const   quat_scalar,
      double const   quat_vector[3],
      double const   omega[3],
      double const   omega_dot[3],
      double       * qdotdot_scalar,
      double         qdotdot_vector[3] );

   /*! @brief Compute the acceleration of the attitude quaternion.
    *  @param q         Pointer to an attitude quaternion.
    *  @param omega     Angular velocity vector.
    *  @param omega_dot Angular accelaration vector.
    *  @param q_dotdot  Pointer to the quaternion acceleration. */
   static void compute_Q_dotdot(
      QuaternionData const * q,
      double const           omega[3],
      double const           omega_dot[3],
      QuaternionData       * q_dotdot );

   /*! @brief Compute the angluar rate from the attitude quaternion rate and the
    *  attitude quaternion.
    *  @param qdot_scalar Scalar part of the quaternion rate.
    *  @param qdot_vector Vector part of the quaternion rate.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param omega Angular velocity vector. */
   static void compute_omega (
      double const qdot_scalar,
      double const qdot_vector[3],
      double const quat_scalar,
      double const quat_vector[3],
      double       omega[3] );

   /*! @brief Compute the angluar rate from the attitude quaternion rate and the
    *  attitude quaternion.
    *  @param qdot  Pointer to an attitude quaternion rate.
    *  @param q     Pointer to an attitude quaternion.
    *  @param omega Angular velocity vector. */
   static void compute_omega_Q (
      QuaternionData const * q_dot,
      QuaternionData const * q,
      double                 omega[3] );

   /*! @brief Pre-multiply a quaternion by another's conjugate: prod = conj(lq) * rq.
    *  @param lq_scalar Left side quaternion scalar.
    *  @param lq_vector Left side quaternion vector.
    *  @param rq_scalar Right side quaternion scalar.
    *  @param rq_vector Right side quaternion vector.
    *  @param scalar Pointer to product quaternion scalar.
    *  @param vector Product quaternion vector. */
   static void conjugate_multiply(
      double const lq_scalar,
      double const lq_vector[3],
      double const rq_scalar,
      double const rq_vector[3],
      double       * scalar,
      double         vector[3] );

   /*! @brief Post-multiply a quaternion by another's conjugate: prod = lq * conj(rq).
    *  @param lq_scalar Left side quaternion scalar.
    *  @param lq_vector Left side quaternion vector.
    *  @param rq_scalar Right side quaternion scalar.
    *  @param rq_vector Right side quaternion vector.
    *  @param scalar Pointer to product quaternion scalar.
    *  @param vector Product quaternion vector. */
   static void multiply_conjugate(
      double const lq_scalar,
      double const lq_vector[3],
      double const rq_scalar,
      double const rq_vector[3],
      double       * scalar,
      double         vector[3] );

};

} // namespace SpaceFOM

#endif // SPACEFOM_QUATERNION_DATA_HH: Do NOT put anything after this line!
