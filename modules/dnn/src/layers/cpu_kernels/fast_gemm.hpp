// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

// This file is modified from the ficus (https://github.com/vpisarev/ficus/blob/master/runtime/ficus/impl/gemm.impl.h).
// Here is the original license:
/*
    This file is a part of ficus language project.
    See ficus/LICENSE for the licensing terms
*/

#ifndef OPENCV_DNN_FAST_GEMM_HPP
#define OPENCV_DNN_FAST_GEMM_HPP

#include "opencv2/core/hal/intrin.hpp"
#include <opencv2/dnn/shape_utils.hpp>

namespace cv { namespace dnn {

struct FastGemmOpt {
    bool use_avx;
    bool use_avx2;
    bool use_neon_aarch64;

    FastGemmOpt() {
        use_avx = false;
        use_avx2 = false;
        use_neon_aarch64 = false;
    }

    void init() {
        use_avx = checkHardwareSupport(CPU_AVX);
        use_avx2 = checkHardwareSupport(CPU_AVX2);
#ifdef CV_NEON_AARCH64
        use_neon_aarch64 = checkHardwareSupport(CPU_NEON) && CV_NEON_AARCH64;
#else
        use_neon_aarch64 = false;
#endif
    }

    bool all() {
        return use_avx || use_avx2 || use_neon_aarch64;
    }
};

void fastGemmPackB(const Mat &m, std::vector<float> &packed_B, bool trans, FastGemmOpt &opt);

void fastGemm(bool trans_a, int M, int N, int K,
              float alpha, const float *A, int lda,
              const float *packed_B, float beta,
              float *C, int ldc, FastGemmOpt &opt);
void fastGemm(bool trans_a, bool trans_b, int ma, int na, int mb, int nb,
              float alpha, const float *A, int lda0, int lda1, const float *B, int ldb0, int ldb1,
              float beta, float *C, int ldc, FastGemmOpt &opt);
void fastGemm(bool trans_a, bool trans_b,
              float alpha, const Mat &A, const Mat &B,
              float beta, Mat &C, FastGemmOpt &opt);

}} // cv::dnn

#endif // OPENCV_DNN_FAST_GEMM_HPP
