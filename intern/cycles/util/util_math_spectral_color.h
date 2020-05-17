/*
 * Copyright 2011-2017 Blender Foundation
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

#ifndef __UTIL_MATH_SPECTRAL_COLOR_H__
#define __UTIL_MATH_SPECTRAL_COLOR_H__

#ifndef __UTIL_MATH_H__
#  error "Do not include this file directly, include util_types.h instead."
#endif

CCL_NAMESPACE_BEGIN

ccl_device_inline SpectralColor ensure_finite(SpectralColor v)
{
  SPECTRAL_COLOR_FOR_EACH(i)
  {
    v[i] = (isfinite_safe(v[i])) ? v[i] : 0.0f;
  }
  return v;
}

ccl_device_inline float reduce_add_spectral(const SpectralColor &a)
{
  float f = 0.0f;
  SPECTRAL_COLOR_FOR_EACH(i)
  {
    f += a[i];
  }
  return f;
}

ccl_device_inline float reduce_max_spectral(const SpectralColor &a)
{
  float f = -INFINITY;
  SPECTRAL_COLOR_FOR_EACH(i)
  {
    f = max(f, a[i]);
  }
  return f;
}

ccl_device_inline SpectralColor fabs(SpectralColor &a)
{
  SPECTRAL_COLOR_FOR_EACH(i)
  {
    a[i] += fabsf(a[i]);
  }
  return a;
}

ccl_device_inline SpectralColor exp_s(SpectralColor v)
{
  SPECTRAL_COLOR_FOR_EACH(i)
  {
    v[i] = expf(v[i]);
  }
  return v;
}

ccl_device_inline SpectralColor log_s(SpectralColor v)
{
  SPECTRAL_COLOR_FOR_EACH(i)
  {
    v[i] = logf(v[i]);
  }
  return v;
}

CCL_NAMESPACE_END

#endif /* __UTIL_MATH_SPECTRAL_COLOR_H__ */