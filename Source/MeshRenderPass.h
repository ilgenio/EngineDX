#pragma once

#include <filesystem>
#include "RenderList.h"

class MeshRenderPass
{
    struct PushConstants
    {
        Matrix model;
        Matrix normalMat;
    };

    ComPtr<ID3D12PipelineState>  opaquePSO;
    ComPtr<ID3D12PipelineState>  transparentPSO;

public:

    void record(ID3D12CommandList* commnadList, const RenderList& renderList);

private:

    void recordList(ID3D12CommandList* commnadList, const RenderItemList& renderList);

};