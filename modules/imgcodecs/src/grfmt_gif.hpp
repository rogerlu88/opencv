// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level
// directory of this distribution and at http://opencv.org/license.html

#ifndef OPENCV_GRFMT_GIF_HPP
#define OPENCV_GRFMT_GIF_HPP
#ifdef HAVE_IMGCODEC_GIF

#include "grfmt_base.hpp"

namespace cv
{

enum GifOpMode
{
    GRFMT_GIF_Nothing = 0,
    GRFMT_GIF_PreviousImage = 1,
    GRFMT_GIF_Background = 2,
    GRFMT_GIF_Cover = 3
};

//////////////////////////////////////////////////////////////////////
////                        GIF Decoder                           ////
//////////////////////////////////////////////////////////////////////
struct lzwNodeD
{
    int length;
    std::vector<uchar> prefix;
    uchar suffix;
};

class GifDecoder CV_FINAL : public BaseImageDecoder
{
public:
    GifDecoder();
    ~GifDecoder() CV_OVERRIDE;

    bool readHeader() CV_OVERRIDE;
    bool readData(Mat& img) CV_OVERRIDE;
    bool nextPage() CV_OVERRIDE;
    void close();

    ImageDecoder newDecoder() const CV_OVERRIDE;

protected:
    RLByteStream        m_strm;

    int                 bgColor;
    int                 depth;

    GifOpMode           opMode;
    bool                hasTransparentColor;
    uchar               transparentColor;
    int                 top, left, width, height;

    bool                hasRead;
    AutoBuffer<uchar>   globalColorTable;
    AutoBuffer<uchar>   localColorTable;

    int                 lzwMinCodeSize;
    int                 globalColorTableSize;
    int                 localColorTableSize;

    Mat                 lastImage;
    AutoBuffer<uchar>   currentImageCodeStream;

    void readExtensions();
    void code2pixel(Mat& img, int start, int k);
    bool lzwDecode();
};



//////////////////////////////////////////////////////////////////////
////                        GIF Encoder                           ////
//////////////////////////////////////////////////////////////////////
class GifEncoder CV_FINAL : public BaseImageEncoder {
public:
    GifEncoder();
    ~GifEncoder() CV_OVERRIDE;

    bool isFormatSupported(int depth) const CV_OVERRIDE;

    bool write(const Mat& img, const std::vector<int>& params) CV_OVERRIDE;

    bool writemulti(const std::vector<Mat>& img_vec,
                    const std::vector<int>& params) CV_OVERRIDE;

    ImageEncoder newEncoder() const CV_OVERRIDE;

private:
/**  Color Quantization  **/
    class OctreeColorQuant
    {
        struct OctreeNode
        {
            bool  isLeaf;
            std::shared_ptr<OctreeNode> children[8]{};
            int   level;
            uchar index;
            int   leaf;
            int   pixelCount;
            size_t redSum, greenSum, blueSum;

            OctreeNode();
        };

        std::shared_ptr<OctreeNode> root;
        std::vector<std::shared_ptr<OctreeNode>> m_nodeList[8];
        int32_t m_bitLength;
        int32_t m_maxColors;
        int32_t m_leafCount;
        uchar   m_criticalTransparency;
        uchar   r, g, b; // color under transparent color

    public:
        explicit OctreeColorQuant(int maxColors = 256, int bitLength = 8, uchar criticalTransparency = 1);

        int   getPalette(uchar* colorTable);
        uchar getLeaf(uchar red, uchar green, uchar blue);

        void  addMat(const Mat& img);
        void  addMats(const std::vector<Mat>& img_vec);
        void  addColor(int red, int green, int blue);
        void  reduceTree();
        void  recurseReduce(const std::shared_ptr<OctreeNode>& node);
    };

    WLByteStream    strm;
    int             m_width, m_height;

    int             globalColorTableSize;
    int             localColorTableSize;

    uchar           opMode;
    uchar           criticalTransparency;
    uchar           transparentColor;
    int             top, left, width, height;

    OctreeColorQuant quantG;
    OctreeColorQuant quantL;

    AutoBuffer<int>   lzwTable;
    AutoBuffer<uchar> imgCodeStream;

    AutoBuffer<uchar> globalColorTable;
    AutoBuffer<uchar> localColorTable;

    // params
    int             loopCount;
    int             frameDelay;
    int             colorNum;
    int             bitDepth;
    int             dithering;
    int             lzwMinCodeSize, lzwMaxCodeSize;

    bool writeFrames(std::vector<Mat>& img_vec, const std::vector<int>& params);
    bool writeHeader(const std::vector<Mat>& img_vec);
    bool writeFrame(const Mat& img);
    bool pixel2code(const Mat& img);
    void getColorTable(const std::vector<Mat>& img_vec, bool isGlobal);
    static void ditheringKernel(Mat &img, int depth, uchar transparency);
    bool lzwEncode();
    void close();
};


} // namespace cv

#endif // HAVE_IMGCODEC_GIF
#endif //OPENCV_GRFMT_GIF_HPP
