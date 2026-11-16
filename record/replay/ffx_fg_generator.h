// AMD FidelityFX Frame Generation (FSR 3 / 3.1) interface for nr-replay.
// Evaluates frame generation using AMD's FidelityFX API DLL (amd_fidelityfx_dx12.dll or
// amd_fidelityfx_loader_dx12.dll) driven by the reader's typed parameters (record::FgCall).
#pragma once

#include <memory>
#include <string>

#include "record/replay/generator.h"

namespace replay {

// Creates an AMD FidelityFX Frame Generation generator.
// dll is amd_fidelityfx_dx12.dll or amd_fidelityfx_loader_dx12.dll.
std::unique_ptr<FgGenerator> MakeFfxFgGenerator(const std::string& dll, const std::string& label,
                                                std::string& error);

}  // namespace replay
