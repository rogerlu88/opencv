// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level
// directory of this distribution and at http://opencv.org/license.html

#ifdef HAVE_AVIF

#include "precomp.hpp"

#include <avif/avif.h>
#include <fstream>

#include <opencv2/core/utils/configuration.private.hpp>
#include "opencv2/imgproc.hpp"
#include "grfmt_avif.hpp"

#define CV_AVIF_USE_QUALITY \
  (AVIF_VERSION > ((0 * 1000000) + (11 * 10000) + (1 * 100)))

#if !CV_AVIF_USE_QUALITY
#define AVIF_QUALITY_LOSSLESS 100
#define AVIF_QUALITY_WORST 0
#define AVIF_QUALITY_BEST 100

#endif

namespace cv {
namespace {

struct AvifImageDeleter {
  void operator()(avifImage *image) { avifImageDestroy(image); }
};

using AvifImageUniquePtr = std::unique_ptr<avifImage, AvifImageDeleter>;

avifResult CopyToMat(const avifImage *image, int channels, Mat *mat) {
  CV_Assert((int)image->height == mat->rows);
  CV_Assert((int)image->width == mat->cols);
  if (channels == 1) {
    cv::Mat(image->height, image->width,
            CV_MAKE_TYPE((image->depth == 8) ? CV_8U : CV_16U, 1),
            image->yuvPlanes[0], image->yuvRowBytes[0])
        .copyTo(*mat);
    return AVIF_RESULT_OK;
  }
  avifRGBImage rgba;
  avifRGBImageSetDefaults(&rgba, image);
  if (channels == 3) {
    rgba.format = AVIF_RGB_FORMAT_BGR;
  } else {
    CV_Assert(channels == 4);
    rgba.format = AVIF_RGB_FORMAT_BGRA;
  }
  rgba.rowBytes = mat->step[0];
  rgba.depth = image->depth;
  rgba.pixels = reinterpret_cast<uint8_t *>(mat->data);
  return avifImageYUVToRGB(image, &rgba);
}

AvifImageUniquePtr ConvertToAvif(const cv::Mat &img, bool lossless,
                                 int bit_depth) {
  // img must be 8 bit or 16 bit (rescaled float image).
  CV_Assert(img.depth() == CV_8U || img.depth() == CV_16U);

  const int width = img.cols;
  const int height = img.rows;

  avifImage *result;

  if (img.channels() == 1) {
    result = avifImageCreateEmpty();
    if (result == nullptr) return nullptr;
    result->width = width;
    result->height = height;
    result->depth = bit_depth;
    result->yuvFormat = AVIF_PIXEL_FORMAT_YUV400;
    result->colorPrimaries = AVIF_COLOR_PRIMARIES_UNSPECIFIED;
    result->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_UNSPECIFIED;
    result->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_IDENTITY;
    result->yuvRange = AVIF_RANGE_FULL;
    result->yuvPlanes[0] = img.data;
    result->yuvRowBytes[0] = img.step[0];
    result->imageOwnsYUVPlanes = AVIF_FALSE;
    return AvifImageUniquePtr(result);
  }

  if (lossless) {
    result =
        avifImageCreate(width, height, bit_depth, AVIF_PIXEL_FORMAT_YUV444);
    if (result == nullptr) return nullptr;
    result->colorPrimaries = AVIF_COLOR_PRIMARIES_UNSPECIFIED;
    result->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_UNSPECIFIED;
    result->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_IDENTITY;
    result->yuvRange = AVIF_RANGE_FULL;
  } else {
    result =
        avifImageCreate(width, height, bit_depth, AVIF_PIXEL_FORMAT_YUV420);
    if (result == nullptr) return nullptr;
    result->colorPrimaries = AVIF_COLOR_PRIMARIES_BT709;
    result->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SRGB;
    result->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_BT601;
    result->yuvRange = AVIF_RANGE_FULL;
  }

  avifRGBImage rgba;
  avifRGBImageSetDefaults(&rgba, result);
  if (img.channels() == 3) {
    rgba.format = AVIF_RGB_FORMAT_BGR;
  } else {
    CV_Assert(img.channels() == 4);
    rgba.format = AVIF_RGB_FORMAT_BGRA;
  }
  rgba.rowBytes = img.step[0];
  rgba.depth = bit_depth;
  rgba.pixels =
      const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(img.data));

  avifImageRGBToYUV(result, &rgba);
  return AvifImageUniquePtr(result);
}

}  // namespace

// 64Mb limit to avoid memory saturation.
static const size_t kParamMaxFileSize = utils::getConfigurationParameterSizeT(
    "OPENCV_IMGCODECS_AVIF_MAX_FILE_SIZE", 64 * 1024 * 1024);

static constexpr size_t kAvifSignatureSize = 500;

AvifDecoder::AvifDecoder() {
  m_buf_supported = true;
  channels_ = 0;
  decoder_ = avifDecoderCreate();
  if (decoder_) decoder_->allowIncremental = AVIF_TRUE;
}

AvifDecoder::~AvifDecoder() {
  if (decoder_ != nullptr) avifDecoderDestroy(decoder_);
}

size_t AvifDecoder::signatureLength() const { return kAvifSignatureSize; }

bool AvifDecoder::checkSignature(const String &signature) const {
  avifDecoderSetIOMemory(decoder_,
                         reinterpret_cast<const uint8_t *>(signature.c_str()),
                         signature.size());
  decoder_->io->sizeHint = 1e9;
  const avifResult status = avifDecoderParse(decoder_);
  return (status != AVIF_RESULT_INVALID_FTYP &&
          status != AVIF_RESULT_BMFF_PARSE_FAILED);
}

#define OPENCV_AVIF_CHECK_STATUS(X)                                       \
  {                                                                       \
    const avifResult status = (X);                                        \
    if (status != AVIF_RESULT_OK) {                                       \
      const std::string error(decoder_->diag.error);                      \
      CV_Error(Error::StsParseError, error + avifResultToString(status)); \
      return false;                                                       \
    }                                                                     \
  }

ImageDecoder AvifDecoder::newDecoder() const { return makePtr<AvifDecoder>(); }

bool AvifDecoder::readHeader() {
  OPENCV_AVIF_CHECK_STATUS(
      m_buf.empty()
          ? avifDecoderSetIOFile(decoder_, m_filename.c_str())
          : avifDecoderSetIOMemory(
                decoder_, reinterpret_cast<const uint8_t *>(m_buf.data),
                m_buf.total()));
  OPENCV_AVIF_CHECK_STATUS(avifDecoderParse(decoder_));

  m_width = decoder_->image->width;
  m_height = decoder_->image->height;
  channels_ = (decoder_->image->yuvFormat == AVIF_PIXEL_FORMAT_YUV400) ? 1 : 3;
  if (decoder_->alphaPresent) ++channels_;
  bit_depth_ = decoder_->image->depth;
  CV_Assert(bit_depth_ == 8 || bit_depth_ == 10 || bit_depth_ == 12);
  m_type = CV_MAKETYPE(bit_depth_ == 8 ? CV_8U : CV_32F, channels_);
  return true;
}

bool AvifDecoder::readData(Mat &img) {
  CV_CheckGE(m_width, 0, "");
  CV_CheckGE(m_height, 0, "");

  CV_CheckEQ(img.cols, m_width, "");
  CV_CheckEQ(img.rows, m_height, "");
  CV_CheckType(
      img.type(),
      (img.channels() == 1 || img.channels() == 3 || img.channels() == 4) &&
          (img.depth() == CV_8U || img.depth() == CV_32F),
      "AVIF only supports 1,3,4 channels and CV_8U and CV_32F");

  if (!m_buf.empty()) {
    CV_Assert(m_buf.type() == CV_8UC1);
    CV_Assert(m_buf.rows == 1);
  }

  Mat read_img;
  if (bit_depth_ > 8) {
    // For higher bit depth, a temporary is needed to store uint16_t, that
    // will then be converted to float.
    read_img.create(m_height, m_width, CV_MAKE_TYPE(CV_16U, channels_));
  } else if (img.type() != m_type) {
    read_img.create(m_height, m_width, m_type);
  } else {
    read_img = img;  // copy header
  }

  OPENCV_AVIF_CHECK_STATUS(avifDecoderNextImage(decoder_));

  if (CopyToMat(decoder_->image, channels_, &read_img) != AVIF_RESULT_OK) {
    CV_Error(Error::StsInternal, "Cannot convert from AVIF to Mat");
    return false;
  }

  if (read_img.data == img.data && img.type() == m_type) {
    // We already wrote to the right buffer.
  } else if (img.channels() == channels_) {
    if (bit_depth_ > 8) {
      read_img.convertTo(img, CV_32F, 1. / ((1 << bit_depth_) - 1));
    }
  } else {
    if (bit_depth_ > 8) {
      read_img.convertTo(read_img, CV_32F, 1. / ((1 << bit_depth_) - 1));
    }
    if (img.channels() == 1 && channels_ == 3) {
      cvtColor(read_img, img, COLOR_BGR2GRAY);
    } else if (img.channels() == 3 && channels_ == 1) {
      cvtColor(read_img, img, COLOR_GRAY2BGR);
    } else if (img.channels() == 3 && channels_ == 4) {
      cvtColor(read_img, img, COLOR_BGRA2BGR);
    } else if (img.channels() == 4 && channels_ == 3) {
      cvtColor(read_img, img, COLOR_BGR2BGRA);
    } else {
      CV_Error(Error::StsInternal, "");
    }
  }
  return true;
}

bool AvifDecoder::nextPage() { return true; }

////////////////////////////////////////////////////////////////////////////////

AvifEncoder::AvifEncoder() {
  m_description = "AVIF files (*.avif)";
  m_buf_supported = true;
  encoder_ = avifEncoderCreate();
}

AvifEncoder::~AvifEncoder() {
  if (encoder_) avifEncoderDestroy(encoder_);
}

bool AvifEncoder::isFormatSupported(int depth) const {
  return (depth == CV_8U || depth == CV_32F);
}

bool AvifEncoder::write(const Mat &img, const std::vector<int> &params) {
  std::vector<Mat> img_vec(1, img);
  return writeToOutput(img_vec, params);
}

bool AvifEncoder::writemulti(const std::vector<Mat> &img_vec,
                             const std::vector<int> &params) {
  encoder_->timescale = 10;
  return writeToOutput(img_vec, params);
}

bool AvifEncoder::writeToOutput(const std::vector<Mat> &img_vec,
                                const std::vector<int> &params) {
  int bit_depth = 8;
  int speed = AVIF_SPEED_FASTEST;
  for (size_t i = 0; i < params.size(); i += 2) {
    if (params[i] == IMWRITE_AVIF_QUALITY) {
      const int quality = std::min(std::max(params[i + 1], AVIF_QUALITY_WORST),
                                   AVIF_QUALITY_BEST);
#if CV_AVIF_USE_QUALITY
      encoder_->quality = quality;
#else
      encoder_->minQuantizer = encoder_->maxQuantizer =
          (AVIF_QUANTIZER_BEST_QUALITY - AVIF_QUANTIZER_WORST_QUALITY) *
              quality / (AVIF_QUALITY_BEST - AVIF_QUALITY_WORST) +
          AVIF_QUANTIZER_WORST_QUALITY;
#endif
    } else if (params[i] == IMWRITE_AVIF_DEPTH) {
      bit_depth = params[i + 1];
    } else if (params[i] == IMWRITE_AVIF_SPEED) {
      speed = params[i + 1];
    }
  }

  avifRWData output_ori = AVIF_DATA_EMPTY;
  std::unique_ptr<avifRWData, decltype(&avifRWDataFree)> output(&output_ori,
                                                                avifRWDataFree);
#if CV_AVIF_USE_QUALITY
  const bool do_lossless = (encoder_->quality == AVIF_QUALITY_LOSSLESS);
#else
  const bool do_lossless =
      (encoder_->minQuantizer == AVIF_QUANTIZER_BEST_QUALITY &&
       encoder_->maxQuantizer == AVIF_QUANTIZER_BEST_QUALITY);
#endif
  encoder_->speed = speed;

  const avifAddImageFlags flag = (img_vec.size() == 1)
                                     ? AVIF_ADD_IMAGE_FLAG_SINGLE
                                     : AVIF_ADD_IMAGE_FLAG_NONE;
  std::vector<AvifImageUniquePtr> images;
  std::vector<cv::Mat> imgs_scaled;
  for (const cv::Mat &img : img_vec) {
    CV_CheckType(
        CV_MAKE_TYPE(bit_depth, img.channels()),
        (bit_depth == 8 && img.depth() == CV_8U) ||
            ((bit_depth == 10 || bit_depth == 12) && img.depth() == CV_32F),
        "AVIF only supports bit depth of 8 with CV_8U input or "
        "bit depth of 10 or 12 with CV_32F input");
    CV_Check(img.channels(),
             img.channels() == 1 || img.channels() == 3 || img.channels() == 4,
             "AVIF only supports 1, 3, 4 channels");

    cv::Mat img_scaled;
    if (img.depth() == CV_32F) {
      img.convertTo(img_scaled, CV_16U, (1 << bit_depth) - 1);
    } else {
      img_scaled = img;
    }
    imgs_scaled.push_back(img_scaled);

    images.push_back(ConvertToAvif(img_scaled, do_lossless, bit_depth));
  }
  for (const AvifImageUniquePtr &image : images) {
    const avifResult status = avifEncoderAddImage(
        encoder_, image.get(), /*durationInTimescale=*/1, flag);
    if (status != AVIF_RESULT_OK) {
      const std::string error(encoder_->diag.error);
      CV_Error(Error::StsParseError, error);
      return false;
    }
  }

  const avifResult status = avifEncoderFinish(encoder_, output.get());
  if (status != AVIF_RESULT_OK) {
    const std::string error(encoder_->diag.error);
    CV_Error(Error::StsParseError, error);
    return false;
  }

  if (m_buf) {
    m_buf->resize(output->size);
    std::memcpy(m_buf->data(), output->data, output->size);
  } else {
    std::ofstream file(m_filename, std::ofstream::binary);
    file.write(reinterpret_cast<char *>(output->data), output->size);
    file.close();
  }

  return (output->size > 0);
}

ImageEncoder AvifEncoder::newEncoder() const { return makePtr<AvifEncoder>(); }

}  // namespace cv

#endif
