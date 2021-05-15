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

#pragma once

#include "kernel/kernel_accumulate.h"
#include "kernel/kernel_emission.h"
#include "kernel/kernel_light.h"
#include "kernel/kernel_shader.h"

CCL_NAMESPACE_BEGIN

ccl_device_noinline_cpu SpectralColor integrator_eval_background_shader(
    INTEGRATOR_STATE_ARGS, ccl_global float *ccl_restrict render_buffer)
{
#ifdef __BACKGROUND__
  const int shader = kernel_data.background.surface_shader;
  const uint32_t path_flag = INTEGRATOR_STATE(path, flag);

  /* Use visibility flag to skip lights. */
  if (shader & SHADER_EXCLUDE_ANY) {
    if (((shader & SHADER_EXCLUDE_DIFFUSE) && (path_flag & PATH_RAY_DIFFUSE)) ||
        ((shader & SHADER_EXCLUDE_GLOSSY) && ((path_flag & (PATH_RAY_GLOSSY | PATH_RAY_REFLECT)) ==
                                              (PATH_RAY_GLOSSY | PATH_RAY_REFLECT))) ||
        ((shader & SHADER_EXCLUDE_TRANSMIT) && (path_flag & PATH_RAY_TRANSMIT)) ||
        ((shader & SHADER_EXCLUDE_CAMERA) && (path_flag & PATH_RAY_CAMERA)) ||
        ((shader & SHADER_EXCLUDE_SCATTER) && (path_flag & PATH_RAY_VOLUME_SCATTER)))
      return zero_spectral_color();
  }

  /* Fast path for constant color shader. */
  SpectralColor L = zero_spectral_color();
  if (shader_constant_emission_eval(kg, shader, &L)) {
    return L;
  }

  /* Evaluate background shader. */
  {
    /* TODO: does aliasing like this break automatic SoA in CUDA?
     * Should we instead store closures separate from ShaderData? */
    ShaderDataTinyStorage emission_sd_storage;
    ShaderData *emission_sd = AS_SHADER_DATA(&emission_sd_storage);

    shader_setup_from_background(kg,
                                 emission_sd,
                                 INTEGRATOR_STATE(ray, P),
                                 INTEGRATOR_STATE(ray, D),
                                 INTEGRATOR_STATE(ray, time));
    shader_eval_surface(
        INTEGRATOR_STATE_PASS, emission_sd, render_buffer, path_flag | PATH_RAY_EMISSION);

    L = shader_background_eval(emission_sd);
  }

  /* Background MIS weights. */
#  ifdef __BACKGROUND_MIS__
  /* Check if background light exists or if we should skip pdf. */
  if (!(INTEGRATOR_STATE(path, flag) & PATH_RAY_MIS_SKIP) && kernel_data.background.use_mis) {
    const float3 ray_P = INTEGRATOR_STATE(ray, P);
    const float3 ray_D = INTEGRATOR_STATE(ray, D);
    const float mis_ray_pdf = INTEGRATOR_STATE(path, mis_ray_pdf);
    const float mis_ray_t = INTEGRATOR_STATE(path, mis_ray_t);

    /* multiple importance sampling, get background light pdf for ray
     * direction, and compute weight with respect to BSDF pdf */
    const float pdf = background_light_pdf(kg, ray_P - ray_D * mis_ray_t, ray_D);
    const float mis_weight = power_heuristic(mis_ray_pdf, pdf);

    L *= mis_weight;
  }
#  endif

  return L;
#else
  return make_spectral_color(0.8f);
#endif
}

ccl_device_inline void integrate_background(INTEGRATOR_STATE_ARGS,
                                            ccl_global float *ccl_restrict render_buffer)
{
  /* Accumulate transparency for transparent background. We can skip background
   * shader evaluation unless a background pass is used. */
  bool eval_background = true;
  float transparent = 0.0f;

  if (kernel_data.background.transparent &&
      (INTEGRATOR_STATE(path, flag) & PATH_RAY_TRANSPARENT_BACKGROUND)) {
    transparent = average(INTEGRATOR_STATE(path, throughput));

#ifdef __PASSES__
    eval_background = (kernel_data.film.light_pass_flag & PASSMASK(BACKGROUND));
#else
    eval_background = false;
#endif
  }

  /* Evaluate background shader. */
  SpectralColor L = (eval_background) ?
                        integrator_eval_background_shader(INTEGRATOR_STATE_PASS, render_buffer) :
                        zero_spectral_color();

  /* When using the ao bounces approximation, adjust background
   * shader intensity with ao factor. */
  if (path_state_ao_bounce(INTEGRATOR_STATE_PASS)) {
    L *= kernel_data.background.ao_bounces_factor;
  }

  /* Write to render buffer. */
  kernel_accum_background(INTEGRATOR_STATE_PASS, L, transparent, render_buffer);
}

ccl_device_inline void integrate_distant_lights(INTEGRATOR_STATE_ARGS,
                                                ccl_global float *ccl_restrict render_buffer)
{
  const float3 ray_D = INTEGRATOR_STATE(ray, D);
  const float ray_time = INTEGRATOR_STATE(ray, time);
  LightSample ls ccl_optional_struct_init;
  for (int lamp = 0; lamp < kernel_data.integrator.num_all_lights; lamp++) {
    if (light_sample_from_distant_ray(kg, ray_D, lamp, &ls)) {
      /* Use visibility flag to skip lights. */
#ifdef __PASSES__
      const uint32_t path_flag = INTEGRATOR_STATE(path, flag);

      if (ls.shader & SHADER_EXCLUDE_ANY) {
        if (((ls.shader & SHADER_EXCLUDE_DIFFUSE) && (path_flag & PATH_RAY_DIFFUSE)) ||
            ((ls.shader & SHADER_EXCLUDE_GLOSSY) &&
             ((path_flag & (PATH_RAY_GLOSSY | PATH_RAY_REFLECT)) ==
              (PATH_RAY_GLOSSY | PATH_RAY_REFLECT))) ||
            ((ls.shader & SHADER_EXCLUDE_TRANSMIT) && (path_flag & PATH_RAY_TRANSMIT)) ||
            ((ls.shader & SHADER_EXCLUDE_SCATTER) && (path_flag & PATH_RAY_VOLUME_SCATTER)))
          return;
      }
#endif

      /* Evaluate light shader. */
      /* TODO: does aliasing like this break automatic SoA in CUDA? */
      ShaderDataTinyStorage emission_sd_storage;
      ShaderData *emission_sd = AS_SHADER_DATA(&emission_sd_storage);
      SpectralColor light_eval = light_sample_shader_eval(
          INTEGRATOR_STATE_PASS, emission_sd, &ls, ray_time);
      if (is_zero(light_eval)) {
        return;
      }

      /* MIS weighting. */
      if (!(path_flag & PATH_RAY_MIS_SKIP)) {
        /* multiple importance sampling, get regular light pdf,
         * and compute weight with respect to BSDF pdf */
        const float mis_ray_pdf = INTEGRATOR_STATE(path, mis_ray_pdf);
        const float mis_weight = power_heuristic(mis_ray_pdf, ls.pdf);
        light_eval *= mis_weight;
      }

      /* Write to render buffer. */
      kernel_accum_emission(INTEGRATOR_STATE_PASS, light_eval, render_buffer);
    }
  }
}

ccl_device void integrator_shade_background(INTEGRATOR_STATE_ARGS,
                                            ccl_global float *ccl_restrict render_buffer)
{
  // TODO: unify these in a single loop to only have a single shader evaluation call
  integrate_distant_lights(INTEGRATOR_STATE_PASS, render_buffer);
  integrate_background(INTEGRATOR_STATE_PASS, render_buffer);

  /* Path ends here. */
  INTEGRATOR_PATH_TERMINATE(SHADE_BACKGROUND);
}

CCL_NAMESPACE_END
