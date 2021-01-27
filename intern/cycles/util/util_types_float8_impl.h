/*
 * Original code Copyright 2017, Intel Corporation
 * Modifications Copyright 2018, Blender Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UTIL_TYPES_FLOAT8_IMPL_H__
#define __UTIL_TYPES_FLOAT8_IMPL_H__

#ifndef __UTIL_TYPES_H__
#  error "Do not include this file directly, include util_types.h instead."
#endif

#ifndef __KERNEL_GPU__
#  include <cstdio>
#endif

CCL_NAMESPACE_BEGIN

#ifdef __KERNEL_AVX2__
__forceinline float8::float8()
{
}

__forceinline float8::float8(const float8 &f) : m256(f.m256)
{
}

__forceinline float8::float8(const __m256 &f) : m256(f)
{
}

__forceinline float8::operator const __m256 &() const
{
  return m256;
}

__forceinline float8::operator __m256 &()
{
  return m256;
}

__forceinline float8 &float8::operator=(const float8 &f)
{
  m256 = f.m256;
  return *this;
}
#endif /* __KERNEL_AVX2__ */

#ifdef __KERNEL_GPU__
ccl_device_forceinline
#else
__forceinline
#endif
    float float8::operator[](int i) const
{
  util_assert(i >= 0);
  util_assert(i < 8);
  return *(&a + i);
}

#ifdef __KERNEL_GPU__
ccl_device_forceinline
#else
__forceinline
#endif
    float &float8::operator[](int i)
{
  util_assert(i >= 0);
  util_assert(i < 8);
  return *(&a + i);
}

ccl_device_inline float8 make_float8(float f)
{
#ifdef __KERNEL_AVX2__
  float8 r(_mm256_set1_ps(f));
#else
  float8 r = {f, f, f, f, f, f, f, f};
#endif
  return r;
}

ccl_device_inline float8
make_float8(float a, float b, float c, float d, float e, float f, float g, float h)
{
#ifdef __KERNEL_AVX2__
  float8 r(_mm256_set_ps(h, g, f, e, d, c, b, a));
#else
  float8 r = {a, b, c, d, e, f, g, h};
#endif
  return r;
}

ccl_device_inline float8 load_float8(const float *v)
{
  return make_float8(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
}

ccl_device_inline void store_float8(const float8 &a, float *v)
{
  v[0] = a.a;
  v[1] = a.b;
  v[2] = a.c;
  v[3] = a.d;
  v[4] = a.e;
  v[5] = a.f;
  v[6] = a.g;
  v[7] = a.h;
}

ccl_device_inline float3 float8_to_float3(const float8 &f)
{
  return make_float3(f[0], f[1], f[2]);
}

ccl_device_inline float8 float3_to_float8(const float3 &f)
{
  float8 res;
  res[0] = f.x;
  res[1] = f.y;
  res[2] = f.z;
  return res;
}

CCL_NAMESPACE_END

#endif /* __UTIL_TYPES_FLOAT8_IMPL_H__ */
