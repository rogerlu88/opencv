// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2021 Intel Corporation

#ifndef GAPI_STREAMING_ONVPL_PREPROC_SESSION_HPP
#define GAPI_STREAMING_ONVPL_PREPROC_SESSION_HPP
#include <stdio.h>
#include <memory>
#include <queue>

#include <opencv2/gapi/streaming/meta.hpp>
#include "streaming/onevpl/engine/engine_session.hpp"
#include "streaming/onevpl/accelerators/accel_policy_interface.hpp"
#include "streaming/onevpl/engine/preproc/vpp_preproc_defines.hpp"

#ifdef HAVE_ONEVPL

#ifdef HAVE_INF_ENGINE
#include <inference_engine.hpp>

namespace cv {
namespace gapi {
namespace wip {
namespace onevpl {
struct vpp_pp_params {
    mfxSession handle;
    mfxFrameInfo info;
};

class VPPPreprocEngine;
class PreprocSession : public EngineSession {
public:
    friend class VPPPreprocEngine;
    PreprocSession(mfxSession sess, const mfxVideoParam &vpp_out_param);
    ~PreprocSession();
    using EngineSession::EngineSession;

    Data::Meta generate_frame_meta();
    void swap_surface(VPPPreprocEngine& engine);
    void init_surface_pool(VPLAccelerationPolicy::pool_key_t key);

    virtual const mfxFrameInfo& get_video_param() const override;
private:
    mfxVideoParam mfx_vpp_out_param;
    VPLAccelerationPolicy::pool_key_t vpp_pool_id;
    mfxFrameAllocRequest request;

protected:
    std::weak_ptr<Surface> procesing_surface_ptr;
    using op_handle_t = std::pair<mfxSyncPoint, mfxFrameSurface1*>;
    std::queue<op_handle_t> sync_in_queue;
    std::queue<op_handle_t> vpp_out_queue;
    int64_t preprocessed_frames_count;
};
} // namespace onevpl
} // namespace wip
} // namespace gapi
} // namespace cv
#endif // HAVE_INF_ENGINE
#endif // HAVE_ONEVPL
#endif // GAPI_STREAMING_ONVPL_PREPROC_SESSION_HPP
