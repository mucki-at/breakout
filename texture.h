//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "buffermanager.h"

[[nodiscard]] DeviceImage createImageFromFile(
    const string& filename,
    const BufferManager& bufferManager
);
