// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2019 Intel Corporation

#ifndef OPENCV_GAPI_GSTREAMING_EXECUTOR_HPP
#define OPENCV_GAPI_GSTREAMING_EXECUTOR_HPP

#include <memory> // unique_ptr, shared_ptr
#include <thread> // thread

#if defined(HAVE_TBB)
#  include <tbb/concurrent_queue.h> // FIXME: drop it from here!
template<typename T> using QueueClass = tbb::concurrent_bounded_queue<T>;
#else
#  include "executor/conc_queue.hpp"
template<typename T> using QueueClass = cv::gapi::own::concurrent_bounded_queue<T>;
#endif // TBB

#include <ade/graph.hpp>

#include "backends/common/gbackend.hpp"

namespace cv {
namespace gimpl {

namespace stream {
struct Start {};
struct Stop {};

using Cmd = cv::util::variant
    < Start        // Tells emitters to start working. Not broadcasted to workers.
    , Stop         // Tells emitters to stop working. Broadcasted to workers.
    , cv::GRunArg  // Workers data payload to process.
    , cv::GRunArgs // Full results vector
    >;
using Q = QueueClass<Cmd>;
} // namespace stream

// FIXME: Currently all GExecutor comments apply also
// to this one. Please document it separately in the future.

class GStreamingExecutor
{
protected:
    enum class State {
        STOPPED,
        RUNNING,
    } state = State::STOPPED;

    std::unique_ptr<ade::Graph> m_orig_graph;
    std::shared_ptr<ade::Graph> m_island_graph;

    cv::gimpl::GModel::Graph       m_gm;  // FIXME: make const?
    cv::gimpl::GIslandModel::Graph m_gim; // FIXME: make const?

    // FIXME: Naive executor details are here for now
    // but then it should be moved to another place
    struct OpDesc
    {
        std::vector<RcDesc> in_objects;
        std::vector<RcDesc> out_objects;
        cv::GMetaArgs       out_metas;
        ade::NodeHandle     nh;

        // FIXME: remove it as unused
        std::shared_ptr<GIslandExecutable> isl_exec;
    };
    std::vector<OpDesc> m_ops;

    struct DataDesc
    {
        ade::NodeHandle slot_nh;
        ade::NodeHandle data_nh;
    };
    std::vector<DataDesc> m_slots;

    // Order in these vectors follows the GComputaion's protocol
    std::vector<ade::NodeHandle> m_emitters;
    std::vector<ade::NodeHandle> m_sinks;

    std::vector<std::thread> m_threads;
    std::vector<stream::Q>   m_emitter_queues;
    std::vector<stream::Q*>  m_sink_queues;
    stream::Q m_out_queue;

public:
    explicit GStreamingExecutor(std::unique_ptr<ade::Graph> &&g_model);
    void setSource(GRunArgs &&args);
    void start();
    bool pull(cv::GRunArgsP &&outs);
    bool try_pull(cv::GRunArgsP &&outs);
    void stop();
    bool running() const;
};

} // namespace gimpl
} // namespace cv

#endif // OPENCV_GAPI_GSTREAMING_EXECUTOR_HPP
