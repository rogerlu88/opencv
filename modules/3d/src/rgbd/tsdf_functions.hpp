// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html

// Partially rewritten from https://github.com/Nerei/kinfu_remake
// Copyright(c) 2012, Anatoly Baksheev. All rights reserved.

#ifndef OPENCV_3D_TSDF_FUNCTIONS_HPP
#define OPENCV_3D_TSDF_FUNCTIONS_HPP

#include "../precomp.hpp"
#include "utils.hpp"

namespace cv
{

typedef int8_t TsdfType;
typedef uchar WeightType;

struct TsdfVoxel
{
    TsdfVoxel(TsdfType _tsdf, WeightType _weight) :
        tsdf(_tsdf), weight(_weight)
    { }
    TsdfType tsdf;
    WeightType weight;
};

typedef Vec<uchar, sizeof(TsdfVoxel)> VecTsdfVoxel;

typedef short int ColorType;
struct RGBTsdfVoxel
{
    RGBTsdfVoxel(TsdfType _tsdf, WeightType _weight, ColorType _r, ColorType _g, ColorType _b) :
        tsdf(_tsdf), weight(_weight), r(_r), g(_g), b(_b)
    { }
    TsdfType tsdf;
    WeightType weight;
    ColorType r, g, b;
};

typedef Vec<uchar, sizeof(RGBTsdfVoxel)> VecRGBTsdfVoxel;

inline v_float32x4 tsdfToFloat_INTR(const v_int32x4& num)
{
    v_float32x4 num128 = v_setall_f32(-1.f / 128.f);
    return v_cvt_f32(num) * num128;
}

inline TsdfType floatToTsdf(float num)
{
    //CV_Assert(-1 < num <= 1);
    int8_t res = int8_t(num * (-128.f));
    res = res ? res : (num < 0 ? 1 : -1);
    return res;
}

inline float tsdfToFloat(TsdfType num)
{
    return float(num) * (-1.f / 128.f);
}

inline void colorFix(ColorType& r, ColorType& g, ColorType&b)
{
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
}

inline void colorFix(Point3f& c)
{
    if (c.x > 255) c.x = 255;
    if (c.y > 255) c.y = 255;
    if (c.z > 255) c.z = 255;
}

void preCalculationPixNorm(Size size, const Intr& intrinsics, Mat& pixNorm);
#ifdef HAVE_OPENCL
void ocl_preCalculationPixNorm(Size size, const Intr& intrinsics, UMat& pixNorm);
#endif

inline depthType bilinearDepth(const Depth& m, cv::Point2f pt)
{
    const bool fixMissingData = false;
    const depthType defaultValue = qnan;
    if (pt.x < 0 || pt.x >= m.cols - 1 ||
        pt.y < 0 || pt.y >= m.rows - 1)
        return defaultValue;

    int xi = cvFloor(pt.x), yi = cvFloor(pt.y);

    const depthType* row0 = m[yi + 0];
    const depthType* row1 = m[yi + 1];

    depthType v00 = row0[xi + 0];
    depthType v01 = row0[xi + 1];
    depthType v10 = row1[xi + 0];
    depthType v11 = row1[xi + 1];

    // assume correct depth is positive
    bool b00 = v00 > 0;
    bool b01 = v01 > 0;
    bool b10 = v10 > 0;
    bool b11 = v11 > 0;

    if (!fixMissingData)
    {
        if (!(b00 && b01 && b10 && b11))
            return defaultValue;
        else
        {
            float tx = pt.x - xi, ty = pt.y - yi;
            depthType v0 = v00 + tx * (v01 - v00);
            depthType v1 = v10 + tx * (v11 - v10);
            return v0 + ty * (v1 - v0);
        }
    }
    else
    {
        int nz = b00 + b01 + b10 + b11;
        if (nz == 0)
        {
            return defaultValue;
        }
        if (nz == 1)
        {
            if (b00) return v00;
            if (b01) return v01;
            if (b10) return v10;
            if (b11) return v11;
        }
        if (nz == 2)
        {
            if (b00 && b10) v01 = v00, v11 = v10;
            if (b01 && b11) v00 = v01, v10 = v11;
            if (b00 && b01) v10 = v00, v11 = v01;
            if (b10 && b11) v00 = v10, v01 = v11;
            if (b00 && b11) v01 = v10 = (v00 + v11) * 0.5f;
            if (b01 && b10) v00 = v11 = (v01 + v10) * 0.5f;
        }
        if (nz == 3)
        {
            if (!b00) v00 = v10 + v01 - v11;
            if (!b01) v01 = v00 + v11 - v10;
            if (!b10) v10 = v00 + v11 - v01;
            if (!b11) v11 = v01 + v10 - v00;
        }

        float tx = pt.x - xi, ty = pt.y - yi;
        depthType v0 = v00 + tx * (v01 - v00);
        depthType v1 = v10 + tx * (v11 - v10);
        return v0 + ty * (v1 - v0);
    }
}

void _integrateVolumeUnit(
    float truncDist, float voxelSize, int maxWeight,
    cv::Matx44f _pose, Point3i volResolution, Vec4i volStrides,
    InputArray _depth, float depthFactor, const cv::Matx44f& cameraPose,
    const cv::Intr& intrinsics, InputArray _pixNorms, InputArray _volume);

void _integrateRGBVolumeUnit(
    float truncDist, float voxelSize, int maxWeight,
    cv::Matx44f _pose, Point3i volResolution, Vec4i volStrides,
    InputArray _depth, InputArray _rgb, float depthFactor, const cv::Matx44f& cameraPose,
    const cv::Intr& depth_intrinsics, const cv::Intr& rgb_intrinsics, InputArray _pixNorms, InputArray _volume);


void integrateVolumeUnit(const VolumeSettings& settings, const Matx44f& cameraPose,
                         InputArray _depth, InputArray _pixNorms, InputArray _volume);


void raycastVolumeUnit(const VolumeSettings& settings, const Matx44f& cameraPose, int height, int width,
                       InputArray _volume, OutputArray _points, OutputArray _normals);

void fetchNormalsFromTsdfVolumeUnit(const VolumeSettings& settings, InputArray _volume,
                                    InputArray _points, OutputArray _normals);


#ifdef HAVE_OPENCL
void ocl_integrateVolumeUnit(
    const VolumeSettings& settings, const Matx44f& cameraPose,
    InputArray _depth, InputArray _pixNorms, InputArray _volume);

void ocl_raycastVolumeUnit(
    const VolumeSettings& settings, const Matx44f& cameraPose, int height, int width,
    InputArray _volume, OutputArray _points, OutputArray _normals);

void ocl_fetchNormalsFromTsdfVolumeUnit(
    const VolumeSettings& settings, InputArray _volume,
    InputArray _points, OutputArray _normals);

#endif


class CustomHashSet
{
public:
    static const int hashDivisor = 32768;
    static const int startCapacity = 2048;

    std::vector<int> hashes;
    // 0-3 for key, 4th for internal use
    // don't keep keep value
    std::vector<Vec4i> data;
    int capacity;
    int last;

    CustomHashSet()
    {
        hashes.resize(hashDivisor);
        for (int i = 0; i < hashDivisor; i++)
            hashes[i] = -1;
        capacity = startCapacity;

        data.resize(capacity);
        for (int i = 0; i < capacity; i++)
            data[i] = { 0, 0, 0, -1 };

        last = 0;
    }

    ~CustomHashSet() { }

    inline size_t calc_hash(Vec3i x) const
    {
        uint32_t seed = 0;
        constexpr uint32_t GOLDEN_RATIO = 0x9e3779b9;
        for (int i = 0; i < 3; i++)
        {
            seed ^= x[i] + GOLDEN_RATIO + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    // should work on existing elements too
    // 0 - need resize
    // 1 - idx is inserted
    // 2 - idx already exists
    int insert(Vec3i idx)
    {
        if (last < capacity)
        {
            int hash = int(calc_hash(idx) % hashDivisor);
            int place = hashes[hash];
            if (place >= 0)
            {
                int oldPlace = place;
                while (place >= 0)
                {
                    if (data[place][0] == idx[0] &&
                        data[place][1] == idx[1] &&
                        data[place][2] == idx[2])
                        return 2;
                    else
                    {
                        oldPlace = place;
                        place = data[place][3];
                        //std::cout << "place=" << place << std::endl;
                    }
                }

                // found, create here
                data[oldPlace][3] = last;
            }
            else
            {
                // insert at last
                hashes[hash] = last;
            }

            data[last][0] = idx[0];
            data[last][1] = idx[1];
            data[last][2] = idx[2];
            data[last][3] = -1;
            last++;

            return 1;
        }
        else
            return 0;
    }

    int find(Vec3i idx) const
    {
        int hash = int(calc_hash(idx) % hashDivisor);
        int place = hashes[hash];
        // search a place
        while (place >= 0)
        {
            if (data[place][0] == idx[0] &&
                data[place][1] == idx[1] &&
                data[place][2] == idx[2])
                break;
            else
            {
                place = data[place][3];
            }
        }

        return place;
    }
};

// TODO: remove this structure as soon as HashTSDFGPU data is completely on GPU;
// until then CustomHashTable can be replaced by this one if needed

const int NAN_ELEMENT = -2147483647;

struct Volume_NODE
{
    Vec4i idx = Vec4i(NAN_ELEMENT);
    int32_t row = -1;
    int32_t nextVolumeRow = -1;
    int32_t dummy = 0;
    int32_t dummy2 = 0;
};

const int _hash_divisor = 32768;
const int _list_size = 4;

class VolumesTable
{
public:
    const int hash_divisor = _hash_divisor;
    const int list_size = _list_size;
    const int32_t free_row = -1;
    const int32_t free_isActive = 0;

    const cv::Vec4i nan4 = cv::Vec4i(NAN_ELEMENT);

    int bufferNums;
    cv::Mat volumes;

    VolumesTable() : bufferNums(1)
    {
        this->volumes = cv::Mat(hash_divisor * list_size, 1, rawType<Volume_NODE>());
        for (int i = 0; i < volumes.size().height; i++)
        {
            Volume_NODE* v = volumes.ptr<Volume_NODE>(i);
            v->idx = nan4;
            v->row = -1;
            v->nextVolumeRow = -1;
        }
    }
    const VolumesTable& operator=(const VolumesTable& vt)
    {
        this->volumes = vt.volumes;
        this->bufferNums = vt.bufferNums;
        return *this;
    }
    ~VolumesTable() {};

    bool insert(Vec3i idx, int row)
    {
        CV_Assert(row >= 0);

        int bufferNum = 0;
        int hash = int(calc_hash(idx) % hash_divisor);
        int start = getPos(idx, bufferNum);
        int i = start;

        while (i >= 0)
        {
            Volume_NODE* v = volumes.ptr<Volume_NODE>(i);

            if (v->idx[0] == NAN_ELEMENT)
            {
                Vec4i idx4(idx[0], idx[1], idx[2], 0);

                bool extend = false;
                if (i != start && i % list_size == 0)
                {
                    if (bufferNum >= bufferNums - 1)
                    {
                        extend = true;
                        volumes.resize(hash_divisor * bufferNums);
                        bufferNums++;
                    }
                    bufferNum++;
                    v->nextVolumeRow = (bufferNum * hash_divisor + hash) * list_size;
                }
                else
                {
                    v->nextVolumeRow = i + 1;
                }

                v->idx = idx4;
                v->row = row;

                return extend;
            }

            i = v->nextVolumeRow;
        }
        return false;
    }
    int findRow(Vec3i idx) const
    {
        int bufferNum = 0;
        int i = getPos(idx, bufferNum);

        while (i >= 0)
        {
            const Volume_NODE* v = volumes.ptr<Volume_NODE>(i);

            if (v->idx == Vec4i(idx[0], idx[1], idx[2], 0))
                return v->row;
            else
                i = v->nextVolumeRow;
        }

        return -1;
    }

    inline int getPos(Vec3i idx, int bufferNum) const
    {
        int hash = int(calc_hash(idx) % hash_divisor);
        return (bufferNum * hash_divisor + hash) * list_size;
    }

    inline size_t calc_hash(Vec3i x) const
    {
        uint32_t seed = 0;
        constexpr uint32_t GOLDEN_RATIO = 0x9e3779b9;
        for (int i = 0; i < 3; i++)
        {
            seed ^= x[i] + GOLDEN_RATIO + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

}  // namespace cv

#endif // include guard
