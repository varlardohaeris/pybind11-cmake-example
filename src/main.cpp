#include "BS_thread_pool.hpp"
#include <future>
#include <iostream>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <thread>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

std::vector<AVFrame *> read_frame() {
  // std::vector<AVFrame *> read_frame(const char *filename) {
  const char *filename = "/Users/been/cmake_example/test.mp4";
  AVFormatContext *format_ctx = nullptr;

  // 打开视频文件
  if (avformat_open_input(&format_ctx, filename, nullptr, nullptr) != 0) {
    std::cerr << "Cannot open file" << std::endl;
    return {};
  }

  // 获取视频流信息
  if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
    std::cerr << "Cannot find stream info" << std::endl;
    avformat_close_input(&format_ctx);
    return {};
  }

  // 查找视频流
  int video_stream_index = -1;
  for (unsigned i = 0; i < format_ctx->nb_streams; i++) {
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
      break;
    }
  }

  if (video_stream_index == -1) {
    std::cerr << "Cannot find video stream" << std::endl;
    avformat_close_input(&format_ctx);
    return {};
  }

  // 获取视频流的编码信息
  AVCodecParameters *codecpar =
      format_ctx->streams[video_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (!codec) {
    std::cerr << "Cannot find decoder" << std::endl;
    avformat_close_input(&format_ctx);
    return {};
  }

  AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(codec_ctx, codecpar);

  // 打开解码器
  if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
    std::cerr << "Cannot open codec" << std::endl;
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return {};
  }

  int64_t totalFrames = format_ctx->streams[video_stream_index]->nb_frames;

  if (totalFrames <= 0) {
    std::cerr << "Unable to determine the total number of frames." << std::endl;
    return {};
  }
  std::cout << "Total frames in the video: " << totalFrames << std::endl;

  int interval = totalFrames / 8;
  AVFrame *frame = av_frame_alloc();
  AVPacket *packet = av_packet_alloc();
  int frame_count = 0;

  struct SwsContext *sws_ctx =
      sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                     codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
                     SWS_BICUBIC, nullptr, nullptr, nullptr);
  std::vector<AVFrame *> frames;

  // 读取并解码帧
  while (av_read_frame(format_ctx, packet) >= 0) {
    if (packet->stream_index == video_stream_index) {
      if (avcodec_send_packet(codec_ctx, packet) == 0) {
        while (avcodec_receive_frame(codec_ctx, frame) == 0) {
          frame_count++;
          if (frame_count % interval == 0) {

            // 如果帧的格式是 yuv420p，则转换为 RGB24
            if (frame->format == AV_PIX_FMT_YUV420P) {

              AVFrame *rgb_frame = av_frame_alloc();
              rgb_frame->format = AV_PIX_FMT_RGB24;
              rgb_frame->width = frame->width;
              rgb_frame->height = frame->height;
              av_image_alloc(rgb_frame->data, rgb_frame->linesize, frame->width,
                             frame->height, AV_PIX_FMT_RGB24, 1);

              // 转换 yuv420p 到 rgb24
              sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
                        rgb_frame->data, rgb_frame->linesize);
              frames.push_back(rgb_frame);
              // numpyFrames.push_back(npArray);
              // 释放资源
              // av_freep(&rgb_frame->data[0]);
              // av_frame_free(&rgb_frame);
            } else {
              std::cerr << "Frame is not in YUV420P format" << std::endl;
            }

            break;
          }
        }
      }
    }
    av_packet_unref(packet);
  }

  // 释放资源
  // av_frame_free(&frame);
  // av_packet_free(&packet);
  // avcodec_free_context(&codec_ctx);
  // avformat_close_input(&format_ctx);
  // if (sws_ctx) {
  //   sws_freeContext(sws_ctx);
  // }
  return frames;
}

int add(int i, int j) {
  std::thread::id id = std::this_thread::get_id();
  std::cout << "Thread id " << id << std::endl;
  return i + j;
}

std::vector<std::vector<py::array_t<uint8_t>>> multi_thread_loader() {
  std::vector<std::future<std::vector<AVFrame *>>> results;
  int bs = std::atoi(std::getenv("BATCH_SIZE"));
  for (int i = 0; i < bs; ++i) {
    results.push_back(std::async(std::launch::async, read_frame));
  }

  std::vector<std::vector<py::array_t<uint8_t>>> output;

  for (int i = 0; i < bs; i++) {
    std::vector<AVFrame *> frames = results[i].get();
    std::vector<py::array_t<uint8_t>> data(frames.size());
    for (int j = 0; j < frames.size(); j++) {
      AVFrame *rgb_frame = frames[j];

      py::array_t<uint8_t> raw_data(
          {rgb_frame->height, rgb_frame->width, 3}, // shape
          rgb_frame->data[0]                        // pointer to the frame data
      );
      data.push_back(raw_data);
    }
  }

  return output;
}

std::vector<int> multi_thread(int input) {
  // Constructs a thread pool with as many threads as available in the hardware.
  BS::thread_pool pool;
  std::vector<std::future<int>> results;

  for (int i = 0; i < 4; ++i) {
    results.emplace_back(
        pool.submit_task([input, i]() -> int { return add(input, i); }));
  }

  std::vector<int> output;
  for (auto &&result : results)
    output.emplace_back(result.get());

  return output;
}

py::array_t<uint8_t> create_numpy_array() {
  std::vector<size_t> shape = {3, 4, 5};
  size_t total = 1;
  for (auto s : shape)
    total *= s;
  std::vector<uint8_t> data(total, 1);
  return py::array_t<uint8_t>(shape, data.data());
}

PYBIND11_MODULE(cmake_example, m) {
  m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: cmake_example

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

  m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");
  m.def("create_numpy_array", &create_numpy_array,
        "Create a (3,4,5) NumPy array");

  m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");
  m.def("multi_thread", &multi_thread,
        "Submit four add function calls to the thread pool");

  m.def("multi_thread_loader", &multi_thread_loader, "multi_thread_loader");
#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}
