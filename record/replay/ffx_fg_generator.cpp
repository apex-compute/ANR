// AMD FidelityFX Frame Generation (FSR 3 / 3.1) generator for nr-replay.
// Drives AMD's FidelityFX API DLL (amd_fidelityfx_dx12.dll or amd_fidelityfx_loader_dx12.dll)
// using the recorded frame generation inputs and parameters (record::FgCall).

#include "record/replay/ffx_fg_generator.h"

#include <windows.h>
#include <d3d12.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "record/replay/ngx_common.h"

namespace replay {

namespace {

#pragma pack(push, 8)

// FidelityFX effect identifiers
constexpr uint32_t FFX_API_EFFECT_ID_FRAMEGENERATION = 0x00020000u;
constexpr uint32_t FfxSubId(uint32_t id, uint32_t sub) {
    return (id & 0xFFFF0000u) | (sub & 0x0000FFFFu);
}

constexpr uint32_t FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12 = 0x00000001u;
constexpr uint32_t FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION = FfxSubId(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x01);
constexpr uint32_t FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION      = FfxSubId(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x02);
constexpr uint32_t FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION       = FfxSubId(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x03);
constexpr uint32_t FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE = FfxSubId(FFX_API_EFFECT_ID_FRAMEGENERATION, 0x04);
constexpr uint32_t FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION     = 0x00000001u;

// Frame generation flags
enum FfxApiCreateContextFramegenerationFlags {
    FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT             = (1 << 0),
    FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS  = (1 << 1),
    FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION = (1 << 2),
    FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED                     = (1 << 3),
};

// Resource states
constexpr uint32_t FFX_API_RESOURCE_STATE_UNORDERED_ACCESS = 0;
constexpr uint32_t FFX_API_RESOURCE_STATE_COMPUTE_READ     = 1;

// Core descriptor headers
struct FfxApiHeader {
    uint32_t type;
    const FfxApiHeader* pNext;
};

struct FfxApiDimensions2D {
    uint32_t width;
    uint32_t height;
};

struct FfxApiFloatCoords2D {
    float x;
    float y;
};

struct FfxApiRect2D {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
};

struct FfxApiResource {
    void* resource;
    uint32_t state;
    const wchar_t* description;
};

struct FfxCreateBackendDX12Desc {
    FfxApiHeader header;
    ID3D12Device* device;
};

struct FfxCreateContextDescFrameGeneration {
    FfxApiHeader header;
    uint32_t flags;
    FfxApiDimensions2D displaySize;
    FfxApiDimensions2D maxRenderSize;
    uint32_t backBufferFormat;
};

struct FfxDispatchDescFrameGenerationPrepare {
    FfxApiHeader header;
    uint64_t frameID;
    uint32_t flags;
    void* commandList;
    FfxApiDimensions2D renderSize;
    FfxApiFloatCoords2D jitterOffset;
    FfxApiFloatCoords2D motionVectorScale;
    float frameTimeDelta;
    bool unused_reset;
    float cameraNear;
    float cameraFar;
    float cameraFovAngleVertical;
    float viewSpaceToMetersFactor;
    FfxApiResource depth;
    FfxApiResource motionVectors;
};

struct FfxDispatchDescFrameGeneration {
    FfxApiHeader header;
    void* commandList;
    FfxApiResource presentColor;
    FfxApiResource outputs[4];
    uint32_t numGeneratedFrames;
    bool reset;
    uint32_t backbufferTransferFunction;
    float minMaxLuminance[2];
    FfxApiRect2D generationRect;
    uint64_t frameID;
};

struct FfxConfigureDescFrameGeneration {
    FfxApiHeader header;
    void* swapChain;
    void* presentCallback;
    void* presentCallbackUserContext;
    void* frameGenerationCallback;
    void* frameGenerationCallbackUserContext;
    bool frameGenerationEnabled;
    bool allowAsyncWorkloads;
    FfxApiResource HUDLessColor;
    uint32_t flags;
    bool onlyPresentGenerated;
    FfxApiRect2D generationRect;
    uint64_t frameID;
};

struct FfxQueryGetProviderVersion {
    FfxApiHeader header;
    const char* versionName;
};

#pragma pack(pop)

using PfnFfxCreateContext = uint32_t (*)(void** context, const FfxApiHeader* desc, void* memCb);
using PfnFfxDestroyContext = uint32_t (*)(void** context, void* memCb);
using PfnFfxDispatch = uint32_t (*)(void** context, const FfxApiHeader* desc);
using PfnFfxQuery = uint32_t (*)(void** context, FfxApiHeader* desc);
using PfnFfxConfigure = uint32_t (*)(void** context, const FfxApiHeader* desc);

inline FfxApiResource Resource(ID3D12Resource* r, uint32_t state) {
    FfxApiResource res = {};
    res.resource = r;
    res.state = state;
    return res;
}

inline uint32_t ToFfxSurfaceFormat(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return 2;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return 3;
        case DXGI_FORMAT_R16G16B16A16_UNORM: return 4;
        case DXGI_FORMAT_R8G8B8A8_UNORM:     return 8;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:return 10;
        case DXGI_FORMAT_B8G8R8A8_UNORM:     return 12;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return 13;
        case DXGI_FORMAT_R11G11B10_FLOAT:    return 14;
        case DXGI_FORMAT_R10G10B10A2_UNORM:  return 15;
        case DXGI_FORMAT_R16G16_FLOAT:       return 16;
        case DXGI_FORMAT_R32_FLOAT:          return 25;
        default: return 0;
    }
}

inline void TransitionBarrier(ID3D12GraphicsCommandList* list, ID3D12Resource* res,
                              D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    if (!res || before == after)
        return;
    D3D12_RESOURCE_BARRIER b = {};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = res;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = before;
    b.Transition.StateAfter = after;
    list->ResourceBarrier(1, &b);
}

class FfxFgGenerator final : public FgGenerator {
public:
    FfxFgGenerator(std::string dll, std::string label)
        : dll_(std::move(dll)), label_(std::move(label)) {}

    ~FfxFgGenerator() override {
        Release();
    }

    bool Load(std::string& error) {
        std::error_code ec;
        const std::filesystem::path path = std::filesystem::absolute(dll_, ec);
        if (!std::filesystem::is_regular_file(path, ec)) {
            error = dll_ + ": no such file";
            return false;
        }
        dll_ = path.string();

        module_ = LoadLibraryExW(Wide(dll_).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!module_) {
            error = "cannot load " + dll_ + " (error " + std::to_string(GetLastError()) + ")";
            return false;
        }

        auto get = [&](const char* name) { return (void*)GetProcAddress(module_, name); };
        create_ = get("ffxCreateContext");
        destroy_ = get("ffxDestroyContext");
        dispatch_ = get("ffxDispatch");
        query_ = get("ffxQuery");
        configure_ = get("ffxConfigure");

        if (!create_ || !destroy_ || !dispatch_ || !query_) {
            error = dll_ + " lacks FidelityFX API exports (ffxCreateContext, ffxDestroyContext, ffxDispatch, ffxQuery)";
            return false;
        }
        return true;
    }

    bool Create(const FgSetup& setup, ID3D12GraphicsCommandList*, std::string& error) override {
        setup_ = setup;

        FfxCreateBackendDX12Desc backend = {};
        backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        backend.device = setup.device;

        FfxCreateContextDescFrameGeneration desc = {};
        desc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
        desc.header.pNext = &backend.header;
        desc.displaySize = {setup.width, setup.height};
        desc.maxRenderSize = {setup.render_width, setup.render_height};
        desc.backBufferFormat = ToFfxSurfaceFormat(setup.backbuffer_format);
        desc.flags = (setup.render_width == setup.width ? FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS : 0u);

        if (setup.recording && !setup.recording->calls.empty()) {
            record::FgCall first_call;
            std::string err;
            if (record::Fg(*setup.recording, setup.recording->calls[0], first_call, err)) {
                if (first_call.depth_inverted)
                    desc.flags |= FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED;
                if (first_call.motion_jittered)
                    desc.flags |= FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;
            }
        }

        void* context = nullptr;
        const uint32_t r = ((PfnFfxCreateContext)create_)(&context, &desc.header, nullptr);
        if (r != 0 || !context) {
            error = "ffxCreateContext failed with code " + std::to_string(r);
            return false;
        }
        context_ = context;

        FfxQueryGetProviderVersion version = {};
        version.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
        if (((PfnFfxQuery)query_)(&context_, &version.header) == 0 && version.versionName)
            version_ = version.versionName;

        return true;
    }

    bool Evaluate(ID3D12GraphicsCommandList* list, const FgFrame& frame, const FgInputs& inputs,
                  const FgOutputs& outputs, std::string& error) override {
        if (!context_) {
            error = "no FidelityFX Frame Generation context";
            return false;
        }
        if (outputs.generated.empty() || !outputs.generated[0]) {
            error = "no destination texture provided for generated frame";
            return false;
        }

        auto texture = [&](const char* role) -> ID3D12Resource* {
            auto it = inputs.textures.find(role);
            return it == inputs.textures.end() ? nullptr : it->second;
        };

        ID3D12Resource* backbuffer = texture("backbuffer");
        ID3D12Resource* depth = texture("depth");
        ID3D12Resource* motion = texture("motion");
        ID3D12Resource* hudless = texture("hudless");

        if (!backbuffer || !depth || !motion) {
            error = "missing required frame generation input textures (backbuffer, depth, motion)";
            return false;
        }

        const record::FgCall& call = frame.typed.empty() ? record::FgCall{} : frame.typed[0];
        ++frame_id_;

        // In nr-replay, backbuffer is in NON_PIXEL_SHADER_RESOURCE; depth, motion, hudless are in COPY_DEST.
        // Transition depth, motion, and hudless to COMPUTE_READ / NON_PIXEL_SHADER_RESOURCE for FidelityFX shaders.
        TransitionBarrier(list, depth, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionBarrier(list, motion, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        if (hudless)
            TransitionBarrier(list, hudless, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        // Optional runtime configuration for hudless / UI composition
        if (hudless && configure_) {
            FfxConfigureDescFrameGeneration conf = {};
            conf.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
            conf.frameGenerationEnabled = true;
            conf.allowAsyncWorkloads = false;
            conf.HUDLessColor = Resource(hudless, FFX_API_RESOURCE_STATE_COMPUTE_READ);
            conf.generationRect = {0, 0, (int32_t)setup_.width, (int32_t)setup_.height};
            conf.frameID = frame_id_;
            ((PfnFfxConfigure)configure_)(&context_, &conf.header);
        }

        // Pass 1: Prepare dispatch (depth & motion vectors reconstruction)
        FfxDispatchDescFrameGenerationPrepare prep = {};
        prep.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE;
        prep.frameID = frame_id_;
        prep.flags = 0;
        prep.commandList = list;
        prep.renderSize = {setup_.render_width, setup_.render_height};
        prep.jitterOffset = {call.jitter[0], call.jitter[1]};
        prep.motionVectorScale = {call.motion_scale[0], call.motion_scale[1]};
        prep.frameTimeDelta = 1000.0f / 60.0f;
        prep.cameraNear = call.near_plane > 0 ? call.near_plane : 0.1f;
        prep.cameraFar = call.far_plane > 0 ? call.far_plane : 10000.0f;
        prep.cameraFovAngleVertical = call.fov > 0 ? call.fov : 1.0472f;
        prep.viewSpaceToMetersFactor = 1.0f;
        prep.depth = Resource(depth, FFX_API_RESOURCE_STATE_COMPUTE_READ);
        prep.motionVectors = Resource(motion, FFX_API_RESOURCE_STATE_COMPUTE_READ);

        uint32_t r = ((PfnFfxDispatch)dispatch_)(&context_, &prep.header);
        if (r != 0) {
            error = "ffxDispatch (FrameGenerationPrepare) failed: " + std::to_string(r);
            return false;
        }

        // Pass 2: Optical flow frame generation dispatch
        FfxDispatchDescFrameGeneration gen = {};
        gen.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION;
        gen.commandList = list;
        gen.presentColor = Resource(backbuffer, FFX_API_RESOURCE_STATE_COMPUTE_READ);
        for (size_t i = 0; i < outputs.generated.size() && i < 4; i++) {
            gen.outputs[i] = Resource(outputs.generated[i], FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
        }
        gen.numGeneratedFrames = (uint32_t)std::min<size_t>(outputs.generated.size(), 4);
        gen.reset = frame.reset || call.reset;
        gen.backbufferTransferFunction = call.hdr ? 1 : 0;
        gen.minMaxLuminance[0] = 0.0f;
        gen.minMaxLuminance[1] = 1000.0f;
        gen.generationRect = {0, 0, (int32_t)setup_.width, (int32_t)setup_.height};
        gen.frameID = frame_id_;

        r = ((PfnFfxDispatch)dispatch_)(&context_, &gen.header);
        if (r != 0) {
            error = "ffxDispatch (FrameGeneration) failed: " + std::to_string(r);
            return false;
        }

        // Restore textures back to COPY_DEST state for nr-replay's next frame uploads
        TransitionBarrier(list, depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionBarrier(list, motion, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        if (hudless)
            TransitionBarrier(list, hudless, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

        return true;
    }

    void Release() override {
        if (context_ && destroy_) {
            void* ctx = context_;
            ((PfnFfxDestroyContext)destroy_)(&ctx, nullptr);
            context_ = nullptr;
        }
        if (module_) {
            FreeLibrary(module_);
            module_ = nullptr;
        }
    }

    nlohmann::json Describe() const override {
        const std::string name = "AMD FidelityFX Frame Generation";
        const std::string version = FileVersion(dll_);
        nlohmann::json out = {
            {"kind", "fsr"},
            {"label", label_.empty() ? (version_.empty() ? name : "FSR FG " + version_) : label_},
            {"dll", dll_},
            {"file_version", version.empty() ? nlohmann::json() : nlohmann::json(version)},
            {"sha256", FileSha256(dll_)},
        };
        if (!version_.empty())
            out["provider"] = version_;
        return out;
    }

private:
    std::string dll_, label_, version_;
    HMODULE module_ = nullptr;
    void* create_ = nullptr;
    void* destroy_ = nullptr;
    void* dispatch_ = nullptr;
    void* query_ = nullptr;
    void* configure_ = nullptr;
    void* context_ = nullptr;
    FgSetup setup_ = {};
    uint64_t frame_id_ = 0;
};

}  // namespace

std::unique_ptr<FgGenerator> MakeFfxFgGenerator(const std::string& dll, const std::string& label,
                                                std::string& error) {
    auto g = std::make_unique<FfxFgGenerator>(dll, label);
    return g->Load(error) ? std::move(g) : nullptr;
}

}  // namespace replay
