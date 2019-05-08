// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2018 Intel Corporation


#ifndef OPENCV_GAPI_GCPUIMGPROC_HPP
#define OPENCV_GAPI_GCPUIMGPROC_HPP

#include <map>
#include <string>

#include "opencv2/gapi/cpu/gcpukernel.hpp"

namespace cv { namespace gimpl {

// NB: This is what a "Kernel Package" from the origianl Wiki doc should be.
void loadCPUImgProc(std::map<std::string, cv::GCPUKernel> &kmap);

}

namespace gapi {

void convertRGB2YUV422(const cv::Mat& in, cv::Mat& out);

}

}

#endif // OPENCV_GAPI_GCPUIMGPROC_HPP
