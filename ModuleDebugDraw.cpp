#define DEBUG_DRAW_IMPLEMENTATION
#define DEBUG_DRAW_EXPLICIT_CONTEXT
#include "debug_draw.hpp"

#include "Globals.h"

class RenderInterfaceD3D12 final : public dd::RenderInterface
{
public:

    RenderInterfaceD3D12()
    {
        initPSOs();
        initBuffers();
    }

    void setMvpMatrixPtr(const float * const mtx)
    {
        // TODO: Ojo esto no tiene sentido!!!
        constantBufferData.mvpMatrix = DirectX::XMMATRIX(mtx);
    }

    void setCameraFrame(const XMFLOAT3& up, const XMFLOAT3& right, const XMFLOAT3& origin)
    {
        camUp = up; camRight = right; camOrigin = origin;
    }

    //
    // dd::RenderInterface overrides:
    //

    void beginDraw() override
    {
        // TODO:
#if 0
        // Update and set the constant buffer for this frame
        deviceContext->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &constantBufferData, 0, 0);
        deviceContext->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

        // Disable culling for the screen text
        deviceContext->RSSetState(rasterizerState.Get());
#endif 
    }

    void endDraw() override
    {
        // No work done here at the moment.
    }

    dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void * pixels) override
    {
        /*
        UINT numQualityLevels = 0;
        d3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8_UNORM, 1, &numQualityLevels);

        D3D12_TEXTURE2D_DESC tex2dDesc  = {};
        tex2dDesc.Usage                 = D3D11_USAGE_DEFAULT;
        tex2dDesc.BindFlags             = D3D11_BIND_SHADER_RESOURCE;
        tex2dDesc.Format                = DXGI_FORMAT_R8_UNORM;
        tex2dDesc.Width                 = width;
        tex2dDesc.Height                = height;
        tex2dDesc.MipLevels             = 1;
        tex2dDesc.ArraySize             = 1;
        tex2dDesc.SampleDesc.Count      = 1;
        tex2dDesc.SampleDesc.Quality    = numQualityLevels - 1;

        D3D12_SAMPLER_DESC samplerDesc  = {};
        samplerDesc.Filter              = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU            = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV            = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW            = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc      = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MaxAnisotropy       = 1;
        samplerDesc.MipLODBias          = 0.0f;
        samplerDesc.MinLOD              = 0.0f;
        samplerDesc.MaxLOD              = D3D11_FLOAT32_MAX;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = pixels;
        initData.SysMemPitch            = width;

        auto * texImpl = new TextureImpl{};

        if (FAILED(d3dDevice->CreateTexture2D(&tex2dDesc, &initData, &texImpl->d3dTexPtr)))
        {
            errorF("CreateTexture2D failed!");
            destroyGlyphTexture(texImpl);
            return nullptr;
        }
        if (FAILED(d3dDevice->CreateShaderResourceView(texImpl->d3dTexPtr, nullptr, &texImpl->d3dTexSRV)))
        {
            errorF("CreateShaderResourceView failed!");
            destroyGlyphTexture(texImpl);
            return nullptr;
        }
        if (FAILED(d3dDevice->CreateSamplerState(&samplerDesc, &texImpl->d3dSampler)))
        {
            errorF("CreateSamplerState failed!");
            destroyGlyphTexture(texImpl);
            return nullptr;
        }

        return static_cast<dd::GlyphTextureHandle>(texImpl);
        */
    }

    void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override
    {
        /*
        auto * texImpl = static_cast<TextureImpl *>(glyphTex);
        if (texImpl)
        {
            if (texImpl->d3dSampler) { texImpl->d3dSampler->Release(); }
            if (texImpl->d3dTexSRV)  { texImpl->d3dTexSRV->Release();  }
            if (texImpl->d3dTexPtr)  { texImpl->d3dTexPtr->Release();  }
            delete texImpl;
        }
        */
    }

    void drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex) override
    {
        /*
        assert(glyphs != nullptr);
        assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

        auto * texImpl = static_cast<TextureImpl *>(glyphTex);
        assert(texImpl != nullptr);

        // Map the vertex buffer:
        D3D11_MAPPED_SUBRESOURCE mapInfo;
        if (FAILED(deviceContext->Map(glyphVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo)))
        {
            panicF("Failed to map vertex buffer!");
        }

        // Copy into mapped buffer:
        auto * verts = static_cast<Vertex *>(mapInfo.pData);
        for (int v = 0; v < count; ++v)
        {
            verts[v].pos.x = glyphs[v].glyph.x;
            verts[v].pos.y = glyphs[v].glyph.y;
            verts[v].pos.z = 0.0f;
            verts[v].pos.w = 1.0f;

            verts[v].uv.x = glyphs[v].glyph.u;
            verts[v].uv.y = glyphs[v].glyph.v;
            verts[v].uv.z = 0.0f;
            verts[v].uv.w = 0.0f;

            verts[v].color.x = glyphs[v].glyph.r;
            verts[v].color.y = glyphs[v].glyph.g;
            verts[v].color.z = glyphs[v].glyph.b;
            verts[v].color.w = 1.0f;
        }

        // Unmap and draw:
        deviceContext->Unmap(glyphVertexBuffer.Get(), 0);

        // Bind texture & sampler (t0, s0):
        deviceContext->PSSetShaderResources(0, 1, &texImpl->d3dTexSRV);
        deviceContext->PSSetSamplers(0, 1, &texImpl->d3dSampler);

        const float blendFactor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        deviceContext->OMSetBlendState(blendStateText.Get(), blendFactor, 0xFFFFFFFF);

        // Draw with the current buffer:
        drawHelper(count, glyphShaders, glyphVertexBuffer.Get(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Restore default blend state.
        deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        */
    }

    void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled) override
    {
        /*
        (void)depthEnabled; // TODO: not implemented yet - not required by this sample

        // Emulating points as billboarded quads, so each point will use 6 vertexes.
        // D3D11 doesn't support "point sprites" like OpenGL (gl_PointSize).
        const int maxVerts = DEBUG_DRAW_VERTEX_BUFFER_SIZE / 6;

        // OpenGL point size scaling produces gigantic points with the billboarding fallback.
        // This is some arbitrary down-scaling factor to more or less match the OpenGL samples.
        const float D3DPointSpriteScalingFactor = 0.01f;

        assert(points != nullptr);
        assert(count > 0 && count <= maxVerts);

       // Map the vertex buffer:
        D3D11_MAPPED_SUBRESOURCE mapInfo;
        if (FAILED(deviceContext->Map(pointVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo)))
        {
            panicF("Failed to map vertex buffer!");
        }

        const int numVerts = count * 6;
        const int indexes[6] = {0, 1, 2, 2, 3, 0};

        int v = 0;
        auto * verts = static_cast<Vertex *>(mapInfo.pData);

        // Expand each point into a quad:
        for (int p = 0; p < count; ++p)
        {
            const float ptSize      = points[p].point.size * D3DPointSpriteScalingFactor;
            const Vector3 halfWidth = (ptSize * 0.5f) * camRight; // X
            const Vector3 halfHeigh = (ptSize * 0.5f) * camUp;    // Y
            const Vector3 origin    = Vector3{ points[p].point.x, points[p].point.y, points[p].point.z };

            Vector3 corners[4];
            corners[0] = origin + halfWidth + halfHeigh;
            corners[1] = origin - halfWidth + halfHeigh;
            corners[2] = origin - halfWidth - halfHeigh;
            corners[3] = origin + halfWidth - halfHeigh;

            for (int i : indexes)
            {
                verts[v].pos.x = corners[i].getX();
                verts[v].pos.y = corners[i].getY();
                verts[v].pos.z = corners[i].getZ();
                verts[v].pos.w = 1.0f;

                verts[v].color.x = points[p].point.r;
                verts[v].color.y = points[p].point.g;
                verts[v].color.z = points[p].point.b;
                verts[v].color.w = 1.0f;

                ++v;
            }
        }
        assert(v == numVerts);

        // Unmap and draw:
        deviceContext->Unmap(pointVertexBuffer.Get(), 0);

        // Draw with the current buffer:
        drawHelper(numVerts, pointShaders, pointVertexBuffer.Get(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        */
    }

    void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled) override
    {
        /*
        (void)depthEnabled; // TODO: not implemented yet - not required by this sample

        assert(lines != nullptr);
        assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

        // Map the vertex buffer:
        D3D11_MAPPED_SUBRESOURCE mapInfo;
        if (FAILED(deviceContext->Map(lineVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo)))
        {
            panicF("Failed to map vertex buffer!");
        }

        // Copy into mapped buffer:
        auto * verts = static_cast<Vertex *>(mapInfo.pData);
        for (int v = 0; v < count; ++v)
        {
            verts[v].pos.x = lines[v].line.x;
            verts[v].pos.y = lines[v].line.y;
            verts[v].pos.z = lines[v].line.z;
            verts[v].pos.w = 1.0f;

            verts[v].color.x = lines[v].line.r;
            verts[v].color.y = lines[v].line.g;
            verts[v].color.z = lines[v].line.b;
            verts[v].color.w = 1.0f;
        }

        // Unmap and draw:
        deviceContext->Unmap(lineVertexBuffer.Get(), 0);

        // Draw with the current buffer:
        drawHelper(count, lineShaders, lineVertexBuffer.Get(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        */
    }

private:

    //
    // Local types:
    //

    struct ConstantBufferData
    {
        DirectX::XMMATRIX mvpMatrix        = DirectX::XMMatrixIdentity();
        DirectX::XMFLOAT4 screenDimensions = {float(WindowWidth), float(WindowHeight), 0.0f, 0.0f};
    };

    struct Vertex
    {
        DirectX::XMFLOAT4A pos;   // 3D position
        DirectX::XMFLOAT4A uv;    // Texture coordinates
        DirectX::XMFLOAT4A color; // RGBA float
    };

    struct TextureImpl : public dd::OpaqueTextureType
    {
        ID3D11Texture2D*           d3dTexPtr  = nullptr;
        ID3D11ShaderResourceView*  d3dTexSRV  = nullptr;
        ID3D11SamplerState*        d3dSampler = nullptr;
    };

    ComPtr<ID3D11Buffer>          constantBuffer;
    ConstantBufferData            constantBufferData;

    ComPtr<ID3D11Buffer>          lineVertexBuffer;
    ComPtr<ID3D11Buffer>          pointVertexBuffer;
    ComPtr<ID3D11Buffer>          glyphVertexBuffer;

    ComPtr<ID3DBlob>              lineVS;
    ComPtr<ID3DBlob>              linePS;
    ComPtr<ID3DBlob>              pointVS;
    ComPtr<ID3DBlob>              pointPS;
    ComPtr<ID3DBlob>              textVS;
    ComPtr<ID3DBlob>              textPS;

    ComPtr<ID3D12PipelineState>   linePSO;
    ComPtr<ID3D12PipelineState>   pointPSO;
    ComPtr<ID3D12PipelineState>   textPSO;


    // Camera vectors for the emulated point sprites
    XMFLOAT3                      camUp     = {0.0f, 0.0f, 0.0f};
    XMFLOAT3                      camRight  = {0.0f, 0.0f, 0.0f};
    XMFLOAT3                      camOrigin = {0.0f, 0.0f, 0.0f};

    bool loadShader(const wchart_t* file, const char* vertexFunc, const char* pixelFunc, unsigned flags, 
                    ComPtr<ID3DBlob>& vertex, ComPtr<ID3DBlob>& pixel)
    {
        ComPtr<ID3DBlob> errorBuff;

        if (FAILED(D3DCompileFromFile(file, nullptr, nullptr, vertexFunc, "vs_5_0", flags, 0, &vertex, &errorBuff)))
        {
            OutputDebugStringA((char *)errorBuff->GetBufferPointer());
            return false;
        }

        if (FAILED(D3DCompileFromFile(file, nullptr, nullptr, pixelFunc, "ps_5_0", flags, 0, &pixel, &errorBuff)))
        {
            OutputDebugStringA((char *)errorBuff->GetBufferPointer());
            return false;
        }

        return true;
    }

    bool initRootSignature()
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        CD3DX12_ROOT_PARAMETER rootParameters[2];

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[0].Constants.Num32BitValues = sizeof(ConstantBufferData) / sizeof(UINT32);
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].Constants.ShaderRegister = 0;

        D3D12_DESCRIPTOR_RANGE tableRange;

        tableRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        tableRange.NumDescriptors = 1;
        tableRange.BaseShaderRegister = 0;
        tableRange.RegisterSpace = 0;
        tableRange.OffsetInDescriptorsFromTableStart = 0;

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[1].DescriptorTable.pDescriptorRanges = &tableRange;

        D3D12_STATIC_SAMPLER_DESC sampler;

        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        rootSignatureDesc.Init(2, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> rootSignatureBlob;

        if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr)))
        {
            return false;
        }

        if (FAILED(app->getRender()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
        {
            return false;
        }

        return true;
    }

    bool initPSO(const wchart_t* file, const char* vertexFunc, const char* pixelFunc, ComPtr<ID3DBlob>& vertex, 
                 ComPtr<ID3DBlob>& pixel, ComPtr<ID3D12PipelineState>& pso)
    {
#ifdef _DEBUG
        unsigned flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        unsigned flags = 0;
#endif

        bool ok = loadShader(file, vertexFunc, pixelFunc, flags, vertex, pixel);

        if(ok)
        {
            // Same vertex format used by all buffers to simplify things.
            const D3D12_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
            inputLayoutDesc.NumElements = sizeof(layout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
            inputLayoutDesc.pInputElementDescs = layout;

            D3D12_RASTERIZER_DESC rsDesc = {};
            rsDesc.FillMode              = D3D12_FILL_SOLID;
            rsDesc.CullMode              = D3D12_CULL_NONE;
            rsDesc.FrontCounterClockwise = true;
            rsDesc.DepthBias             = 0;
            rsDesc.DepthBiasClamp        = 0.0f;
            rsDesc.SlopeScaledDepthBias  = 0.0f;
            rsDesc.DepthClipEnable       = false;
            rsDesc.ScissorEnable         = false;
            rsDesc.MultisampleEnable     = false;
            rsDesc.AntialiasedLineEnable = false;

            D3D12_BLEND_DESC bsDesc                      = {};
            bsDesc.RenderTarget[0].BlendEnable           = TRUE;
            bsDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            bsDesc.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
            bsDesc.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
            bsDesc.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
            bsDesc.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
            bsDesc.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_ZERO;
            bsDesc.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
            bsDesc.RenderTarget[0].LogicOp               = D3D12_LOGIC_OP_NOOP;

            D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
            vertexShaderBytecode.BytecodeLength = vertex->GetBufferSize();
            vertexShaderBytecode.pShaderBytecode = vertex->GetBufferPointer();

            D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
            pixelShaderBytecode.BytecodeLength = pixel->GetBufferSize();
            pixelShaderBytecode.pShaderBytecode = pixel->GetBufferPointer();

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = inputLayoutDesc;                                  
            psoDesc.pRootSignature = rootSignature.Get();                           
            psoDesc.VS = vertexShaderBytecode;                                      
            psoDesc.PS = pixelShaderBytecode;                                       
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // TODO: Ojo!!!!
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                     
            psoDesc.SampleDesc = {1, 0};                                            
            psoDesc.SampleMask = 0xffffffff;                                        
            psoDesc.RasterizerState = rsDesc;
            psoDesc.BlendState = bsDesc;
            psoDesc.NumRenderTargets = 1;                                           

            ok = SUCCEEDED(app->getRender()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
        }

        return ok;
    }

    bool initPSOs()
    {
        bool ok = initRootSignature();

        ok = ok && initPSO(L"Shaders/ddShader/hlsl", "VS_LinePoint", "PS_LinePoint", lineVS, linePS, linePSO);
        ok = ok && initPSO(L"Shaders/ddShader/hlsl", "VS_LinePoint", "PS_LinePoint", pointVS, pointPS, pointPSO);
        ok = ok && initPSO(L"Shaders/ddShader/hlsl", "VS_TextGlyph", "PS_TextGlyph", textVS, textPS, textPSO);

        return ok;
    }

    void initBuffers()
    {
#if 0
        D3D11_BUFFER_DESC bd;

        // Create the shared constant buffer:
        bd                = {};
        bd.Usage          = D3D11_USAGE_DEFAULT;
        bd.ByteWidth      = sizeof(ConstantBufferData);
        bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, constantBuffer.GetAddressOf())))
        {
            panicF("Failed to create shader constant buffer!");
        }

        // Create the vertex buffers for lines/points/glyphs:
        bd                = {};
        bd.Usage          = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth      = sizeof(Vertex) * DEBUG_DRAW_VERTEX_BUFFER_SIZE;
        bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, lineVertexBuffer.GetAddressOf())))
        {
            panicF("Failed to create lines vertex buffer!");
        }
        if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, pointVertexBuffer.GetAddressOf())))
        {
            panicF("Failed to create points vertex buffer!");
        }
        if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, glyphVertexBuffer.GetAddressOf())))
        {
            panicF("Failed to create glyphs vertex buffer!");
        }
#endif 
    }

#if 0
    bool createBuffer(void *bufferData, unsigned bufferSize, ComPtr<ID3D12Resource> &buffer, ComPtr<ID3D12Resource> &upload)
    {
        ModuleRender *render = app->getRender();
        ID3D12Device2 *device = render->getDevice();

        bool ok = SUCCEEDED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&buffer)));

        ok = ok && SUCCEEDED(device->CreateCommittedResource(
                       &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                       D3D12_HEAP_FLAG_NONE,
                       &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                       D3D12_RESOURCE_STATE_GENERIC_READ,
                       nullptr,
                       IID_PPV_ARGS(&upload)));

        ID3D12GraphicsCommandList *commandList = render->getCommandList();
        ok = ok && SUCCEEDED(commandList->Reset(render->getCommandAllocator(), nullptr));

        if (ok)
        {
            // Copy to intermediate
            BYTE *pData = nullptr;
            upload->Map(0, nullptr, reinterpret_cast<void **>(&pData));
            memcpy(pData, bufferData, bufferSize);
            upload->Unmap(0, nullptr);

            // Copy to vram
            commandList->CopyBufferRegion(buffer.Get(), 0, upload.Get(), 0, bufferSize);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
            commandList->Close();

            render->executeCommandList();
        }

        return ok;
    }
#endif 

    void drawHelper(const int numVerts, const ShaderSetD3D11 & ss, ID3D11Buffer * vb, const D3D11_PRIMITIVE_TOPOLOGY topology)
    {
#if 0
        const UINT offset = 0;
        const UINT stride = sizeof(Vertex);
        deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        deviceContext->IASetPrimitiveTopology(topology);
        deviceContext->IASetInputLayout(ss.vertexLayout.Get());
        deviceContext->VSSetShader(ss.vs.Get(), nullptr, 0);
        deviceContext->PSSetShader(ss.ps.Get(), nullptr, 0);
        deviceContext->Draw(numVerts, 0);
#endif 
    }
};

// ========================================================
// Sample drawing
// ========================================================

static void drawGrid(dd::ContextHandle ctx)
{
    if (!keys.showGrid)
    {
        return;
    }

    // Grid from -50 to +50 in both X & Z
    dd::xzSquareGrid(ctx, -50.0f, 50.0f, -1.0f, 1.7f, dd::colors::Green);
}

static void drawLabel(dd::ContextHandle ctx, ddVec3_In pos, const char * name)
{
    if (!keys.showLabels)
    {
        return;
    }

    // Only draw labels inside the camera frustum.
    if (camera.isPointInsideFrustum(pos[0], pos[1], pos[2]))
    {
        const ddVec3 textColor = { 0.8f, 0.8f, 1.0f };
        dd::projectedText(ctx, name, pos, textColor, toFloatPtr(camera.vpMatrix),
                          0, 0, WindowWidth, WindowHeight, 0.5f);
    }
}

static void drawMiscObjects(dd::ContextHandle ctx)
{
    // Start a row of objects at this position:
    ddVec3 origin = { -15.0f, 0.0f, 0.0f };

    // Box with a point at it's center:
    drawLabel(ctx, origin, "box");
    dd::box(ctx, origin, dd::colors::Blue, 1.5f, 1.5f, 1.5f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);
    origin[0] += 3.0f;

    // Sphere with a point at its center
    drawLabel(ctx, origin, "sphere");
    dd::sphere(ctx, origin, dd::colors::Red, 1.0f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);
    origin[0] += 4.0f;

    // Two cones, one open and one closed:
    const ddVec3 condeDir = { 0.0f, 2.5f, 0.0f };
    origin[1] -= 1.0f;

    drawLabel(ctx, origin, "cone (open)");
    dd::cone(ctx, origin, condeDir, dd::colors::Yellow, 1.0f, 2.0f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);
    origin[0] += 4.0f;

    drawLabel(ctx, origin, "cone (closed)");
    dd::cone(ctx, origin, condeDir, dd::colors::Cyan, 0.0f, 1.0f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);
    origin[0] += 4.0f;

    // Axis-aligned bounding box:
    const ddVec3 bbMins = { -1.0f, -0.9f, -1.0f };
    const ddVec3 bbMaxs = {  1.0f,  2.2f,  1.0f };
    const ddVec3 bbCenter = {
        (bbMins[0] + bbMaxs[0]) * 0.5f,
        (bbMins[1] + bbMaxs[1]) * 0.5f,
        (bbMins[2] + bbMaxs[2]) * 0.5f
    };
    drawLabel(ctx, origin, "AABB");
    dd::aabb(ctx, bbMins, bbMaxs, dd::colors::Orange);
    dd::point(ctx, bbCenter, dd::colors::White, 15.0f);

    // Move along the Z for another row:
    origin[0] = -15.0f;
    origin[2] += 5.0f;

    // A big arrow pointing up:
    const ddVec3 arrowFrom = { origin[0], origin[1], origin[2] };
    const ddVec3 arrowTo   = { origin[0], origin[1] + 5.0f, origin[2] };
    drawLabel(ctx, arrowFrom, "arrow");
    dd::arrow(ctx, arrowFrom, arrowTo, dd::colors::Magenta, 1.0f);
    dd::point(ctx, arrowFrom, dd::colors::White, 15.0f);
    dd::point(ctx, arrowTo, dd::colors::White, 15.0f);
    origin[0] += 4.0f;

    // Plane with normal vector:
    const ddVec3 planeNormal = { 0.0f, 1.0f, 0.0f };
    drawLabel(ctx, origin, "plane");
    dd::plane(ctx, origin, planeNormal, dd::colors::Yellow, dd::colors::Blue, 1.5f, 1.0f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);
    origin[0] += 4.0f;

    // Circle on the Y plane:
    drawLabel(ctx, origin, "circle");
    dd::circle(ctx, origin, planeNormal, dd::colors::Orange, 1.5f, 15.0f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);
    origin[0] += 3.2f;

    // Tangent basis vectors:
    const ddVec3 normal    = { 0.0f, 1.0f, 0.0f };
    const ddVec3 tangent   = { 1.0f, 0.0f, 0.0f };
    const ddVec3 bitangent = { 0.0f, 0.0f, 1.0f };
    origin[1] += 0.1f;
    drawLabel(ctx, origin, "tangent basis");
    dd::tangentBasis(ctx, origin, normal, tangent, bitangent, 2.5f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);

    // And a set of intersecting axes:
    origin[0] += 4.0f;
    origin[1] += 1.0f;
    drawLabel(ctx, origin, "cross");
    dd::cross(ctx, origin, 2.0f);
    dd::point(ctx, origin, dd::colors::White, 15.0f);
}

static void drawFrustum(dd::ContextHandle ctx)
{
    const ddVec3 color  = {  0.8f, 0.3f, 1.0f  };
    const ddVec3 origin = { -8.0f, 0.5f, 14.0f };
    drawLabel(ctx, origin, "frustum + axes");

    // The frustum will depict a fake camera:
    const Matrix4 proj = Matrix4::perspective(degToRad(45.0f), 800.0f / 600.0f, 0.5f, 4.0f);
    const Matrix4 view = Matrix4::lookAt(Point3(-8.0f, 0.5f, 14.0f), Point3(-8.0f, 0.5f, -14.0f), Vector3::yAxis());
    const Matrix4 clip = inverse(proj * view);
    dd::frustum(ctx, toFloatPtr(clip), color);

    // A white dot at the eye position:
    dd::point(ctx, origin, dd::colors::White, 15.0f);

    // A set of arrows at the camera's origin/eye:
    const Matrix4 transform = Matrix4::translation(Vector3(-8.0f, 0.5f, 14.0f)) * Matrix4::rotationZ(degToRad(60.0f));
    dd::axisTriad(ctx, toFloatPtr(transform), 0.3f, 2.0f);
}

static void drawText(dd::ContextHandle ctx)
{
    // HUD text:
    const ddVec3 textColor = { 1.0f,  1.0f,  1.0f };
    const ddVec3 textPos2D = { 10.0f, 15.0f, 0.0f };
    dd::screenText(ctx, "Welcome to the D3D11 Debug Draw demo.\n\n"
                        "[SPACE]  to toggle labels on/off\n"
                        "[RETURN] to toggle grid on/off",
                        textPos2D, textColor, 0.55f);
}
