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

#include "device/device_kernel.h"

CCL_NAMESPACE_BEGIN

class Device;

/* Abstraction of a command queue for a device.
 * Provides API to schedule kernel execution in a specific queue with minimal possible overhead
 * from driver side.
 *
 * This class encapsulates all properties needed for commands execution. */
class DeviceQueue {
 public:
  virtual ~DeviceQueue() = default;

  /* Enqueue kernel execution. */
  virtual void enqueue(DeviceKernel kernel) = 0;

  /* Device this queue has been created for. */
  Device *device;

 protected:
  /* Hide construction so that allocation via `Device` API is enforced. */
  explicit DeviceQueue(Device *device);
};

CCL_NAMESPACE_END