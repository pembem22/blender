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

CCL_NAMESPACE_BEGIN

/* Nishita sky texture */

/* Constants */
ccl_static_constant float rayleigh_scale = 8e3f;        // Rayleigh scale height (m)
ccl_static_constant float mie_scale = 1.2e3f;           // Mie scale height (m)
ccl_static_constant float mie_coeff = 2e-5f;            // Mie scattering coefficient
ccl_static_constant float mie_G = 0.76f;                // aerosols anisotropy
ccl_static_constant float sqr_G = 0.76f * 0.76f;        // squared aerosols anisotropy
ccl_static_constant float earth_radius = 6360e3f;       // radius of Earth (m)
ccl_static_constant float atmosphere_radius = 6420e3f;  // radius of atmosphere (m)
ccl_static_constant int steps = 32;                     // segments per primary ray
ccl_static_constant int steps_light = 16;               // segments per sun connection ray

/* Does not compile with CUDA. */
// ccl_static_constant float sqr_G = mie_G * mie_G;        // squared aerosols anisotropy

#define SKY_LUT_WAVELENGTH_MIN 380
#define SKY_LUT_WAVELENGTH_MAX 780
#define SKY_LUT_WAVELENGTH_NUM 21
#define SKY_LUT_WAVELENGTH_STEP 19

ccl_static_constant float step_lambda = (SKY_LUT_WAVELENGTH_MAX - SKY_LUT_WAVELENGTH_MIN) /
                                        (SKY_LUT_WAVELENGTH_NUM -
                                         1);  // step between each sampled wavelength (nm)

/* LUTs. Range 380 - 780, step 19. */

/* irradiance at top of atmosphere (W*m^-2*nm^-1) */
ccl_static_constant float irradiance[] = {
    1.45756829855592995315f, 1.56596305559738380175f, 1.65148449067670455293f,
    1.71496242737209314555f, 1.75797983805020541226f, 1.78256407885924539336f,
    1.79095108475838560302f, 1.78541550133410664714f, 1.76815554864306845317f,
    1.74122069647250410362f, 1.70647127164943679389f, 1.66556087452739887134f,
    1.61993437242451854274f, 1.57083597368892080581f, 1.51932335059305478886f,
    1.46628494965214395407f, 1.41245852740172450623f, 1.35844961970384092709f,
    1.30474913844739281998f, 1.25174963272610817455f, 1.19975998755420620867f};
/* Rayleigh scattering coefficient (m^-1) */
ccl_static_constant float rayleigh_coeff[] = {
    0.00005424820087636473f, 0.00004418549866505454f, 0.00003635151910165377f,
    0.00003017929012024763f, 0.00002526320226989157f, 0.00002130859310621843f,
    0.00001809838025320633f, 0.00001547057129129042f, 0.00001330284977336850f,
    0.00001150184784075764f, 0.00000999557429990163f, 0.00000872799973630707f,
    0.00000765513700977967f, 0.00000674217203751443f, 0.00000596134125832052f,
    0.00000529034598065810f, 0.00000471115687557433f, 0.00000420910481110487f,
    0.00000377218381260133f, 0.00000339051255477280f, 0.00000305591531679811f};
/* Ozone absorption coefficient (m^-1) */
ccl_static_constant float ozone_coeff[] = {
    0.00000000325126849861f, 0.00000000585395365047f, 0.00000001977191155085f,
    0.00000007309568762914f, 0.00000020084561514287f, 0.00000040383958096161f,
    0.00000063551335912363f, 0.00000096707041180970f, 0.00000154797400424410f,
    0.00000209038647223331f, 0.00000246128056164565f, 0.00000273551299461512f,
    0.00000215125863128643f, 0.00000159051840791988f, 0.00000112356197979857f,
    0.00000073527551487574f, 0.00000046450130357806f, 0.00000033096079921048f,
    0.00000022512612292678f, 0.00000014879129266490f, 0.00000016828623364192f};

// ccl_device_inline float3 geographical_to_direction(float lat, float lon)
// {
//   return make_float3(cosf(lat) * cosf(lon), cosf(lat) * sinf(lon), sinf(lat));
// }

/* Atmosphere volume models */

ccl_device_inline float density_rayleigh(float height)
{
  return expf(-height / rayleigh_scale);
}

ccl_device_inline float density_mie(float height)
{
  return expf(-height / mie_scale);
}

ccl_device_inline float density_ozone(float height)
{
  float den = 0.0f;
  if (height >= 10000.0f && height < 25000.0f)
    den = 1.0f / 15000.0f * height - 2.0f / 3.0f;
  else if (height >= 25000 && height < 40000)
    den = -(1.0f / 15000.0f * height - 8.0f / 3.0f);
  return den;
}

ccl_device_inline float phase_rayleigh(float mu)
{
  return 3.0f / (16.0f * M_PI_F) * (1.0f + sqr(mu));
}

ccl_device_inline float phase_mie(float mu)
{
  return (3.0f * (1.0f - sqr_G) * (1.0f + sqr(mu))) /
         (8.0f * M_PI_F * (2.0f + sqr_G) * powf((1.0f + sqr_G - 2.0f * mie_G * mu), 1.5));
}

/* Intersection helpers */
ccl_device_inline bool surface_intersection(float3 pos, float3 dir)
{
  if (dir.z >= 0)
    return false;
  float t = dot(dir, -pos) / len_squared(dir);
  float D = pos.x * pos.x - 2.0f * (-pos.x) * dir.x * t + dir.x * t * dir.x * t + pos.y * pos.y -
            2.0f * (-pos.y) * dir.y * t + (dir.y * t) * (dir.y * t) + pos.z * pos.z -
            2.0f * (-pos.z) * dir.z * t + dir.z * t * dir.z * t;
  return (D <= sqr(earth_radius));
}

ccl_device_inline float3 atmosphere_intersection(float3 pos, float3 dir)
{
  float b = -2.0f * dot(dir, -pos);
  float c = len_squared(pos) - sqr(atmosphere_radius);
  float t = (-b + sqrtf(b * b - 4.0f * c)) / 2.0f;
  return make_float3(pos.x + dir.x * t, pos.y + dir.y * t, pos.z + dir.z * t);
}

ccl_device_inline float3 ray_optical_depth(float3 ray_origin, float3 ray_dir)
{
  /* This code computes the optical depth along a ray through the atmosphere. */
  float3 ray_end = atmosphere_intersection(ray_origin, ray_dir);
  float ray_length = distance(ray_origin, ray_end);

  /* To compute the optical depth, we step along the ray in segments and
   * accumulate the optical depth along each segment. */
  float segment_length = ray_length / steps_light;
  float3 segment = segment_length * ray_dir;

  /* Instead of tracking the transmission spectrum across all wavelengths directly,
   * we use the fact that the density always has the same spectrum for each type of
   * scattering, so we split the density into a constant spectrum and a factor and
   * only track the factors. */
  float3 optical_depth = make_float3(0.0f, 0.0f, 0.0f);

  /* The density of each segment is evaluated at its middle. */
  float3 P = ray_origin + 0.5f * segment;

  for (int i = 0; i < steps_light; i++) {
    /* Compute height above sea level. */
    float height = len(P) - earth_radius;

    /* Accumulate optical depth of this segment (density is assumed to be constant along it). */
    float3 density = make_float3(
        density_rayleigh(height), density_mie(height), density_ozone(height));
    optical_depth += density;

    /* Advance along ray. */
    P += segment;
  }

  return optical_depth * segment_length;
}

/* Single Scattering implementation */
ccl_device SpectralColor single_scattering(float3 ray_dir,
                                           float3 sun_dir,
                                           float3 ray_origin,
                                           float air_density,
                                           float dust_density,
                                           float ozone_density,
                                           SpectralColor wavelengths)
{
  SpectralColor spectrum = make_spectral_color(0.0f);

  /* This code computes single-inscattering along a ray through the atmosphere. */
  float3 ray_end = atmosphere_intersection(ray_origin, ray_dir);
  float ray_length = distance(ray_origin, ray_end);

  /* To compute the inscattering, we step along the ray in segments and accumulate
   * the inscattering as well as the optical depth along each segment. */
  float segment_length = ray_length / steps;
  float3 segment = segment_length * ray_dir;

  /* Instead of tracking the transmission spectrum across all wavelengths directly,
   * we use the fact that the density always has the same spectrum for each type of
   * scattering, so we split the density into a constant spectrum and a factor and
   * only track the factors. */
  float3 optical_depth = make_float3(0.0f, 0.0f, 0.0f);

  /* Compute phase function for scattering and the density scale factor. */
  float mu = dot(ray_dir, sun_dir);
  float3 phase_function = make_float3(phase_rayleigh(mu), phase_mie(mu), 0.0f);
  float3 density_scale = make_float3(air_density, dust_density, ozone_density);

  /* The density and in-scattering of each segment is evaluated at its middle. */
  float3 P = ray_origin + 0.5f * segment;

  for (int i = 0; i < steps; i++) {
    /* Compute height above sea level. */
    float height = len(P) - earth_radius;

    /* Evaluate and accumulate optical depth along the ray. */
    float3 density = density_scale * make_float3(density_rayleigh(height),
                                                 density_mie(height),
                                                 density_ozone(height));
    optical_depth += segment_length * density;

    /* If the earth isn't in the way, evaluate inscattering from the sun. */
    if (!surface_intersection(P, sun_dir)) {
      float3 light_optical_depth = density_scale * ray_optical_depth(P, sun_dir);
      float3 total_optical_depth = optical_depth + light_optical_depth;

      // /* attenuation of light */
      // FOR_EACH_CHANNEL(wl)
      // {
      //   float wavelength = wavelengths[wl];
      //   float current_irradiance = find_position_in_lookup_unit_step(irradiance,
      //                                                                wavelength,
      //                                                                SKY_LUT_WAVELENGTH_MIN,
      //                                                                SKY_LUT_WAVELENGTH_MAX,
      //                                                                SKY_LUT_WAVELENGTH_STEP);
      //   float current_rayleigh_coeff = find_position_in_lookup_unit_step(rayleigh_coeff,
      //                                                                    wavelength,
      //                                                                    SKY_LUT_WAVELENGTH_MIN,
      //                                                                    SKY_LUT_WAVELENGTH_MAX,
      //                                                                    SKY_LUT_WAVELENGTH_STEP);
      //   float current_ozone_coeff = find_position_in_lookup_unit_step(ozone_coeff,
      //                                                                 wavelength,
      //                                                                 SKY_LUT_WAVELENGTH_MIN,
      //                                                                 SKY_LUT_WAVELENGTH_MAX,
      //                                                                 SKY_LUT_WAVELENGTH_STEP);

      //   float3 extinction_density = total_optical_depth * make_float3(current_rayleigh_coeff,
      //                                                                 1.11f * mie_coeff,
      //                                                                 current_ozone_coeff);
      //   float attenuation = expf(-reduce_add_f(extinction_density));

      //   float3 scattering_density = density * make_float3(current_rayleigh_coeff, mie_coeff,
      //   0.0f);

      //   /* The total inscattered radiance from one segment is:
      //    * Tr(A<->B) * Tr(B<->C) * sigma_s * phase * L * segment_length
      //    *
      //    * These terms are:
      //    * Tr(A<->B): Transmission from start to scattering position (tracked in optical_depth)
      //    * Tr(B<->C): Transmission from scattering position to light (computed in
      //    * ray_optical_depth)
      //    * sigma_s: Scattering density
      //    * phase: Phase function of the scattering type (Rayleigh or Mie)
      //    * L: Radiance coming from the light source segment_length: The length of the segment
      //    *
      //    * The code here is just that, with a bit of additional optimization to not store full
      //    * spectra for the optical depth.
      //    */

      //   spectrum[wl] += attenuation * reduce_add_f(phase_function * scattering_density) *
      //                   current_irradiance * segment_length;
      // }
    }

    /* Advance along ray. */
    P += segment;
  }

  return spectrum;
}

/* Sun disc */
ccl_device SpectralColor sun_radiation(float3 cam_dir,
                                       float altitude,
                                       float air_density,
                                       float dust_density,
                                       float solid_angle,
                                       SpectralColor wavelengths)
{
  float3 cam_pos = make_float3(0, 0, earth_radius + altitude);
  float3 optical_depth = ray_optical_depth(cam_pos, cam_dir);

  SpectralColor radiation;

  // /* Compute final spectrum. */
  // FOR_EACH_CHANNEL(i)
  // {
  //   float wavelength = wavelengths[i];
  //   float current_irradiance = find_position_in_lookup_unit_step(irradiance,
  //                                                                wavelength,
  //                                                                SKY_LUT_WAVELENGTH_MIN,
  //                                                                SKY_LUT_WAVELENGTH_MAX,
  //                                                                SKY_LUT_WAVELENGTH_STEP);
  //   float current_rayleigh_coeff = find_position_in_lookup_unit_step(rayleigh_coeff,
  //                                                                    wavelength,
  //                                                                    SKY_LUT_WAVELENGTH_MIN,
  //                                                                    SKY_LUT_WAVELENGTH_MAX,
  //                                                                    SKY_LUT_WAVELENGTH_STEP);

  //   /* Combine spectra and the optical depth into transmittance. */
  //   float transmittance = current_rayleigh_coeff * optical_depth.x * air_density +
  //                         1.11f * mie_coeff * optical_depth.y * dust_density;
  //   radiation[i] = current_irradiance * expf(-transmittance) / solid_angle;
  // }

  return radiation;
}

ccl_device void svm_node_tex_sky_nishita(
    KernelGlobals *kg, PathState *state, float *stack, uint4 node, int *offset)
{
  SpectralColor spectrum;

  //   /* convert dir to spherical coordinates */
  uint spectrum_out = node.z;
  uint dir_offset = node.y;
  float3 cam_dir = stack_load_float3(stack, dir_offset);
  //   float2 direction = direction_to_spherical(dir);

  float sky_intensity = __uint_as_float(node.w);
  float4 data = read_node_float(kg, offset);
  float sun_elevation = data.x;
  float sun_rotation = data.y;
  float sun_size = data.z;
  bool enable_sun_disk = __float_as_uint(data.w) != 0;

  data = read_node_float(kg, offset);
  float altitude = data.x;
  float air_density = data.y;
  float dust_density = data.z;
  float ozone_density = data.w;

  float3 cam_pos = make_float3(0, 0, earth_radius + altitude);
  float3 sun_dir = geographical_to_direction(sun_elevation, -sun_rotation + M_PI_2_F);

  /* render above the horizon */
  if (cam_dir.z >= 0.0f) {
    /* definitions */
    float sun_dir_angle = precise_angle(cam_dir, sun_dir);
    float half_angular = sun_size / 2.0f;
    float solid_angle = M_2PI_F * (1.0f - cosf(half_angular));
    // float dir_elevation = M_PI_2_F - direction.x;

    /* if ray inside sun disc render it, otherwise render sky */
    if (enable_sun_disk && sun_dir_angle < half_angular) {
      spectrum = sun_radiation(
          cam_dir, altitude, air_density, dust_density, solid_angle, state->wavelengths);

      /* limb darkening, coefficient is 0.6f */
      float limb_darkening = (1.0f -
                              0.6f * (1.0f - sqrtf(1.0f - sqr(sun_dir_angle / half_angular))));
      spectrum *= limb_darkening;
      spectrum *= sky_intensity;
    }
    /* sky */
    else {
      spectrum = single_scattering(
          cam_dir, sun_dir, cam_pos, air_density, dust_density, ozone_density, state->wavelengths);
    }
  }
  /* ground */
  else {
    if (cam_dir.z < -0.4f) {
      spectrum = make_spectral_color(0.0f);
    }
    else {
      /* black ground fade */
      float fade = 1.0f + cam_dir.z * 2.5f;
      fade = sqr(fade) * fade;
      /* interpolation */

      cam_dir.z = 0.0f;
      spectrum = single_scattering(
          cam_dir, sun_dir, cam_pos, air_density, dust_density, ozone_density, state->wavelengths);
      spectrum *= fade;
    }
  }

  spectrum *= step_lambda;

  stack_store_spectral(stack, spectrum_out, spectrum);
}

CCL_NAMESPACE_END
