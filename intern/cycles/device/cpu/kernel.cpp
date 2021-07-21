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

#include "device/cpu/kernel.h"

#include "kernel/device/cpu/kernel_rgb.h"
#include "kernel/device/cpu/kernel_spectral.h"

CCL_NAMESPACE_BEGIN

CPUKernels::CPUKernels(
    IntegratorInitFunction integrator_init_from_camera,
    IntegratorInitFunction integrator_init_from_bake,
    IntegratorFunction integrator_intersect_closest,
    IntegratorFunction integrator_intersect_shadow,
    IntegratorFunction integrator_intersect_subsurface,
    IntegratorFunction integrator_intersect_volume_stack,
    IntegratorShadeFunction integrator_shade_background,
    IntegratorShadeFunction integrator_shade_light,
    IntegratorShadeFunction integrator_shade_shadow,
    IntegratorShadeFunction integrator_shade_surface,
    IntegratorShadeFunction integrator_shade_volume,
    IntegratorShadeFunction integrator_megakernel,
    ShaderEvalFunction shader_eval_displace,
    ShaderEvalFunction shader_eval_background,
    AdaptiveSamplingConvergenceCheckFunction adaptive_sampling_convergence_check,
    AdaptiveSamplingFilterXFunction adaptive_sampling_filter_x,
    AdaptiveSamplingFilterYFunction adaptive_sampling_filter_y,
    CryptomattePostprocessFunction cryptomatte_postprocess,
    BakeFunction bake)
    : integrator_init_from_camera(integrator_init_from_camera),
      integrator_init_from_bake(integrator_init_from_bake),
      integrator_intersect_closest(integrator_intersect_closest),
      integrator_intersect_shadow(integrator_intersect_shadow),
      integrator_intersect_subsurface(integrator_intersect_subsurface),
      integrator_intersect_volume_stack(integrator_intersect_volume_stack),
      integrator_shade_background(integrator_shade_background),
      integrator_shade_light(integrator_shade_light),
      integrator_shade_shadow(integrator_shade_shadow),
      integrator_shade_surface(integrator_shade_surface),
      integrator_shade_volume(integrator_shade_volume),
      integrator_megakernel(integrator_megakernel),
      shader_eval_displace(shader_eval_displace),
      shader_eval_background(shader_eval_background),
      adaptive_sampling_convergence_check(adaptive_sampling_convergence_check),
      adaptive_sampling_filter_x(adaptive_sampling_filter_x),
      adaptive_sampling_filter_y(adaptive_sampling_filter_y),
      cryptomatte_postprocess(cryptomatte_postprocess),
      bake(bake)
{
}

#define KERNEL_FUNCTIONS(name) \
  KERNEL_NAME_EVAL(cpu, name, KERNEL_TYPE), KERNEL_NAME_EVAL(cpu_sse2, name, KERNEL_TYPE), \
      KERNEL_NAME_EVAL(cpu_sse3, name, KERNEL_TYPE), \
      KERNEL_NAME_EVAL(cpu_sse41, name, KERNEL_TYPE), \
      KERNEL_NAME_EVAL(cpu_avx, name, KERNEL_TYPE), KERNEL_NAME_EVAL(cpu_avx2, name, KERNEL_TYPE)

#define REGISTER_KERNEL(name) \
  { \
    KERNEL_FUNCTIONS(name) \
  }

#define KERNEL_FUNCTIONS_TYPELESS(name) \
  KERNEL_NAME_EVAL_TYPELESS(cpu, name), KERNEL_NAME_EVAL_TYPELESS(cpu_sse2, name), \
      KERNEL_NAME_EVAL_TYPELESS(cpu_sse3, name), KERNEL_NAME_EVAL_TYPELESS(cpu_sse41, name), \
      KERNEL_NAME_EVAL_TYPELESS(cpu_avx, name), KERNEL_NAME_EVAL_TYPELESS(cpu_avx2, name)

#define REGISTER_KERNEL_TYPELESS(name) \
  { \
    KERNEL_FUNCTIONS_TYPELESS(name) \
  }

#define REGISTER_KERNELS(kernels_name) \
  kernels_name::kernels_name() \
      : CPUKernels(/* Integrator. */ \
                   REGISTER_KERNEL(integrator_init_from_camera), \
                   REGISTER_KERNEL(integrator_init_from_bake), \
                   REGISTER_KERNEL(integrator_intersect_closest), \
                   REGISTER_KERNEL(integrator_intersect_shadow), \
                   REGISTER_KERNEL(integrator_intersect_subsurface), \
                   REGISTER_KERNEL(integrator_intersect_volume_stack), \
                   REGISTER_KERNEL(integrator_shade_background), \
                   REGISTER_KERNEL(integrator_shade_light), \
                   REGISTER_KERNEL(integrator_shade_shadow), \
                   REGISTER_KERNEL(integrator_shade_surface), \
                   REGISTER_KERNEL(integrator_shade_volume), \
                   REGISTER_KERNEL(integrator_megakernel), /* Shader evaluation. */ \
                   REGISTER_KERNEL(shader_eval_displace), \
                   REGISTER_KERNEL(shader_eval_background), /* Adaptive sampling. */ \
                   REGISTER_KERNEL(adaptive_sampling_convergence_check), \
                   REGISTER_KERNEL(adaptive_sampling_filter_x), \
                   REGISTER_KERNEL(adaptive_sampling_filter_y), /* Bake. */ \
                   REGISTER_KERNEL(cryptomatte_postprocess),    /* Cryptomatte. */ \
                   REGISTER_KERNEL(bake)) \
  { \
  }

#define KERNEL_TYPE rgb
REGISTER_KERNELS(CPUKernelsRGB);
#undef KERNEL_TYPE

#define KERNEL_TYPE spectral
REGISTER_KERNELS(CPUKernelsSpectral);
#undef KERNEL_TYPE

#undef REGISTER_KERNEL
#undef KERNEL_FUNCTIONS

CCL_NAMESPACE_END
