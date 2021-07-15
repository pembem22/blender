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

#pragma once

#include "kernel_adaptive_sampling.h"
#include "kernel_color.h"
#include "kernel_random.h"
#include "kernel_shadow_catcher.h"
#include "kernel_write_passes.h"

CCL_NAMESPACE_BEGIN

/* --------------------------------------------------------------------
 * BSDF Evaluation
 *
 * BSDF evaluation result, split between diffuse and glossy. This is used to
 * accumulate render passes separately. Note that reflection, transmission
 * and volume scattering are written to different render passes, but we assume
 * that only one of those can happen at a bounce, and so do not need to accumulate
 * them separately. */

ccl_device_inline void bsdf_eval_init(BsdfEval *eval,
                                      const bool is_diffuse,
                                      SpectralColor value,
                                      const bool use_light_pass)
{
  eval->diffuse = zero_spectral_color();
  eval->glossy = zero_spectral_color();

  if (is_diffuse) {
    eval->diffuse = value;
  }
  else {
    eval->glossy = value;
  }
}

ccl_device_inline void bsdf_eval_accum(BsdfEval *eval,
                                       const bool is_diffuse,
                                       SpectralColor value,
                                       float mis_weight)
{
  value *= mis_weight;

  if (is_diffuse) {
    eval->diffuse += value;
  }
  else {
    eval->glossy += value;
  }
}

ccl_device_inline bool bsdf_eval_is_zero(BsdfEval *eval)
{
  return is_zero(eval->diffuse) && is_zero(eval->glossy);
}

ccl_device_inline void bsdf_eval_mul(BsdfEval *eval, float value)
{
  eval->diffuse *= value;
  eval->glossy *= value;
}

ccl_device_inline void bsdf_eval_mul(BsdfEval *eval, SpectralColor value)
{
  eval->diffuse *= value;
  eval->glossy *= value;
}

ccl_device_inline SpectralColor bsdf_eval_sum(const BsdfEval *eval)
{
  return eval->diffuse + eval->glossy;
}

ccl_device_inline SpectralColor bsdf_eval_diffuse_glossy_ratio(const BsdfEval *eval)
{
  /* Ratio of diffuse and glossy to recover proportions for writing to render pass.
   * We assume reflection, transmission and volume scatter to be exclusive. */
  return safe_divide(eval->diffuse, eval->diffuse + eval->glossy);
}

/* --------------------------------------------------------------------
 * Clamping
 *
 * Clamping is done on a per-contribution basis so that we can write directly
 * to render buffers instead of using per-thread memory, and to avoid the
 * impact of clamping on other contributions. */

ccl_device_forceinline void kernel_accum_clamp(const KernelGlobals *kg,
                                               SpectralColor *L,
                                               int bounce)
{
  /* Make sure all components are finite, allowing the contribution to be usable by adaptive
   * sampling convergence check, but also to make it so render result never causes issues with
   * post-processing. */
  *L = ensure_finite(*L);

#ifdef __CLAMP_SAMPLE__
  float limit = (bounce > 0) ? kernel_data.integrator.sample_clamp_indirect :
                               kernel_data.integrator.sample_clamp_direct;
  float sum = reduce_add_f(fabs(*L));
  if (sum > limit) {
    *L *= limit / sum;
  }
#endif
}

/* --------------------------------------------------------------------
 * Pass accumulation utilities.
 */

/* Get pointer to pixel in render buffer. */
ccl_device_forceinline ccl_global float *kernel_accum_pixel_render_buffer(
    INTEGRATOR_STATE_CONST_ARGS, ccl_global float *ccl_restrict render_buffer)
{
  const uint32_t render_pixel_index = INTEGRATOR_STATE(path, render_pixel_index);
  const uint64_t render_buffer_offset = (uint64_t)render_pixel_index *
                                        kernel_data.film.pass_stride;
  return render_buffer + render_buffer_offset;
}

/* --------------------------------------------------------------------
 * Adaptive sampling.
 */

ccl_device_inline int kernel_accum_sample(INTEGRATOR_STATE_CONST_ARGS,
                                          ccl_global float *ccl_restrict render_buffer,
                                          int sample)
{
  if (kernel_data.film.pass_sample_count == PASS_UNUSED) {
    return sample;
  }

  ccl_global float *buffer = kernel_accum_pixel_render_buffer(INTEGRATOR_STATE_PASS,
                                                              render_buffer);

  return atomic_fetch_and_add_uint32((uint *)(buffer) + kernel_data.film.pass_sample_count, 1);
}

ccl_device void kernel_accum_adaptive_buffer(INTEGRATOR_STATE_CONST_ARGS,
                                             const float3 contribution_rgb,
                                             ccl_global float *ccl_restrict buffer)
{
  /* Adaptive Sampling. Fill the additional buffer with the odd samples and calculate our stopping
   * criteria. This is the heuristic from "A hierarchical automatic stopping condition for Monte
   * Carlo global illumination" except that here it is applied per pixel and not in hierarchical
   * tiles. */

  if (kernel_data.film.pass_adaptive_aux_buffer == PASS_UNUSED) {
    return;
  }

  const int sample = INTEGRATOR_STATE(path, sample);
  if (sample_is_even(kernel_data.integrator.sampling_pattern, sample)) {
    kernel_write_pass_float4(buffer + kernel_data.film.pass_adaptive_aux_buffer,
                             make_float4(contribution_rgb.x * 2.0f,
                                         contribution_rgb.y * 2.0f,
                                         contribution_rgb.z * 2.0f,
                                         0.0f));
  }
}

/* --------------------------------------------------------------------
 * Shadow catcher.
 */

#ifdef __SHADOW_CATCHER__

/* Accumulate contribution to the Shadow Catcher pass.
 *
 * Returns truth if the contribution is fully handled here and is not to be added to the other
 * passes (like combined, adaptive sampling). */

ccl_device bool kernel_accum_shadow_catcher(INTEGRATOR_STATE_CONST_ARGS,
                                            const SpectralColor contribution,
                                            ccl_global float *ccl_restrict buffer)
{
  if (!kernel_data.integrator.has_shadow_catcher) {
    return false;
  }

  kernel_assert(kernel_data.film.pass_shadow_catcher != PASS_UNUSED);
  kernel_assert(kernel_data.film.pass_shadow_catcher_matte != PASS_UNUSED);

  const float3 contribution_rgb = spectrum_to_rgb(INTEGRATOR_STATE_PASS, contribution);

  /* Matte pass. */
  if (kernel_shadow_catcher_is_matte_path(INTEGRATOR_STATE_PASS)) {
    kernel_write_pass_float3(buffer + kernel_data.film.pass_shadow_catcher_matte,
                             contribution_rgb);
    /* NOTE: Accumulate the combined pass and to the samples count pass, so that the adaptive
     * sampling is based on how noisy the combined pass is as if there were no catchers in the
     * scene. */
  }

  /* Shadow catcher pass. */
  if (kernel_shadow_catcher_is_object_pass(INTEGRATOR_STATE_PASS)) {
    kernel_write_pass_float3(buffer + kernel_data.film.pass_shadow_catcher, contribution_rgb);
    return true;
  }

  return false;
}

ccl_device bool kernel_accum_shadow_catcher_transparent(INTEGRATOR_STATE_CONST_ARGS,
                                                        const SpectralColor contribution,
                                                        const float transparent,
                                                        ccl_global float *ccl_restrict buffer)
{
  if (!kernel_data.integrator.has_shadow_catcher) {
    return false;
  }

  kernel_assert(kernel_data.film.pass_shadow_catcher != PASS_UNUSED);
  kernel_assert(kernel_data.film.pass_shadow_catcher_matte != PASS_UNUSED);

  const float3 contribution_rgb = spectrum_to_rgb(INTEGRATOR_STATE_PASS, contribution);

  /* Matte pass. */
  if (kernel_shadow_catcher_is_matte_path(INTEGRATOR_STATE_PASS)) {
    kernel_write_pass_float4(
        buffer + kernel_data.film.pass_shadow_catcher_matte,
        make_float4(contribution_rgb.x, contribution_rgb.y, contribution_rgb.z, transparent));
    /* NOTE: Accumulate the combined pass and to the samples count pass, so that the adaptive
     * sampling is based on how noisy the combined pass is as if there were no catchers in the
     * scene. */
  }

  /* Shadow catcher pass. */
  if (kernel_shadow_catcher_is_object_pass(INTEGRATOR_STATE_PASS)) {
    /* NOTE: The transparency of the shadow catcher pass is ignored. It is not needed for the
     * calculation and the alpha channel of the pass contains numbers of samples contributed to a
     * pixel of the pass. */
    kernel_write_pass_float3(buffer + kernel_data.film.pass_shadow_catcher, contribution_rgb);
    return true;
  }

  return false;
}

#endif /* __SHADOW_CATCHER__ */

/* --------------------------------------------------------------------
 * Render passes.
 */

/* Write combined pass. */
ccl_device_inline void kernel_accum_combined_pass(INTEGRATOR_STATE_CONST_ARGS,
                                                  const SpectralColor contribution,
                                                  ccl_global float *ccl_restrict buffer)
{
#ifdef __SHADOW_CATCHER__
  if (kernel_accum_shadow_catcher(INTEGRATOR_STATE_PASS, contribution, buffer)) {
    return;
  }
#endif

  const float3 contribution_rgb = spectrum_to_rgb(INTEGRATOR_STATE_PASS, contribution);

  if (kernel_data.film.light_pass_flag & PASSMASK(COMBINED)) {
    kernel_write_pass_float3(buffer + kernel_data.film.pass_combined, contribution_rgb);
  }

  kernel_accum_adaptive_buffer(INTEGRATOR_STATE_PASS, contribution_rgb, buffer);
}

/* Write combined pass with transparency. */
ccl_device_inline void kernel_accum_combined_transparent_pass(INTEGRATOR_STATE_CONST_ARGS,
                                                              const SpectralColor contribution,
                                                              const float transparent,
                                                              ccl_global float *ccl_restrict
                                                                  buffer)
{
#ifdef __SHADOW_CATCHER__
  if (kernel_accum_shadow_catcher_transparent(
          INTEGRATOR_STATE_PASS, contribution, transparent, buffer)) {
    return;
  }
#endif

  const float3 contribution_rgb = spectrum_to_rgb(INTEGRATOR_STATE_PASS, contribution);

  if (kernel_data.film.light_pass_flag & PASSMASK(COMBINED)) {
    kernel_write_pass_float4(
        buffer + kernel_data.film.pass_combined,
        make_float4(contribution_rgb.x, contribution_rgb.y, contribution_rgb.z, transparent));
  }

  kernel_accum_adaptive_buffer(INTEGRATOR_STATE_PASS, contribution_rgb, buffer);
}

/* Write background or emission to appropriate pass. */
ccl_device_inline void kernel_accum_emission_or_background_pass(INTEGRATOR_STATE_CONST_ARGS,
                                                                SpectralColor contribution,
                                                                ccl_global float *ccl_restrict
                                                                    buffer,
                                                                const int pass)
{
  if (!(kernel_data.film.light_pass_flag & PASS_ANY)) {
    return;
  }

#ifdef __PASSES__
  const int path_flag = INTEGRATOR_STATE(path, flag);
  int pass_offset = PASS_UNUSED;

  /* Denoising albedo. */
#  ifdef __DENOISING_FEATURES__
  if (path_flag & PATH_RAY_DENOISING_FEATURES) {
    if (kernel_data.film.pass_denoising_albedo != PASS_UNUSED) {
      const SpectralColor denoising_feature_throughput = INTEGRATOR_STATE(
          path, denoising_feature_throughput);
      const SpectralColor denoising_albedo = denoising_feature_throughput * contribution;
      kernel_write_pass_spectral_color_unaligned(INTEGRATOR_STATE_PASS,
                                                 buffer + kernel_data.film.pass_denoising_albedo,
                                                 denoising_albedo);
    }
  }
#  endif /* __DENOISING_FEATURES__ */

  if (!(path_flag & PATH_RAY_ANY_PASS)) {
    /* Directly visible, write to emission or background pass. */
    pass_offset = pass;
  }
  else if (path_flag & PATH_RAY_REFLECT_PASS) {
    /* Indirectly visible through reflection. */
    const int glossy_pass_offset = pass_offset = (INTEGRATOR_STATE(path, bounce) == 1) ?
                                                     kernel_data.film.pass_glossy_direct :
                                                     kernel_data.film.pass_glossy_indirect;

    if (glossy_pass_offset != PASS_UNUSED) {
      /* Glossy is a subset of the throughput, reconstruct it here using the
       * diffuse-glossy ratio. */
      const SpectralColor ratio = INTEGRATOR_STATE(path, diffuse_glossy_ratio);
      const SpectralColor glossy_contribution = (one_spectral_color() - ratio) * contribution;
      kernel_write_pass_spectral_color(
          INTEGRATOR_STATE_PASS, buffer + glossy_pass_offset, glossy_contribution);
    }

    /* Reconstruct diffuse subset of throughput. */
    pass_offset = (INTEGRATOR_STATE(path, bounce) == 1) ? kernel_data.film.pass_diffuse_direct :
                                                          kernel_data.film.pass_diffuse_indirect;
    if (pass_offset != PASS_UNUSED) {
      contribution *= INTEGRATOR_STATE(path, diffuse_glossy_ratio);
    }
  }
  else if (path_flag & PATH_RAY_TRANSMISSION_PASS) {
    /* Indirectly visible through transmission. */
    pass_offset = (INTEGRATOR_STATE(path, bounce) == 1) ?
                      kernel_data.film.pass_transmission_direct :
                      kernel_data.film.pass_transmission_indirect;
  }
  else if (path_flag & PATH_RAY_VOLUME_PASS) {
    /* Indirectly visible through volume. */
    pass_offset = (INTEGRATOR_STATE(path, bounce) == 1) ? kernel_data.film.pass_volume_direct :
                                                          kernel_data.film.pass_volume_indirect;
  }

  /* Single write call for GPU coherence. */
  if (pass_offset != PASS_UNUSED) {
    kernel_write_pass_spectral_color(INTEGRATOR_STATE_PASS, buffer + pass_offset, contribution);
  }
#endif /* __PASSES__ */
}

/* Write light contribution to render buffer. */
ccl_device_inline void kernel_accum_light(INTEGRATOR_STATE_CONST_ARGS,
                                          ccl_global float *ccl_restrict render_buffer)
{
  /* The throughput for shadow paths already contains the light shader evaluation. */
  SpectralColor contribution = INTEGRATOR_STATE(shadow_path, throughput);
  kernel_accum_clamp(kg, &contribution, INTEGRATOR_STATE(shadow_path, bounce) - 1);

  ccl_global float *buffer = kernel_accum_pixel_render_buffer(INTEGRATOR_STATE_PASS,
                                                              render_buffer);

  kernel_accum_combined_pass(INTEGRATOR_STATE_PASS, contribution, buffer);

#ifdef __PASSES__
  if (kernel_data.film.light_pass_flag & PASS_ANY) {
    const int path_flag = INTEGRATOR_STATE(shadow_path, flag);
    int pass_offset = PASS_UNUSED;

    if (path_flag & PATH_RAY_REFLECT_PASS) {
      /* Indirectly visible through reflection. */
      const int glossy_pass_offset = pass_offset = (INTEGRATOR_STATE(shadow_path, bounce) == 0) ?
                                                       kernel_data.film.pass_glossy_direct :
                                                       kernel_data.film.pass_glossy_indirect;

      if (glossy_pass_offset != PASS_UNUSED) {
        /* Glossy is a subset of the throughput, reconstruct it here using the
         * diffuse-glossy ratio. */
        const SpectralColor ratio = INTEGRATOR_STATE(shadow_path, diffuse_glossy_ratio);
        const SpectralColor glossy_contribution = (one_spectral_color() - ratio) * contribution;
        kernel_write_pass_spectral_color(
            INTEGRATOR_STATE_PASS, buffer + glossy_pass_offset, glossy_contribution);
      }

      /* Reconstruct diffuse subset of throughput. */
      pass_offset = (INTEGRATOR_STATE(shadow_path, bounce) == 0) ?
                        kernel_data.film.pass_diffuse_direct :
                        kernel_data.film.pass_diffuse_indirect;
      if (pass_offset != PASS_UNUSED) {
        contribution *= INTEGRATOR_STATE(shadow_path, diffuse_glossy_ratio);
      }
    }
    else if (path_flag & PATH_RAY_TRANSMISSION_PASS) {
      /* Indirectly visible through transmission. */
      pass_offset = (INTEGRATOR_STATE(shadow_path, bounce) == 0) ?
                        kernel_data.film.pass_transmission_direct :
                        kernel_data.film.pass_transmission_indirect;
    }
    else if (path_flag & PATH_RAY_VOLUME_PASS) {
      /* Indirectly visible through volume. */
      pass_offset = (INTEGRATOR_STATE(shadow_path, bounce) == 0) ?
                        kernel_data.film.pass_volume_direct :
                        kernel_data.film.pass_volume_indirect;
    }

    /* Single write call for GPU coherence. */
    if (pass_offset != PASS_UNUSED) {
      kernel_write_pass_spectral_color(INTEGRATOR_STATE_PASS, buffer + pass_offset, contribution);
    }

    /* Write shadow pass. */
    if (kernel_data.film.pass_shadow != PASS_UNUSED && (path_flag & PATH_RAY_SHADOW_FOR_LIGHT) &&
        (path_flag & PATH_RAY_CAMERA)) {
      const SpectralColor unshadowed_throughput = INTEGRATOR_STATE(shadow_path,
                                                                   unshadowed_throughput);
      const SpectralColor shadowed_throughput = INTEGRATOR_STATE(shadow_path, throughput);
      const SpectralColor shadow = safe_divide(shadowed_throughput, unshadowed_throughput) *
                                   kernel_data.film.pass_shadow_scale;
      kernel_write_pass_spectral_color(
          INTEGRATOR_STATE_PASS, buffer + kernel_data.film.pass_shadow, shadow);
    }
  }
#endif
}

/* Write transparency to render buffer.
 *
 * Note that we accumulate transparency = 1 - alpha in the render buffer.
 * Otherwise we'd have to write alpha on path termination, which happens
 * in many places. */
ccl_device_inline void kernel_accum_transparent(INTEGRATOR_STATE_CONST_ARGS,
                                                const float transparent,
                                                ccl_global float *ccl_restrict render_buffer)
{
  if (kernel_data.film.light_pass_flag & PASSMASK(COMBINED)) {
    ccl_global float *buffer = kernel_accum_pixel_render_buffer(INTEGRATOR_STATE_PASS,
                                                                render_buffer);
    kernel_write_pass_float(buffer + kernel_data.film.pass_combined + 3, transparent);
  }
}

/* Write background contribution to render buffer.
 *
 * Includes transparency, matching kernel_accum_transparent. */
ccl_device_inline void kernel_accum_background(INTEGRATOR_STATE_CONST_ARGS,
                                               const SpectralColor L,
                                               const float transparent,
                                               ccl_global float *ccl_restrict render_buffer)
{
  SpectralColor contribution = INTEGRATOR_STATE(path, throughput) * L;
  kernel_accum_clamp(kg, &contribution, INTEGRATOR_STATE(path, bounce) - 1);

  ccl_global float *buffer = kernel_accum_pixel_render_buffer(INTEGRATOR_STATE_PASS,
                                                              render_buffer);

  kernel_accum_combined_transparent_pass(INTEGRATOR_STATE_PASS, contribution, transparent, buffer);
  kernel_accum_emission_or_background_pass(
      INTEGRATOR_STATE_PASS, contribution, buffer, kernel_data.film.pass_background);
}

/* Write emission to render buffer. */
ccl_device_inline void kernel_accum_emission(INTEGRATOR_STATE_CONST_ARGS,
                                             const SpectralColor throughput,
                                             const SpectralColor L,
                                             ccl_global float *ccl_restrict render_buffer)
{
  SpectralColor contribution = throughput * L;
  kernel_accum_clamp(kg, &contribution, INTEGRATOR_STATE(path, bounce) - 1);

  ccl_global float *buffer = kernel_accum_pixel_render_buffer(INTEGRATOR_STATE_PASS,
                                                              render_buffer);

  kernel_accum_combined_pass(INTEGRATOR_STATE_PASS, contribution, buffer);
  kernel_accum_emission_or_background_pass(
      INTEGRATOR_STATE_PASS, contribution, buffer, kernel_data.film.pass_emission);
}

CCL_NAMESPACE_END
