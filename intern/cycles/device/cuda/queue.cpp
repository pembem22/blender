/*
 * Copyright 2011-2013 Blender Foundation
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

#include "device/cuda/queue.h"

#ifdef WITH_CUDA

#  include "device/cuda/device_cuda_impl.h"
#  include "device/cuda/kernel.h"

#  include "render/buffers.h"

CCL_NAMESPACE_BEGIN

/* CUDADeviceQueue */

CUDADeviceQueue::CUDADeviceQueue(CUDADevice *device) : DeviceQueue(device), cuda_device_(device)
{
}

void CUDADeviceQueue::init_execution()
{
  /* Synchronize all textures and memory copies before executing task. */
  CUDAContextScope scope(cuda_device_);
  cuda_device_->load_texture_info();
  cuda_device_assert(cuda_device_, cuCtxSynchronize());
}

/* CUDAIntegratorQueue */

CUDAIntegratorQueue::CUDAIntegratorQueue(CUDADevice *device, RenderBuffers *render_buffers)
    : CUDADeviceQueue(device),
      render_buffers_(render_buffers),
      integrator_state_(device, "integrator_state"),
      integrator_path_queue_(device, "integrator_path_queue", MEM_READ_WRITE),
      queued_paths_(device, "queued_paths", MEM_READ_WRITE),
      num_queued_paths_(device, "num_queued_paths", MEM_READ_WRITE),
      work_tiles_(device, "work_tiles", MEM_READ_WRITE),
      max_active_path_index_(0)
{
  integrator_state_.alloc_to_device(get_max_num_paths());
  integrator_state_.zero_to_device();

  integrator_path_queue_.alloc(1);
  integrator_path_queue_.zero_to_device();
}

void CUDAIntegratorQueue::compute_queued_paths(DeviceKernel kernel, int queued_kernel)
{
  /* Launch kernel to count the number of active paths. */
  const CUDADeviceKernel &cuda_kernel = cuda_device_->kernels.get(kernel);

  /* TODO: this could be smaller for terminated paths based on amount of work we want
   * to schedule. */
  const int work_size = (kernel == DeviceKernel::INTEGRATOR_TERMINATED_PATHS_ARRAY) ?
                            get_max_num_paths() :
                            max_active_path_index_;

  /* We perform parallel reduce per block, and then sum the results from each block on the host. */
  const int num_threads_per_block = cuda_kernel.num_threads_per_block;
  const int num_blocks = divide_up(work_size, num_threads_per_block);

  /* See parall_reduce.h for why this amount of shared memory is needed. */
  const int shared_mem_bytes = (num_threads_per_block + 1) * sizeof(int);

  if (num_queued_paths_.size() < 1) {
    num_queued_paths_.alloc(1);
  }
  if (queued_paths_.size() < work_size) {
    queued_paths_.alloc(work_size);
    queued_paths_.zero_to_device(); /* TODO: only need to allocate on device. */
  }

  /* TODO: ensure this happens as part of CUDA stream. */
  num_queued_paths_.zero_to_device();

  CUdeviceptr d_integrator_state = (CUdeviceptr)integrator_state_.device_pointer;
  CUdeviceptr d_queued_paths = (CUdeviceptr)queued_paths_.device_pointer;
  CUdeviceptr d_num_queued_paths = (CUdeviceptr)num_queued_paths_.device_pointer;
  int queued_kernel_flag = 1 << queued_kernel;
  void *args[] = {&d_integrator_state,
                  const_cast<int *>(&work_size),
                  &d_queued_paths,
                  &d_num_queued_paths,
                  &queued_kernel_flag};

  cuda_device_assert(cuda_device_,
                     cuLaunchKernel(cuda_kernel.function,
                                    num_blocks,
                                    1,
                                    1,
                                    num_threads_per_block,
                                    1,
                                    1,
                                    shared_mem_bytes,
                                    0,
                                    args,
                                    0));
}

void CUDAIntegratorQueue::enqueue(DeviceKernel kernel)
{
  if (cuda_device_->have_error()) {
    return;
  }

  const CUDAContextScope scope(cuda_device_);
  const CUDADeviceKernel &cuda_kernel = cuda_device_->kernels.get(kernel);

  CUdeviceptr d_integrator_state = (CUdeviceptr)integrator_state_.device_pointer;
  CUdeviceptr d_integrator_path_queue = (CUdeviceptr)integrator_path_queue_.device_pointer;
  CUdeviceptr d_path_index = (CUdeviceptr)NULL;

  /* Create array of path indices for which this kernel is queued to be executed. */
  int work_size = max_active_path_index_;
  IntegratorPathKernel integrator_kernel = INTEGRATOR_KERNEL_NUM;

  switch (kernel) {
    case DeviceKernel::INTEGRATOR_INTERSECT_CLOSEST:
    case DeviceKernel::INTEGRATOR_MEGAKERNEL:
      integrator_kernel = INTEGRATOR_KERNEL_intersect_closest;
      break;
    case DeviceKernel::INTEGRATOR_INTERSECT_SHADOW:
      integrator_kernel = INTEGRATOR_KERNEL_intersect_shadow;
      break;
    case DeviceKernel::INTEGRATOR_INTERSECT_SUBSURFACE:
      integrator_kernel = INTEGRATOR_KERNEL_intersect_subsurface;
      break;
    case DeviceKernel::INTEGRATOR_SHADE_BACKGROUND:
      integrator_kernel = INTEGRATOR_KERNEL_shade_background;
      break;
    case DeviceKernel::INTEGRATOR_SHADE_SHADOW:
      integrator_kernel = INTEGRATOR_KERNEL_shade_shadow;
      break;
    case DeviceKernel::INTEGRATOR_SHADE_SURFACE:
      integrator_kernel = INTEGRATOR_KERNEL_shade_surface;
      break;
    case DeviceKernel::INTEGRATOR_SHADE_VOLUME:
      integrator_kernel = INTEGRATOR_KERNEL_shade_volume;
      break;

    case DeviceKernel::INTEGRATOR_INIT_FROM_CAMERA:
    case DeviceKernel::INTEGRATOR_QUEUED_PATHS_ARRAY:
    case DeviceKernel::INTEGRATOR_QUEUED_SHADOW_PATHS_ARRAY:
    case DeviceKernel::INTEGRATOR_TERMINATED_PATHS_ARRAY:
    case DeviceKernel::NUM_KERNELS:
      break;
  }

  if (integrator_kernel != INTEGRATOR_KERNEL_NUM) {
    cuda_device_assert(cuda_device_, cuCtxSynchronize());

    integrator_path_queue_.copy_from_device();
    IntegratorPathQueue *path_queue = integrator_path_queue_.data();
    const int num_queued = path_queue->num_queued[integrator_kernel];

    if (num_queued == 0) {
      return;
    }

    if (num_queued < work_size) {
      work_size = num_queued;
      compute_queued_paths((kernel == DeviceKernel::INTEGRATOR_INTERSECT_SHADOW ||
                            kernel == DeviceKernel::INTEGRATOR_SHADE_SHADOW) ?
                               DeviceKernel::INTEGRATOR_QUEUED_SHADOW_PATHS_ARRAY :
                               DeviceKernel::INTEGRATOR_QUEUED_PATHS_ARRAY,
                           integrator_kernel);
      d_path_index = (CUdeviceptr)queued_paths_.device_pointer;
    }
  }

  /* Compute kernel launch parameters. */
  const int num_threads_per_block = cuda_kernel.num_threads_per_block;
  const int num_blocks = divide_up(work_size, num_threads_per_block);

  assert(work_size < get_max_num_paths());

  switch (kernel) {
    case DeviceKernel::INTEGRATOR_INTERSECT_CLOSEST:
    case DeviceKernel::INTEGRATOR_INTERSECT_SHADOW:
    case DeviceKernel::INTEGRATOR_INTERSECT_SUBSURFACE: {
      /* Ray intersection kernels with integrator state. */
      void *args[] = {&d_integrator_state,
                      &d_integrator_path_queue,
                      &d_path_index,
                      const_cast<int *>(&work_size)};

      cuda_device_assert(
          cuda_device_,
          cuLaunchKernel(
              cuda_kernel.function, num_blocks, 1, 1, num_threads_per_block, 1, 1, 0, 0, args, 0));
      break;
    }
    case DeviceKernel::INTEGRATOR_SHADE_BACKGROUND:
    case DeviceKernel::INTEGRATOR_SHADE_SHADOW:
    case DeviceKernel::INTEGRATOR_SHADE_SURFACE:
    case DeviceKernel::INTEGRATOR_SHADE_VOLUME:
    case DeviceKernel::INTEGRATOR_MEGAKERNEL: {
      /* Shading kernels with integrator state and render buffer. */
      CUdeviceptr d_render_buffer = (CUdeviceptr)render_buffers_->buffer.device_pointer;
      void *args[] = {&d_integrator_state,
                      &d_integrator_path_queue,
                      &d_path_index,
                      &d_render_buffer,
                      const_cast<int *>(&work_size)};

      cuda_device_assert(
          cuda_device_,
          cuLaunchKernel(
              cuda_kernel.function, num_blocks, 1, 1, num_threads_per_block, 1, 1, 0, 0, args, 0));
      break;
    }
    case DeviceKernel::INTEGRATOR_INIT_FROM_CAMERA:
    case DeviceKernel::INTEGRATOR_QUEUED_PATHS_ARRAY:
    case DeviceKernel::INTEGRATOR_QUEUED_SHADOW_PATHS_ARRAY:
    case DeviceKernel::INTEGRATOR_TERMINATED_PATHS_ARRAY:
    case DeviceKernel::NUM_KERNELS: {
      break;
    }
  }
}

void CUDAIntegratorQueue::enqueue_work_tiles(DeviceKernel kernel,
                                             const KernelWorkTile work_tiles[],
                                             const int num_work_tiles)
{
  const CUDAContextScope scope(cuda_device_);
  const CUDADeviceKernel &cuda_kernel = cuda_device_->kernels.get(kernel);

  /* Copy work tiles to device. */
  if (work_tiles_.size() < num_work_tiles) {
    work_tiles_.alloc(num_work_tiles);
  }

  for (int i = 0; i < num_work_tiles; i++) {
    KernelWorkTile &work_tile = work_tiles_.data()[i];
    work_tile = work_tiles[i];
    work_tile.buffer = render_buffers_->buffer.data();
  }

  work_tiles_.copy_to_device();

  /* TODO: consider launching a single kernel with an array of work tiles.
   * Mapping global index to the right tile with different sized tiles
   * is not trivial so not done for now. */
  CUdeviceptr d_integrator_state = (CUdeviceptr)integrator_state_.device_pointer;
  CUdeviceptr d_integrator_path_queue = (CUdeviceptr)integrator_path_queue_.device_pointer;
  CUdeviceptr d_work_tile = (CUdeviceptr)work_tiles_.device_pointer;
  CUdeviceptr d_path_index = (CUdeviceptr)NULL;

  if (max_active_path_index_ != 0) {
    compute_queued_paths(DeviceKernel::INTEGRATOR_TERMINATED_PATHS_ARRAY, 0);
    d_path_index = (CUdeviceptr)queued_paths_.device_pointer;
  }

  int num_paths = 0;

  for (int i = 0; i < num_work_tiles; i++) {
    KernelWorkTile &work_tile = work_tiles_.data()[i];

    /* Compute kernel launch parameters. */
    const int tile_work_size = work_tile.w * work_tile.h * work_tile.num_samples;
    const int num_threads_per_block = cuda_kernel.num_threads_per_block;
    const int num_blocks = divide_up(tile_work_size, num_threads_per_block);

    /* Launch kernel. */
    void *args[] = {&d_integrator_state,
                    &d_integrator_path_queue,
                    &d_path_index,
                    &d_work_tile,
                    const_cast<int *>(&tile_work_size),
                    &num_paths};

    cuda_device_assert(
        cuda_device_,
        cuLaunchKernel(
            cuda_kernel.function, num_blocks, 1, 1, num_threads_per_block, 1, 1, 0, 0, args, 0));

    /* Offset work tile and path index pointers for next tile. */
    num_paths += tile_work_size;
    assert(num_paths < get_max_num_paths());

    d_work_tile = (CUdeviceptr)(((KernelWorkTile *)d_work_tile) + 1);
    if (d_path_index) {
      d_path_index = (CUdeviceptr)(((int *)d_path_index) + tile_work_size);
    }
  }

  /* TODO: this could be computed more accurately using on the last entry
   * in the queued_paths array passed to the kernel. ?. */
  max_active_path_index_ = min(max_active_path_index_ + num_paths, get_max_num_paths());
}

int CUDAIntegratorQueue::get_num_active_paths()
{
  /* TODO: set a hard limit in case of undetected kernel failures? */
  if (cuda_device_->have_error()) {
    return 0;
  }

  const CUDAContextScope scope(cuda_device_);

  cuda_device_assert(cuda_device_, cuCtxSynchronize());

  integrator_path_queue_.copy_from_device();
  IntegratorPathQueue *path_queue = integrator_path_queue_.data();

  int num_paths = 0;
  for (int i = 0; i < INTEGRATOR_KERNEL_NUM; i++) {
    num_paths += path_queue->num_queued[i];
  }

  if (num_paths == 0) {
    max_active_path_index_ = 0;
  }

  return num_paths;
}

int CUDAIntegratorQueue::get_max_num_paths()
{
  /* TODO: compute automatically. */
  /* TODO: must have at least num_threads_per_block. */
  return 1048576;
}

CCL_NAMESPACE_END

#endif /* WITH_CUDA */
