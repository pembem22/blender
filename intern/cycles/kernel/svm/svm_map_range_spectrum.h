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

/* Map Range Node */

ccl_device void svm_node_map_range_spectrum(KernelGlobals *kg,
                                            ShaderData *sd,
                                            float *stack,
                                            uint value_stack_offset,
                                            uint parameters_stack_offsets,
                                            uint result_stack_offsets,
                                            int *offset)
{
  uint from_min_stack_offset, from_max_stack_offset, to_min_stack_offset, to_max_stack_offset;
  uint result_stack_offset, clamp;
  svm_unpack_node_uchar4(parameters_stack_offsets,
                         &from_min_stack_offset,
                         &from_max_stack_offset,
                         &to_min_stack_offset,
                         &to_max_stack_offset);
  svm_unpack_node_uchar2(result_stack_offsets, &result_stack_offset, &clamp);

#ifdef __WITH_SPECTRAL_RENDERING__
  SpectralColor value = stack_load_spectral(stack, value_stack_offset);
  SpectralColor from_min = stack_load_spectral(stack, from_min_stack_offset);
  SpectralColor from_max = stack_load_spectral(stack, from_max_stack_offset);
  SpectralColor to_min = stack_load_spectral(stack, to_min_stack_offset);
  SpectralColor to_max = stack_load_spectral(stack, to_max_stack_offset);

  SpectralColor result;

  SpectralColor factor = safe_divide(value - from_min, from_max - from_min);
  result = to_min + factor * (to_max - to_min);

  if (clamp) {
    result = max(result, to_min);
    result = min(result, to_max);
  }

  stack_store_spectral(stack, result_stack_offset, result);
#endif
}

CCL_NAMESPACE_END
