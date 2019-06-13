/*!
 * \file unscented_filter_test.cc
 * \brief  This file implements numerical accuracy test for the CKF library.
 * \author Gerald LaMountain, 2019. gerald(at)ece.neu.edu
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2019  (see AUTHORS file for a list of contributors)
 *
 * GNSS-SDR is a software defined Global Navigation
 *          Satellite Systems receiver
 *
 * This file is part of GNSS-SDR.
 *
 * GNSS-SDR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNSS-SDR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNSS-SDR. If not, see <https://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------
 */

#include "nonlinear_tracking.h"
#include <armadillo>
#include <gtest/gtest.h>
#include <random>

#define UNSCENTED_TEST_N_TRIALS 10
#define UNSCENTED_TEST_TOLERANCE 10

class Transition_Model_UKF : public Model_Function {
    public:
        Transition_Model_UKF(arma::mat kf_F) {coeff_mat = kf_F;};
        virtual arma::vec operator() (arma::vec input) {return coeff_mat*input;};
    private:
        arma::mat coeff_mat;
};

class Measurement_Model_UKF : public Model_Function {
    public:
        Measurement_Model_UKF(arma::mat kf_H) {coeff_mat = kf_H;};
        virtual arma::vec operator() (arma::vec input) {return coeff_mat*input;};
    private:
        arma::mat coeff_mat;
};

TEST(UnscentedFilterComputationTest, UnscentedFilterTest)
{
    Unscented_filter kf_unscented;

    arma::vec kf_x;
    arma::mat kf_P_x;

    arma::vec kf_x_pre;
    arma::mat kf_P_x_pre;

    arma::vec ukf_x_pre;
    arma::mat ukf_P_x_pre;

    arma::vec kf_x_post;
    arma::mat kf_P_x_post;

    arma::vec ukf_x_post;
    arma::mat ukf_P_x_post;

    arma::mat kf_F;
    arma::mat kf_H;

    arma::mat kf_Q;
    arma::mat kf_R;

    arma::vec eta;
    arma::vec nu;

    arma::vec kf_y;
    arma::mat kf_P_y;
    arma::mat kf_K;

    Model_Function* transition_function;
    Model_Function* measurement_function;

    //--- Perform initializations ------------------------------

    std::random_device r;
    std::default_random_engine e1(r());
    std::normal_distribution<float> normal_dist(0, 5);
    std::uniform_real_distribution<float> uniform_dist(0.1, 5.0);

    uint8_t nx = 0;
    uint8_t ny = 0;

    for (uint16_t k = 0; k < UNSCENTED_TEST_N_TRIALS; k++)
        {
            nx = std::rand() % 5 + 1;
            ny = std::rand() % 5 + 1;

            kf_x = arma::randn<arma::vec>(nx,1);

            kf_P_x_post = 5.0 * arma::diagmat(arma::randu<arma::vec>(nx,1));
            kf_x_post = arma::mvnrnd(kf_x, kf_P_x_post);

            kf_unscented.initialize(kf_x_post, kf_P_x_post);

            // Prediction Step
            kf_F = arma::randu<arma::mat>(nx,nx);
            kf_Q = arma::diagmat(arma::randu<arma::vec>(nx,1));

            transition_function = new Transition_Model_UKF(kf_F);
            arma::mat ttx = (*transition_function)(kf_x_post);

            kf_unscented.predict_sequential(kf_x_post,kf_P_x_post,transition_function,kf_Q);

            ukf_x_pre = kf_unscented.get_x_pred();
            ukf_P_x_pre = kf_unscented.get_P_x_pred();

            kf_x_pre = kf_F * kf_x_post;
            kf_P_x_pre = kf_F * kf_P_x_post * kf_F.t() + kf_Q;

            EXPECT_TRUE(arma::approx_equal(ukf_x_pre, kf_x_pre, "absdiff", UNSCENTED_TEST_TOLERANCE));
            EXPECT_TRUE(arma::approx_equal(ukf_P_x_pre, kf_P_x_pre, "absdiff", UNSCENTED_TEST_TOLERANCE));

            // Update Step
            kf_H = arma::randu<arma::mat>(ny,nx);
            kf_R = arma::diagmat(arma::randu<arma::vec>(ny,1));

            eta = arma::mvnrnd(arma::zeros<arma::vec>(nx,1),kf_Q);
            nu = arma::mvnrnd(arma::zeros<arma::vec>(ny,1),kf_R);

            kf_y = kf_H*(kf_F*kf_x + eta) + nu;

            measurement_function = new Measurement_Model_UKF(kf_H);
            kf_unscented.update_sequential(kf_y,kf_x_pre,kf_P_x_pre,measurement_function,kf_R);

            ukf_x_post = kf_unscented.get_x_est();
            ukf_P_x_post = kf_unscented.get_P_x_est();

            kf_P_y = kf_H * kf_P_x_pre * kf_H.t() + kf_R;
            kf_K = (kf_P_x_pre * kf_H.t()) * arma::inv(kf_P_y);

            kf_x_post = kf_x_pre + kf_K * (kf_y - kf_H * kf_x_pre);
            kf_P_x_post = (arma::eye(nx,nx) - kf_K * kf_H) * kf_P_x_pre;

            EXPECT_TRUE(arma::approx_equal(ukf_x_post, kf_x_post, "absdiff", UNSCENTED_TEST_TOLERANCE));
            EXPECT_TRUE(arma::approx_equal(ukf_P_x_post, kf_P_x_post, "absdiff", UNSCENTED_TEST_TOLERANCE));

            delete transition_function;
            delete measurement_function;
        }
}
