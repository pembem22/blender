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

#ifndef __UTIL_MATH_FLOAT_H__
#define __UTIL_MATH_FLOAT_H__

#ifndef __UTIL_MATH_H__
#  error "Do not include this file directly, include util_types.h instead."
#endif

CCL_NAMESPACE_BEGIN

/*******************************************************************************
 * Declaration.
 */

ccl_device_inline float rcp(const float &a);
ccl_device_inline float safe_rcp(const float &a);
ccl_device_inline bool is_zero(const float &a);
ccl_device_inline float average(const float &a);

ccl_device_inline float safe_divide_even(const float a, const float b);

ccl_device_inline float reduce_min(const float &a);
ccl_device_inline float reduce_max(const float &a);
ccl_device_inline float reduce_add(const float &a);

ccl_device_inline float reduce_min_f(const float &a);
ccl_device_inline float reduce_max_f(const float &a);
ccl_device_inline float reduce_add_f(const float &a);

ccl_device_inline bool isequal(const float a, const float b);

/*******************************************************************************
 * Definition.
 */

ccl_device_inline float rcp(const float &a)
{
  return 1.0f / a;
}

ccl_device_inline float safe_rcp(const float &a)
{
  return a == 0.0f ? 0.0f : 1.0f / a;
}

ccl_device_inline bool is_zero(const float &a)
{
  return a == make_float(0.0f);
}

ccl_device_inline float average(const float &a)
{
  return reduce_add_f(a);
}

ccl_device_inline float reduce_min(const float &a)
{
  return a;
}

ccl_device_inline float reduce_max(const float &a)
{
  return a;
}

ccl_device_inline float reduce_add(const float &a)
{
  return a;
}

ccl_device_inline float reduce_min_f(const float &a)
{
  return reduce_min(a);
}

ccl_device_inline float reduce_max_f(const float &a)
{
  return reduce_max(a);
}

ccl_device_inline float reduce_add_f(const float &a)
{
  return reduce_add(a);
}

ccl_device_inline bool isequal(const float a, const float b)
{
  return a == b;
}

ccl_device_inline float safe_divide_even(const float a, const float b)
{
  return b == 0.0f ? 0.0f : a / b;
}

#ifndef __KERNEL_GPU__
ccl_device_inline float fabs(const float &a)
{
  return fabsf(a);
}

ccl_device_inline float log(float v)
{
  return logf(v);
}

ccl_device_inline float pow(float v, float e)
{
  return powf(v, e);
}

ccl_device_inline float exp(float v)
{
  return expf(v);
}

ccl_device_inline float expm1(float v)
{
  return exp(v) - 1;
}
#endif

ccl_device_inline float dot(const float &a, const float &b)
{
  return a * b;
}

CCL_NAMESPACE_END

#endif /* __UTIL_MATH_FLOAT_H__ */
