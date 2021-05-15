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

#ifndef __SVM_RGB_TO_SPECTRUM_H__
#define __SVM_RGB_TO_SPECTRUM_H__

CCL_NAMESPACE_BEGIN

ccl_device void svm_node_rgb_to_spectrum(INTEGRATOR_STATE_CONST_ARGS,
                                         ShaderData *sd,
                                         float *stack,
                                         uint in_color_offset,
                                         uint out_spectrum_offset)
{
  float3 in_color = stack_load_float3(stack, in_color_offset);
  stack_store_spectral_color(
      stack, out_spectrum_offset, rgb_to_spectrum(INTEGRATOR_STATE_PASS, in_color));
}

CCL_NAMESPACE_END

#endif /* __SVM_RGB_TO_SPECTRUM_H__ */