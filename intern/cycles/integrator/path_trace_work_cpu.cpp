/*
 * Copyright 2011-2021 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "integrator/path_trace_work_cpu.h"

#include "device/cpu/kernel.h"
#include "device/device.h"

#include "render/buffers.h"
#include "render/gpu_display.h"

#include "util/util_logging.h"
#include "util/util_tbb.h"

CCL_NAMESPACE_BEGIN

/* Create TBB arena for execution of path tracing and rendering tasks. */
static inline tbb::task_arena local_tbb_arena_create(const Device *device)
{
  /* TODO: limit this to number of threads of CPU device, it may be smaller than
   * the system number of threads when we reduce the number of CPU threads in
   * CPU + GPU rendering to dedicate some cores to handling the GPU device. */
  return tbb::task_arena(device->info.cpu_threads);
}

/* Get CPUKernelThreadGlobals for the current thread. */
static inline CPUKernelThreadGlobals *kernel_thread_globals_get(
    vector<CPUKernelThreadGlobals> &kernel_thread_globals)
{
  const int thread_index = tbb::this_task_arena::current_thread_index();
  DCHECK_GE(thread_index, 0);
  DCHECK_LE(thread_index, kernel_thread_globals.size());

  return &kernel_thread_globals[thread_index];
}

PathTraceWorkCPU::PathTraceWorkCPU(Device *device,
                                   RenderBuffers *buffers,
                                   bool *cancel_requested_flag)
    : PathTraceWork(device, buffers, cancel_requested_flag),
      kernels_(*(device->get_cpu_kernels())),
      render_buffers_(buffers)
{
  DCHECK_EQ(device->info.type, DEVICE_CPU);
}

void PathTraceWorkCPU::init_execution()
{
  /* Cache per-thread kernel globals. */
  device_->get_cpu_kernel_thread_globals(kernel_thread_globals_);
}

void PathTraceWorkCPU::render_samples(int start_sample, int samples_num)
{
  const int64_t image_width = effective_buffer_params_.width;
  const int64_t image_height = effective_buffer_params_.height;
  const int64_t total_pixels_num = image_width * image_height;

  int offset, stride;
  effective_buffer_params_.get_offset_stride(offset, stride);

  tbb::task_arena local_arena = local_tbb_arena_create(device_);
  local_arena.execute([&]() {
    tbb::parallel_for(int64_t(0), total_pixels_num, [&](int64_t work_index) {
      if (is_cancel_requested()) {
        return;
      }

      const int y = work_index / image_width;
      const int x = work_index - y * image_width;

      KernelWorkTile work_tile;
      work_tile.x = effective_buffer_params_.full_x + x;
      work_tile.y = effective_buffer_params_.full_y + y;
      work_tile.w = 1;
      work_tile.h = 1;
      work_tile.start_sample = start_sample;
      work_tile.num_samples = 1;
      work_tile.offset = offset;
      work_tile.stride = stride;

      CPUKernelThreadGlobals *kernel_globals = kernel_thread_globals_get(kernel_thread_globals_);

      render_samples_full_pipeline(kernel_globals, work_tile, samples_num);
    });
  });
}

void PathTraceWorkCPU::render_samples_full_pipeline(KernelGlobals *kernel_globals,
                                                    const KernelWorkTile &work_tile,
                                                    const int samples_num)
{
  IntegratorState integrator_state;
  IntegratorState *state = &integrator_state;

  KernelWorkTile sample_work_tile = work_tile;
  float *render_buffer = render_buffers_->buffer.data();

  for (int sample = 0; sample < samples_num; ++sample) {
    if (is_cancel_requested()) {
      break;
    }

    kernels_.integrator_init_from_camera(kernel_globals, state, &sample_work_tile, render_buffer);
    kernels_.integrator_megakernel(kernel_globals, state, render_buffer);

    ++sample_work_tile.start_sample;
  }
}

void PathTraceWorkCPU::copy_to_gpu_display(GPUDisplay *gpu_display, float sample_scale)
{
  const int full_x = effective_buffer_params_.full_x;
  const int full_y = effective_buffer_params_.full_y;
  const int width = effective_buffer_params_.width;
  const int height = effective_buffer_params_.height;

  half4 *rgba_half = gpu_display->map_texture_buffer();
  if (!rgba_half) {
    /* TODO(sergey): Look into using copy_to_gpu_display() if mapping failed. Might be needed for
     * some implementations of GPUDisplay which can not map memory? */
    return;
  }

  int offset, stride;
  effective_buffer_params_.get_offset_stride(offset, stride);

  tbb::task_arena local_arena = local_tbb_arena_create(device_);
  local_arena.execute([&]() {
    tbb::parallel_for(0, height, [&](int y) {
      for (int x = 0; x < width; ++x) {
        CPUKernelThreadGlobals *kernel_globals = kernel_thread_globals_get(kernel_thread_globals_);

        kernels_.convert_to_half_float(kernel_globals,
                                       reinterpret_cast<uchar4 *>(rgba_half),
                                       reinterpret_cast<float *>(buffers_->buffer.device_pointer),
                                       sample_scale,
                                       full_x + x,
                                       full_y + y,
                                       offset,
                                       stride);
      }
    });
  });

  gpu_display->unmap_texture_buffer();
}

bool PathTraceWorkCPU::adaptive_sampling_filter()
{
  const int full_x = effective_buffer_params_.full_x;
  const int full_y = effective_buffer_params_.full_y;
  const int width = effective_buffer_params_.width;
  const int height = effective_buffer_params_.height;

  /* NOTE: This call is supposed to happen outside of any path tracing, so can pick any of the
   * pre-configured kernel globals. */
  KernelGlobals *kernel_globals = &kernel_thread_globals_[0];

  float *render_buffer = render_buffers_->buffer.data();

  int offset, stride;
  effective_buffer_params_.get_offset_stride(offset, stride);

  bool any = false;

  /* TODO(sergey): Use parallel_for. */

  for (int y = full_y; y < full_y + height; ++y) {
    any |= kernels_.adaptive_sampling_filter_x(
        kernel_globals, render_buffer, y, full_x, width, offset, stride);
  }

  for (int x = full_x; x < full_x + width; ++x) {
    any |= kernels_.adaptive_sampling_filter_y(
        kernel_globals, render_buffer, x, full_y, height, offset, stride);
  }

  return !any;
}

CCL_NAMESPACE_END
