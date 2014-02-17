// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

// Copyright (C) 2014, Advanced Micro Devices, Inc., all rights reserved.
// Third party copyrights are property of their respective owners.

#ifndef __OPENCV_FAST_NLMEANS_DENOISING_OPENCL_HPP__
#define __OPENCV_FAST_NLMEANS_DENOISING_OPENCL_HPP__

#include "precomp.hpp"

#define CV_OPENCL_RUN_ASSERT
#include "opencl_kernels.hpp"

namespace cv {

enum
{
    BLOCK_ROWS = 32,
    BLOCK_COLS = 128,
    CTA_SIZE = 128
};

static inline int getNearestPowerOf2(int value)
{
    int p = 0;
    while (1 << p < value)
        ++p;
    return p;
}

static int divUp(int a, int b)
{
    return (a + b - 1) / b;
}

static bool ocl_calcAlmostDist2Weight(UMat & almostDist2Weight, int searchWindowSize, int templateWindowSize, float h, int cn,
                                      int & almostTemplateWindowSizeSqBinShift)
{
    const int maxEstimateSumValue = searchWindowSize * searchWindowSize * 255;
    int fixedPointMult = std::numeric_limits<int>::max() / maxEstimateSumValue;

    // precalc weight for every possible l2 dist between blocks
    // additional optimization of precalced weights to replace division(averaging) by binary shift
    CV_Assert(templateWindowSize <= 46340); // sqrt(INT_MAX)
    int templateWindowSizeSq = templateWindowSize * templateWindowSize;
    almostTemplateWindowSizeSqBinShift = getNearestPowerOf2(templateWindowSizeSq);
    float almostDist2ActualDistMultiplier = (float)(1 << almostTemplateWindowSizeSqBinShift) / templateWindowSizeSq;

    const float WEIGHT_THRESHOLD = 1e-3f;
    int maxDist = 255 * 255 * cn;
    int almostMaxDist = (int)(maxDist / almostDist2ActualDistMultiplier + 1);
    float den = 1.0f / (h * h * cn);

    almostDist2Weight.create(1, almostMaxDist, CV_32SC1);

    ocl::Kernel k("calcAlmostDist2Weight", ocl::photo::nlmeans_oclsrc,
                  "-D OP_CALC_WEIGHTS");
    if (k.empty())
        return false;

    k.args(ocl::KernelArg::PtrWriteOnly(almostDist2Weight), almostMaxDist,
           almostDist2ActualDistMultiplier, fixedPointMult, den, WEIGHT_THRESHOLD);

    size_t globalsize[1] = { almostMaxDist };
    return k.run(1, globalsize, NULL, false);
}

static bool ocl_fastNlMeansDenoising(InputArray _src, OutputArray _dst, float h,
                                     int templateWindowSize, int searchWindowSize)
{
    int type = _src.type(), depth = CV_MAT_DEPTH(type), cn = CV_MAT_CN(type);
    Size size = _src.size();

    if ( !(depth == CV_8U && cn <= 4 && cn != 3) )
        return false;

    int templateWindowHalfWize = templateWindowSize / 2;
    int searchWindowHalfSize = searchWindowSize / 2;
    templateWindowSize  = templateWindowHalfWize * 2 + 1;
    searchWindowSize = searchWindowHalfSize * 2 + 1;
    int nblocksx = divUp(size.width, BLOCK_COLS), nblocksy = divUp(size.height, BLOCK_ROWS);
    int almostTemplateWindowSizeSqBinShift = -1;

    char cvt[2][40];
    String opts = format("-D OP_CALC_FASTNLMEANS -D TEMPLATE_SIZE=%d -D SEARCH_SIZE=%d"
                         " -D uchar_t=%s -D int_t=%s -D BLOCK_COLS=%d -D BLOCK_ROWS=%d"
                         " -D CTA_SIZE=%d -D TEMPLATE_SIZE2=%d -D SEARCH_SIZE2=%d"
                         " -D convert_int_t=%s -D cn=%d -D CTA_SIZE2=%d -D convert_uchar_t=%s",
                         templateWindowSize, searchWindowSize, ocl::typeToStr(type),
                         ocl::typeToStr(CV_32SC(cn)), BLOCK_COLS, BLOCK_ROWS, CTA_SIZE,
                         templateWindowHalfWize, searchWindowHalfSize,
                         ocl::convertTypeStr(CV_8U, CV_32S, cn, cvt[0]), cn,
                         CTA_SIZE >> 1, ocl::convertTypeStr(CV_32S, CV_8U, cn, cvt[1]));

    ocl::Kernel k("fastNlMeansDenoising", ocl::photo::nlmeans_oclsrc, opts);
    if (k.empty())
        return false;

    UMat almostDist2Weight;
    if (!ocl_calcAlmostDist2Weight(almostDist2Weight, searchWindowSize, templateWindowSize, h, cn,
                                   almostTemplateWindowSizeSqBinShift))
        return false;
    CV_Assert(almostTemplateWindowSizeSqBinShift >= 0);

    UMat srcex;
    int borderSize = searchWindowHalfSize + templateWindowHalfWize;
    copyMakeBorder(_src, srcex, borderSize, borderSize, borderSize, borderSize, BORDER_DEFAULT);

    _dst.create(size, type);
    UMat dst = _dst.getUMat();

    int searchWindowSizeSq = searchWindowSize * searchWindowSize;
    Size upColSumSize(size.width, searchWindowSizeSq * nblocksy);
    Size colSumSize(nblocksx * templateWindowSize, searchWindowSizeSq * nblocksy);
    UMat buffer(upColSumSize + colSumSize, CV_32SC(cn));

    srcex = srcex(Rect(Point(borderSize, borderSize), size));
    k.args(ocl::KernelArg::ReadOnlyNoSize(srcex), ocl::KernelArg::WriteOnly(dst),
           ocl::KernelArg::PtrReadOnly(almostDist2Weight),
           ocl::KernelArg::PtrReadOnly(buffer), almostTemplateWindowSizeSqBinShift);

    size_t globalsize[2] = { nblocksx * BLOCK_COLS, nblocksy * BLOCK_ROWS }, localsize[2] = { CTA_SIZE, 1 };
    return k.run(2, globalsize, localsize, false);
}

}

#endif
