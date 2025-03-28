/*!
@file SpaceFOM/QuaternionData.hh
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
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Made into full class.}
@revs_end

*/

#ifndef SPACEFOM_QUATERNION_DATA_HH
#define SPACEFOM_QUATERNION_DATA_HH

// System include files.
#include <iostream>
#include <string>

// Trick include files.
#include "trick/reference_frame.h"

namespace SpaceFOM
{

class QuaternionData
{

  public:
   double scalar;    ///< @trick_units{--} Attitude quaternion scalar.
   double vector[3]; ///< @trick_units{--} Attitude quaternion vector.

   // Default constructor.
   QuaternionData();

   // Copy constructor.
   /*! @brief Copy constructor for QuaternionData class.
    *  @param source Source data to copy from. */
   QuaternionData( QuaternionData const &source );

   // Initialization constructor.
   /*! @brief Euler angle initialization constructor.
    *  @param sequence Euler sequence of angles.
    *  @param angles   Euler attitude angels {rad}. */
   QuaternionData( Euler_Seq sequence, double const angles[3] );

   // Initialization constructor.
   /*! @brief Transformation matrix initialization constructor.
    *  @param T Direction cosine transformation matrix. */
   explicit QuaternionData( double const T[3][3] );

   /*! @brief Assignment operator for QuaternionData class.
    *  @param rhs Right operand data to copy from. */
   QuaternionData &operator=( QuaternionData const &rhs );

   /*! @brief Equal comparison operator for QuaternionData class.
    *  @param rhs Right operand data to compare to. */
   bool operator==( QuaternionData const &rhs );

   /*! @brief Not equal comparison operator for QuaternionData class.
    *  @param rhs Right operand data to compare to. */
   bool operator!=( QuaternionData const &rhs );

   /*! @brief Print out the quaternion values.
    *  @param stream Output stream. */
   void print_data( std::ostream &stream = std::cout ) const;

   /***********************************************************************
    * QuaternionData methods.
    ***********************************************************************/

   /*! @brief Initialize the attitude quaternion. */
   void initialize();

   /*! @brief Set attitude quaternion from Euler angles.
    *  @param sequence Euler sequence of angles.
    *  @param angles   Euler attitude angels {rad}. */
   void set_from_Euler(
      Euler_Seq    sequence,
      double const angles[3] );

   /*! @brief Set attitude quaternion from Euler angles in degrees.
    *  @param sequence   Euler sequence of angles.
    *  @param angles_deg Euler attitude angels {deg}. */
   void set_from_Euler_deg(
      Euler_Seq    sequence,
      double const angles_deg[3] );

   /*! @brief Get Euler angles from attitude quaternion.
    *  @param sequence Euler sequence of angles.
    *  @param angles   Euler attitude angels {rad}. */
   void get_Euler(
      Euler_Seq sequence,
      double    angles[3] ) const;

   /*! @brief Get Euler angles from attitude quaternion.
    *  @param sequence   Euler sequence of angles.
    *  @param angles_deg Euler attitude angels {deg}. */
   void get_Euler_deg(
      Euler_Seq sequence,
      double    angles_deg[3] ) const;

   /*! @brief Set attitude quaternion from transformation matrix.
    *  @param T  Direction cosine transformation matrix. */
   void set_from_transfrom( double const T[3][3] );

   /*! @brief Get transformation matrix from attitude quaternion.
    *  @param T  Direction cosine transformation matrix. */
   void get_transfrom( double T[3][3] ) const;

   /*! @brief Scale the attitude quaternion.
    *  @param factor Scale factor. */
   void scale( double factor );

   /*! @brief Copy the attitude quaternion.
    *  @param source Source quaternion to copy from. */
   void copy( QuaternionData const &source );

   /*! @brief Compute the conjugate of this quaternion. */
   void conjugate();

   /*! @brief Compute the conjugate of the quaternion.
    *  @param source Source quaternion to conjugate from. */
   void conjugate( QuaternionData const &source );

   /*! @brief Normalize the attitude quaternion. */
   void normalize();

   /*! @brief Compare attitude quaternions.
    *  @return True if equal and false if not.
    *  @param source Quaternion to compare to. */
   bool is_equal( QuaternionData const &source );

   /*! @brief Multiply quaternions.
    *  @param left  Left quaternion operand.
    *  @param right Right quaternion operand. */
   void multiply(
      QuaternionData const &left,
      QuaternionData const &right );

   /*! @brief Multiply a quaternion and a vector.
    *  @param left  Left quaternion operand.
    *  @param right Right vector operand. */
   void multiply(
      QuaternionData const &left,
      double                right[3] );

   /*! @brief Multiply a vector and a quaternion.
    *  @param left  Left vector operand.
    *  @param right Right quaternion operand. */
   void multiply(
      double                left[3],
      QuaternionData const &right );

   /*! @brief Compute the first time derivative of the attitude quaternion.
    *  @param quat  Attitude quaternion.
    *  @param omega Angular velocity vector. */
   void derivative_first(
      QuaternionData const &quat,
      double const          omega[3] );

   /*! @brief Compute the first time derivative of the attitude quaternion.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param omega Angular velocity vector. */
   void derivative_first(
      double const quat_scalar,
      double const quat_vector[3],
      double const omega[3] );

   /*! @brief Compute the second time derivative of the attitude quaternion.
    *  @param quat      Attitude quaternion.
    *  @param omega     Angular velocity vector.
    *  @param omega_dot Angular acceleration vector. */
   void derivative_second(
      QuaternionData const &quat,
      double const          omega[3],
      double const          omega_dot[3] );

   /*! @brief Compute the second time derivative of the attitude quaternion.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param omega       Angular velocity vector.
    *  @param omega_dot   Angular acceleration vector. */
   void derivative_second(
      double const quat_scalar,
      double const quat_vector[3],
      double const omega[3],
      double const omega_dot[3] );

   /*! @brief Compute the angular rate from the attitude quaternion rate and the
    *  associated attitude quaternion.
    *  @param att_quat The associated attitude quaternion.
    *  @param omega    Angular velocity vector. */
   void compute_omega(
      QuaternionData const &att_quat,
      double                omega[3] );

   /*! @brief Transform a vector using this quaternion.
    *  @param v_in  Source vector.
    *  @param v_out Transformed vector. */
   void transform_vector(
      double const v_in[3],
      double       v_out[3] ) const;

   /*! @brief Conjugate transform a vector using this quaternion.
    *  @param v_in  Source vector.
    *  @param v_out Transformed vector. */
   void conjugate_transform_vector(
      double const v_in[3],
      double       v_out[3] ) const;

   /***********************************************************************
    * Static methods.
    ***********************************************************************/

   /*! @brief Multiply two quaternions.
    *  @param ls Left operand scalar.
    *  @param lv Left operand vector.
    *  @param rs Right operand scalar.
    *  @param rv Right operand vector.
    *  @param ps Product scalar.
    *  @param pv Product vector. */
   static void multiply_sv(
      double const ls,
      double const lv[3],
      double const rs,
      double const rv[3],
      double      *ps,
      double       pv[3] );

   /*! @brief Multiply a quaternion by a vector.
    *  @param ls Left operand scalar.
    *  @param lv Left operand vector.
    *  @param rv Right operand vector.
    *  @param ps Product scalar.
    *  @param pv Product vector. */
   static void left_multiply_v(
      double const ls,
      double const lv[3],
      double const rv[3],
      double      *ps,
      double       pv[3] );

   /*! @brief Multiply a vector by a quaternion.
    *  @param lv Left operand vector.
    *  @param rs Right operand scalar.
    *  @param rv Right operand vector.
    *  @param ps Product scalar.
    *  @param pv Product vector. */
   static void right_multiply_v(
      double const lv[3],
      double const rs,
      double const rv[3],
      double      *ps,
      double       pv[3] );

   /*! @brief Normalize the attitude quaternion.
    *  @param qs Pointer to the attitude quaternion scalar.
    *  @param qv Attitude quaternion vector. */
   static void normalize(
      double *qs,
      double  qv[3] );

   /*! @brief Compare attitude quaternions.
    *  @return True if equal and false if not.
    *  @param lhs Left operand quaternion.
    *  @param rhs Right operand quaternion. */
   static bool is_equal(
      QuaternionData const &lhs,
      QuaternionData const &rhs );

   /*! @brief Compare attitude quaternions.
    *  @return True if equal and false if not.
    *  @param lhs_scalar Left operand quaternion scalar.
    *  @param lhs_vector Left operand quaternion vector.
    *  @param rhs_scalar Right operand quaternion scalar.
    *  @param rhs_vector Right operand quaternion vector. */
   static bool is_equal(
      double const lhs_scalar,
      double const lhs_vector[3],
      double const rhs_scalar,
      double const rhs_vector[3] );

   /*! @brief Compute the rate of the attitude quaternion.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param omega Angular velocity vector.
    *  @param qdot_scalar Reference to the scalar part of the quaternion rate.
    *  @param qdot_vector Reference to the vector part of the quaternion rate. */
   static void compute_derivative(
      double const quat_scalar,
      double const quat_vector[3],
      double const omega[3],
      double      *qdot_scalar,
      double       qdot_vector[3] );

   /*! @brief Compute the rate of the attitude quaternion.
    *  @param q     Attitude quaternion.
    *  @param omega Angular velocity vector.
    *  @param q_dot Attitude quaternion rate. */
   static void compute_derivative(
      QuaternionData const *q,
      double const          omega[3],
      QuaternionData       *q_dot );

   /*! @brief Compute the acceleration of the attitude quaternion.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param omega       Angular velocity vector.
    *  @param omega_dot   Angular accelaration vector.
    *  @param qdotdot_scalar Reference to the scalar part of the quaternion acceleration.
    *  @param qdotdot_vector Vector part of the quaternion acceleration. */
   static void compute_2nd_derivative(
      double const quat_scalar,
      double const quat_vector[3],
      double const omega[3],
      double const omega_dot[3],
      double      *qdotdot_scalar,
      double       qdotdot_vector[3] );

   /*! @brief Compute the acceleration of the attitude quaternion.
    *  @param q         Pointer to an attitude quaternion.
    *  @param omega     Angular velocity vector.
    *  @param omega_dot Angular accelaration vector.
    *  @param q_dotdot  Pointer to the quaternion acceleration. */
   static void compute_2nd_derivative(
      QuaternionData const *q,
      double const          omega[3],
      double const          omega_dot[3],
      QuaternionData       *q_dotdot );

   /*! @brief Compute the angular rate from the attitude quaternion rate and the
    *  attitude quaternion.
    *  @param quat_scalar Scalar part of the attitude quaternion.
    *  @param quat_vector Vector part of the attitude quaternion.
    *  @param qdot_scalar Scalar part of the quaternion rate.
    *  @param qdot_vector Vector part of the quaternion rate.
    *  @param omega Angular velocity vector. */
   static void compute_omega(
      double const quat_scalar,
      double const quat_vector[3],
      double const qdot_scalar,
      double const qdot_vector[3],
      double       omega[3] );

   /*! @brief Compute the angular rate from the attitude quaternion rate and the
    *  attitude quaternion.
    *  @param q     Pointer to an attitude quaternion.
    *  @param q_dot Pointer to an attitude quaternion rate.
    *  @param omega Angular velocity vector. */
   static void compute_omega(
      QuaternionData const *q,
      QuaternionData const *q_dot,
      double                omega[3] );

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
      double      *scalar,
      double       vector[3] );

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
      double      *scalar,
      double       vector[3] );
};

} // namespace SpaceFOM

#endif // SPACEFOM_QUATERNION_DATA_HH: Do NOT put anything after this line!
