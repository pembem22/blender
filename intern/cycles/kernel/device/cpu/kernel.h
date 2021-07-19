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

// #pragma once

/* CPU Kernel Interface */

#include "util/util_types.h"

#include "kernel/kernel_types.h"

CCL_NAMESPACE_BEGIN

#ifndef KERNEL_TYPE
#  ifdef __SPECTRAL_RENDERING__
#    define KERNEL_TYPE spectral
#  else
#    define KERNEL_TYPE rgb
#  endif
#endif

#define KERNEL_NAME_JOIN_3(x, y, z) x##_##y##_##z
#define KERNEL_NAME_JOIN_4(x, y, z, w) x##_##y##_##z##_##w
#define KERNEL_NAME_EVAL_TYPELESS(arch, name) KERNEL_NAME_JOIN_3(kernel, arch, name)
#define KERNEL_NAME_EVAL(arch, name, type) KERNEL_NAME_JOIN_4(kernel, arch, name, type)
#define KERNEL_FUNCTION_FULL_NAME(name) KERNEL_NAME_EVAL(KERNEL_ARCH, name, KERNEL_TYPE)

struct IntegratorState;
struct KernelGlobals;
struct KernelData;

KernelGlobals *kernel_globals_create();
void kernel_globals_free(KernelGlobals *kg);

void *kernel_osl_memory(const KernelGlobals *kg);
bool kernel_osl_use(const KernelGlobals *kg);

void kernel_const_copy(KernelGlobals *kg, const char *name, void *host, size_t size);
void kernel_global_memory_copy(KernelGlobals *kg, const char *name, void *mem, size_t size);

#define KERNEL_ARCH cpu
#include "kernel/device/cpu/kernel_arch.h"

#define KERNEL_ARCH cpu_sse2
#include "kernel/device/cpu/kernel_arch.h"

#define KERNEL_ARCH cpu_sse3
#include "kernel/device/cpu/kernel_arch.h"

#define KERNEL_ARCH cpu_sse41
#include "kernel/device/cpu/kernel_arch.h"

#define KERNEL_ARCH cpu_avx
#include "kernel/device/cpu/kernel_arch.h"

#define KERNEL_ARCH cpu_avx2
#include "kernel/device/cpu/kernel_arch.h"

CCL_NAMESPACE_END
