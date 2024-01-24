/**
 * @file yolo_detector.cpp
 * @brief Yolo Object Detection Sample
 * @author OpenCV team
 */

//![includes]
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <fstream>
#include <sstream>
#include "iostream"
#include "common.hpp"
#include <opencv2/highgui.hpp>
//![includes]

using namespace cv;
using namespace cv::dnn;

void getClasses(std::string classesFile);
void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);
void yoloPostProcessing(
    std::vector<Mat>& outs,
    std::vector<int>& keep_classIds,
    std::vector<float>& keep_confidences,
    std::vector<Rect2d>& keep_boxes,
    float conf_threshold,
    float iou_threshold,
    const std::string& test_name
);

std::vector<std::string> classes;


std::string keys =
    "{ help  h     | | Print help message. }"
    "{ @alias      | | An alias name of model to extract preprocessing parameters from models.yml file. }"
    "{ zoo         | models.yml | An optional path to file with preprocessing parameters }"
    "{ device      |  0 | camera device number. }"
    "{ input i     | | Path to input image or video file. Skip this argument to capture frames from a camera. }"
    "{ classes     | | Optional path to a text file with names of classes to label detected objects. }"
    "{ thr         | .5 | Confidence threshold. }"
    "{ nms         | .4 | Non-maximum suppression threshold. }"
    "{ mean        | 0.0 | Normalization constant. }"
    "{ scale       | 1.0 | Normalization scalor. }"
    "{ yolo        | None | yolo model version. }"
    "{ padvalue    | 114.0 | padding value. }"
    "{ paddingmode |  2 | Choose one of computation backends: "
                         "0: resize to required input size without extra processing, "
                         "1: Image will be cropped after resize, "
                         "2: Resize image to the desired size while preserving the aspect ratio of original image }"
    "{ backend     |  0 | Choose one of computation backends: "
                         "0: automatically (by default), "
                         "1: Halide language (http://halide-lang.org/), "
                         "2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit), "
                         "3: OpenCV implementation, "
                         "4: VKCOM, "
                         "5: CUDA }"
    "{ target      | 0 | Choose one of target computation devices: "
                         "0: CPU target (by default), "
                         "1: OpenCL, "
                         "2: OpenCL fp16 (half-float precision), "
                         "3: VPU, "
                         "4: Vulkan, "
                         "6: CUDA, "
                         "7: CUDA fp16 (half-float preprocess) }"
    "{ async       | 0 | Number of asynchronous forwards at the same time. "
                        "Choose 0 for synchronous mode }";

void getClasses(std::string classesFile){
    std::ifstream ifs(classesFile.c_str());
    if (!ifs.is_open())
        CV_Error(Error::StsError, "File " + classesFile  + " not found");
    std::string line;
    while (std::getline(ifs, line))
        classes.push_back(line);
}

void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));

    std::string label = format("%.2f", conf);
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ": " + label;
    }

    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - labelSize.height),
              Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.5, Scalar());
}

void yoloPostProcessing(
    std::vector<Mat>& outs,
    std::vector<int>& keep_classIds,
    std::vector<float>& keep_confidences,
    std::vector<Rect2d>& keep_boxes,
    float conf_threshold,
    float iou_threshold,
    const std::string& test_name
){

    // Retrieve
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<Rect2d> boxes;

    if (test_name == "yolov8"){
        cv::transposeND(outs[0], {0, 2, 1}, outs[0]);
    }

    // each row is [cx, cy, w, h, conf_obj, conf_class1, ..., conf_class80]
    for (auto preds : outs){

        preds = preds.reshape(1, preds.size[1]); // [1, 8400, 85] -> [8400, 85]

        for (int i = 0; i < preds.rows; ++i)
        {
            // filter out non objects
            float obj_conf = (test_name != "yolov8") ? preds.at<float>(i, 4) : 1.0f;
            if (obj_conf < conf_threshold)
                continue;

            Mat scores = preds.row(i).colRange((test_name != "yolov8") ? 5 : 4, preds.cols);
            double conf;
            Point maxLoc;
            minMaxLoc(scores, 0, &conf, 0, &maxLoc);

            conf = (test_name != "yolov8") ? conf * obj_conf : conf;
            if (conf < conf_threshold)
                continue;

            // get bbox coords
            float* det = preds.ptr<float>(i);
            double cx = det[0];
            double cy = det[1];
            double w = det[2];
            double h = det[3];

            // [x1, y1, x2, y2]
            boxes.push_back(Rect2d(cx - 0.5 * w, cy - 0.5 * h,
                                    cx + 0.5 * w, cy + 0.5 * h));
            classIds.push_back(maxLoc.x);
            confidences.push_back(conf);
        }
    }

    // NMS
    std::vector<int> keep_idx;
    NMSBoxes(boxes, confidences, conf_threshold, iou_threshold, keep_idx);

    for (auto i : keep_idx)
    {
        keep_classIds.push_back(classIds[i]);
        keep_confidences.push_back(confidences[i]);
        keep_boxes.push_back(boxes[i]);
    }
}

/**
 * @function main
 * @brief Main function
 */
int main(int argc, char** argv){

    CommandLineParser parser(argc, argv, keys);

    const std::string modelName = parser.get<String>("@alias");
    const std::string zooFile = parser.get<String>("zoo");

    keys += genPreprocArguments(modelName, zooFile);

    parser = CommandLineParser(argc, argv, keys);
    parser.about("Use this script to run object detection deep learning networks using OpenCV.");
    if (argc == 1 || parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    float confThreshold = parser.get<float>("thr");
    float nmsThreshold = parser.get<float>("nms");
    float paddingValue = parser.get<int>("padvalue");
    bool swapRB = parser.get<bool>("rgb");
    int inpWidth = parser.get<int>("width");
    int inpHeight = parser.get<int>("height");
    Scalar scale = parser.get<float>("scale");
    Scalar mean = parser.get<Scalar>("mean");
    ImagePaddingMode paddingMode = static_cast<ImagePaddingMode>(parser.get<int>("paddingmode"));

    CV_Assert(parser.has("model"));
    CV_Assert(parser.has("yolo"));
    std::string weightPath = findFile(parser.get<String>("model"));
    std::string yolo_model = parser.get<String>("yolo");

    // check if yolo model is valid
    if (yolo_model != "yolov5" && yolo_model != "yolov6"
        && yolo_model != "yolov7" && yolo_model != "yolov8" && yolo_model != "yolox")
        CV_Error(Error::StsError, "Invalid yolo model: " + yolo_model);

    // get classes
    if (parser.has("classes"))
        getClasses(parser.get<String>("classes"));

    // load model
    //![read_net]
    Net net = readNet(weightPath);
    int backend = parser.get<int>("backend");
    net.setPreferableBackend(backend);
    net.setPreferableTarget(parser.get<int>("target"));
    //![read_net]

    VideoCapture cap;
    Mat img;
    bool isImage = false;
    bool isCamera = false;

    // Check if input is given
    if (parser.has("input")) {
        String input = parser.get<String>("input");
        // Check if the input is an image
        if (input.find(".jpg") != String::npos || input.find(".png") != String::npos) {
            img = imread(input);
            if (img.empty()) {
                CV_Error(Error::StsError, "Cannot read image file: " + input);
            }
            isImage = true;
        } else {
            cap.open(input);
        }
    } else {
        cap.open(parser.get<int>("device"));
    }

    // image pre-processing
    Size size(inpWidth, inpHeight);
    Image2BlobParams imgParams(
        scale,
        size,
        mean,
        swapRB,
        CV_32F,
        DNN_LAYOUT_NCHW,
        paddingMode,
        paddingValue);

    // rescale boxes back to original image
    Image2BlobParams paramNet;
            paramNet.scalefactor = scale;
            paramNet.size = size;
            paramNet.mean = mean;
            paramNet.swapRB = swapRB;
            paramNet.paddingmode = paddingMode;

    std::vector<Mat> outs;
    std::vector<int> keep_classIds;
    std::vector<float> keep_confidences;
    std::vector<Rect2d> keep_boxes;
    std::vector<Rect> boxes;

    Mat inp;
    while (waitKey(1) < 0)
    {

        if (isCamera)
            cap >> img;
        if (img.empty())
        {
            std::cout << "Empty frame" << std::endl;
            waitKey();
            break;
        }

        inp = blobFromImageWithParams(img, imgParams);

        net.setInput(inp);
        net.forward(outs, net.getUnconnectedOutLayersNames());

        yoloPostProcessing(
            outs, keep_classIds, keep_confidences, keep_boxes,
            confThreshold, nmsThreshold,
            yolo_model);

        for (auto box : keep_boxes){
            std::cout << box.x << " " << box.y << " " << box.width << " " << box.height << std::endl;
        }

        // covert Rect2d to Rect
        for (auto box : keep_boxes){
            boxes.push_back(Rect(box.x, box.y, box.width, box.height));
        }

        paramNet.blobRectsToImageRects(boxes, boxes, img.size());

        for (size_t idx = 0; idx < boxes.size(); ++idx)
        {
            Rect box = boxes[idx];
            drawPred(keep_classIds[idx], keep_confidences[idx], box.x, box.y,
                    box.width, box.height, img);
        }

        static const std::string kWinName = "Yolo objecte detection in OpenCV";
        namedWindow(kWinName, WINDOW_NORMAL);
        imshow(kWinName, img);

        outs.clear();
        keep_classIds.clear();
        keep_confidences.clear();
        keep_boxes.clear();
        boxes.clear();

        if (isImage){
            waitKey();
            break;
        }
    }
}