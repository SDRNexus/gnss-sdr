/*!
 * \file cubature_filter_test.cc
 * \brief  This file implements numerical accuracy test for the CKF library.
 * \author Gerald LaMountain, 2019. gerald(at)ece.neu.edu
 *
 * -----------------------------------------------------------------------------
 *
 * GNSS-SDR is a Global Navigation Satellite System software-defined receiver.
 * This file is part of GNSS-SDR.
 *
 * Copyright (C) 2010-2020  (see AUTHORS file for a list of contributors)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * -----------------------------------------------------------------------------
 */

#include "nonlinear_tracking.h"
#include <armadillo>
#include <gtest/gtest.h>
#include <random>

#define CUBATURE_TEST_N_TRIALS 1000
#define CUBATURE_TEST_TOLERANCE 0.01

class TransitionModel : public ModelFunction
{
public:
    explicit TransitionModel(const arma::mat& kf_F) : coeff_mat(kf_F) {};
    arma::vec operator()(const arma::vec& input) override { return coeff_mat * input; };

private:
    arma::mat coeff_mat;
};

class MeasurementModel : public ModelFunction
{
public:
    explicit MeasurementModel(const arma::mat& kf_H) : coeff_mat(kf_H) {};
    arma::vec operator()(const arma::vec& input) override { return coeff_mat * input; };

private:
    arma::mat coeff_mat;
};

TEST(CubatureFilterComputationTest, CubatureFilterTest)
{
    CubatureFilter kf_cubature;

    arma::vec kf_x;
    arma::mat kf_P_x;

    arma::vec kf_x_pre;
    arma::mat kf_P_x_pre;

    arma::vec ckf_x_pre;
    arma::mat ckf_P_x_pre;

    arma::vec kf_x_post;
    arma::mat kf_P_x_post;

    arma::vec ckf_x_post;
    arma::mat ckf_P_x_post;

    arma::mat kf_F;
    arma::mat kf_H;

    arma::mat kf_Q;
    arma::mat kf_R;

    arma::vec eta;
    arma::vec nu;

    arma::vec kf_y;
    arma::mat kf_P_y;
    arma::mat kf_K;

    ModelFunction* transition_function;
    ModelFunction* measurement_function;

    // -- Perform initializations ------------------------------

    std::random_device r;
    std::default_random_engine e1(r());
    std::normal_distribution<float> normal_dist(0, 5);
    std::uniform_real_distribution<float> uniform_dist(0.1, 5.0);
    std::uniform_int_distribution<> uniform_dist_int(1, 5);

    uint8_t nx = 0;
    uint8_t ny = 0;

    for (uint16_t k = 0; k < CUBATURE_TEST_N_TRIALS; k++)
        {
            nx = static_cast<uint8_t>(uniform_dist_int(e1));
            ny = static_cast<uint8_t>(uniform_dist_int(e1));

            kf_x = arma::randn<arma::vec>(nx, 1);

            kf_P_x_post = 5.0 * arma::diagmat(arma::randu<arma::vec>(nx, 1));
            kf_x_post = arma::mvnrnd(kf_x, kf_P_x_post);

            kf_cubature.initialize(kf_x_post, kf_P_x_post);

            // Prediction Step
            kf_F = arma::randu<arma::mat>(nx, nx);
            kf_Q = arma::diagmat(arma::randu<arma::vec>(nx, 1));

            transition_function = new TransitionModel(kf_F);
            arma::mat ttx = (*transition_function)(kf_x_post);

            kf_cubature.predict_sequential(kf_x_post, kf_P_x_post, transition_function, kf_Q);

            ckf_x_pre = kf_cubature.get_x_pred();
            ckf_P_x_pre = kf_cubature.get_P_x_pred();

            kf_x_pre = kf_F * kf_x_post;
            kf_P_x_pre = kf_F * kf_P_x_post * kf_F.t() + kf_Q;

            EXPECT_TRUE(arma::approx_equal(ckf_x_pre, kf_x_pre, "absdiff", CUBATURE_TEST_TOLERANCE));
            EXPECT_TRUE(arma::approx_equal(ckf_P_x_pre, kf_P_x_pre, "absdiff", CUBATURE_TEST_TOLERANCE));

            // Update Step
            kf_H = arma::randu<arma::mat>(ny, nx);
            kf_R = arma::diagmat(arma::randu<arma::vec>(ny, 1));

            eta = arma::mvnrnd(arma::zeros<arma::vec>(nx, 1), kf_Q);
            nu = arma::mvnrnd(arma::zeros<arma::vec>(ny, 1), kf_R);

            kf_y = kf_H * (kf_F * kf_x + eta) + nu;

            measurement_function = new MeasurementModel(kf_H);
            kf_cubature.update_sequential(kf_y, kf_x_pre, kf_P_x_pre, measurement_function, kf_R);

            ckf_x_post = kf_cubature.get_x_est();
            ckf_P_x_post = kf_cubature.get_P_x_est();

            kf_P_y = kf_H * kf_P_x_pre * kf_H.t() + kf_R;
            kf_K = (kf_P_x_pre * kf_H.t()) * arma::inv(kf_P_y);

            kf_x_post = kf_x_pre + kf_K * (kf_y - kf_H * kf_x_pre);
            kf_P_x_post = (arma::eye(nx, nx) - kf_K * kf_H) * kf_P_x_pre;

            EXPECT_TRUE(arma::approx_equal(ckf_x_post, kf_x_post, "absdiff", CUBATURE_TEST_TOLERANCE));
            EXPECT_TRUE(arma::approx_equal(ckf_P_x_post, kf_P_x_post, "absdiff", CUBATURE_TEST_TOLERANCE));

            delete transition_function;
            delete measurement_function;
        }
}
