
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

/************************************ Path State *****************************/

KERNEL_STRUCT_BEGIN(path)
/* Index of a pixel within the device render buffer where this path will write its result.
 * To get an actual offset within the buffer the value needs to be multiplied by the
 * `kernel_data.film.pass_stride`.
 *
 * The multiplication is delayed for later, so that state can use 32bit integer. */
KERNEL_STRUCT_MEMBER(uint32_t, render_pixel_index)
/* Current sample number. */
KERNEL_STRUCT_MEMBER(uint16_t, sample)
/* Current ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, bounce)
/* Current diffuse ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, diffuse_bounce)
/* Current glossy ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, glossy_bounce)
/* Current transmission ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, transmission_bounce)
/* Current volume ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, volume_bounce)
/* Current transparent ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, transparent_bounce)
/* DeviceKernel bit indicating queued kernels.
 * TODO: reduce size? */
KERNEL_STRUCT_MEMBER(uint32_t, queued_kernel)
/* Random number generator seed. */
KERNEL_STRUCT_MEMBER(uint32_t, rng_hash)
/* Random number dimension offset. */
KERNEL_STRUCT_MEMBER(uint32_t, rng_offset)
/* enum PathRayFlag */
KERNEL_STRUCT_MEMBER(uint32_t, flag)
/* Multiple importance sampling. */
KERNEL_STRUCT_MEMBER(float, mis_ray_pdf)
KERNEL_STRUCT_MEMBER(float, mis_ray_t)
/* Filter glossy. */
KERNEL_STRUCT_MEMBER(float, min_ray_pdf)
/* Throughput. */
KERNEL_STRUCT_MEMBER(SpectralColor, throughput)
/* Ratio of throughput to distinguish diffuse and glossy render passes. */
KERNEL_STRUCT_MEMBER(SpectralColor, diffuse_glossy_ratio)
/* Denoising. */
KERNEL_STRUCT_MEMBER(SpectralColor, denoising_feature_throughput)
KERNEL_STRUCT_END(path)

/************************************** Ray ***********************************/

KERNEL_STRUCT_BEGIN(ray)
KERNEL_STRUCT_MEMBER(float3, P)
KERNEL_STRUCT_MEMBER(float3, D)
KERNEL_STRUCT_MEMBER(float, t)
KERNEL_STRUCT_MEMBER(float, time)
KERNEL_STRUCT_MEMBER(float, dP)
KERNEL_STRUCT_MEMBER(float, dD)

#ifdef __SPECTRAL_RENDERING__
KERNEL_STRUCT_MEMBER(SpectralColor, wavelengths)
#endif

KERNEL_STRUCT_END(ray)

/*************************** Intersection result ******************************/

/* Result from scene intersection. */
KERNEL_STRUCT_BEGIN(isect)
KERNEL_STRUCT_MEMBER(float, t)
KERNEL_STRUCT_MEMBER(float, u)
KERNEL_STRUCT_MEMBER(float, v)
KERNEL_STRUCT_MEMBER(int, prim)
KERNEL_STRUCT_MEMBER(int, object)
KERNEL_STRUCT_MEMBER(int, type)
/* TODO: exclude for GPU. */
KERNEL_STRUCT_MEMBER(float3, Ng)
KERNEL_STRUCT_END(isect)

/*************** Subsurface closure state for subsurface kernel ***************/

KERNEL_STRUCT_BEGIN(subsurface)
KERNEL_STRUCT_MEMBER(SpectralColor, albedo)
KERNEL_STRUCT_MEMBER(SpectralColor, radius)
KERNEL_STRUCT_MEMBER(float, roughness)
KERNEL_STRUCT_END(subsurface)

/********************************** Volume Stack ******************************/

KERNEL_STRUCT_BEGIN(volume_stack)
KERNEL_STRUCT_MEMBER(int, object)
KERNEL_STRUCT_MEMBER(int, shader)
KERNEL_STRUCT_END_ARRAY(volume_stack, INTEGRATOR_VOLUME_STACK_SIZE)

/********************************* Shadow Path State **************************/

KERNEL_STRUCT_BEGIN(shadow_path)
/* Current ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, bounce)
/* Current transparent ray bounce depth. */
KERNEL_STRUCT_MEMBER(uint16_t, transparent_bounce)
/* DeviceKernel bit indicating queued kernels.
 * TODO: reduce size? */
KERNEL_STRUCT_MEMBER(uint32_t, queued_kernel)
/* enum PathRayFlag */
KERNEL_STRUCT_MEMBER(uint32_t, flag)
/* Throughput. */
KERNEL_STRUCT_MEMBER(SpectralColor, throughput)
/* Ratio of throughput to distinguish diffuse and glossy render passes. */
KERNEL_STRUCT_MEMBER(SpectralColor, diffuse_glossy_ratio)
/* Number of intersections found by ray-tracing. */
KERNEL_STRUCT_MEMBER(uint16_t, num_hits)
KERNEL_STRUCT_END(shadow_path)

/********************************** Shadow Ray *******************************/

KERNEL_STRUCT_BEGIN(shadow_ray)
KERNEL_STRUCT_MEMBER(float3, P)
KERNEL_STRUCT_MEMBER(float3, D)
KERNEL_STRUCT_MEMBER(float, t)
KERNEL_STRUCT_MEMBER(float, time)
/* TODO: compact differentials. */
KERNEL_STRUCT_END(shadow_ray)

/*********************** Shadow Intersection result **************************/

/* Result from scene intersection. */
KERNEL_STRUCT_BEGIN(shadow_isect)
KERNEL_STRUCT_MEMBER(float, t)
KERNEL_STRUCT_MEMBER(float, u)
KERNEL_STRUCT_MEMBER(float, v)
KERNEL_STRUCT_MEMBER(int, prim)
KERNEL_STRUCT_MEMBER(int, object)
KERNEL_STRUCT_MEMBER(int, type)
/* TODO: exclude for GPU. */
KERNEL_STRUCT_MEMBER(float3, Ng)
KERNEL_STRUCT_END_ARRAY(shadow_isect, INTEGRATOR_SHADOW_ISECT_SIZE)

/**************************** Shadow Volume Stack *****************************/

KERNEL_STRUCT_BEGIN(shadow_volume_stack)
KERNEL_STRUCT_MEMBER(int, object)
KERNEL_STRUCT_MEMBER(int, shader)
KERNEL_STRUCT_END_ARRAY(shadow_volume_stack, INTEGRATOR_VOLUME_STACK_SIZE)
