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

#ifndef __SVM_SPECTRUM_CURVE_H__
#define __SVM_SPECTRUM_CURVE_H__

CCL_NAMESPACE_BEGIN

ccl_device void svm_node_spectrum_curves(
    KernelGlobals *kg, PathState *state, float *stack, uint4 node, int *offset)
{
  uint out_offset = node.y;
  uint table_size = node.z;
  int table_offset = *offset;

  SpectralColor out;

  float wavelength = state->wavelengths;
  float t = (wavelength - MIN_WAVELENGTH) / (MAX_WAVELENGTH - MIN_WAVELENGTH);

  int position = int(t * (table_size - 1));
  float progress = (t * (table_size - 1)) - position;

  out = mix(fetch_node_float(kg, table_offset + position).x,
            fetch_node_float(kg, table_offset + min(table_size - 1, position + 1)).x,
            progress);

  stack_store_spectral(stack, out_offset, out);

  *offset += table_size;
}

CCL_NAMESPACE_END

#endif /* __SVM_SPECTRUM_CURVE_H__ */
