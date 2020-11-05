/* $Id$ */
/** @file
 * Gallium D3D testcase. Simple D3D11 tests.
 */

/*
 * Copyright (C) 2017-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "d3d11render.h"

/*
 * Clear the backbuffer and display it.
 */

class D3D11RenderClear: public D3D11Render
{
public:
    D3D11RenderClear() {}
    virtual ~D3D11RenderClear() {}
    virtual HRESULT InitRender(D3D11DeviceProvider *pDP);
    virtual HRESULT DoRender(D3D11DeviceProvider *pDP);
};

HRESULT D3D11RenderClear::InitRender(D3D11DeviceProvider *pDP)
{
    (void)pDP;
    return S_OK;
}

HRESULT D3D11RenderClear::DoRender(D3D11DeviceProvider *pDP)
{
    ID3D11DeviceContext *pImmediateContext = pDP->ImmediateContext();

    FLOAT aColor[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    pImmediateContext->ClearRenderTargetView(pDP->RenderTargetView(), aColor);

    return S_OK;
}

/*
 * Simplest colorful triangle using shaders.
 */

class D3D11RenderTriangleShader: public D3D11Render
{
public:
    D3D11RenderTriangleShader();
    virtual ~D3D11RenderTriangleShader();
    virtual HRESULT InitRender(D3D11DeviceProvider *pDP);
    virtual HRESULT DoRender(D3D11DeviceProvider *pDP);
private:
    ID3D11InputLayout *mpInputLayout;
    ID3D11VertexShader *mpVS;
    ID3D11PixelShader *mpPS;
    ID3D11Buffer *mpVB;

    struct Vertex
    {
        float position[3];
        float color[4];
    };
};


D3D11RenderTriangleShader::D3D11RenderTriangleShader()
    : mpInputLayout(0)
    , mpVS(0)
    , mpPS(0)
    , mpVB(0)
{
}

D3D11RenderTriangleShader::~D3D11RenderTriangleShader()
{
    D3D_RELEASE(mpInputLayout);
    D3D_RELEASE(mpVS);
    D3D_RELEASE(mpPS);
    D3D_RELEASE(mpVB);
}

HRESULT D3D11RenderTriangleShader::InitRender(D3D11DeviceProvider *pDP)
{
    ID3D11Device *pDevice = pDP->Device();

    static D3D11_INPUT_ELEMENT_DESC const aVertexDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    #if 0
    //
    // Generated by Microsoft (R) HLSL Shader Compiler 9.29.952.3111
    //
    //
    //   fxc /Fhd3d11color.hlsl.vs.h /Evs_color /Tvs_4_0 d3d11color.hlsl
    //
    //
    //
    // Input signature:
    //
    // Name                 Index   Mask Register SysValue Format   Used
    // -------------------- ----- ------ -------- -------- ------ ------
    // POSITION                 0   xyz         0     NONE  float   xyz
    // COLOR                    0   xyzw        1     NONE  float   xyzw
    //
    //
    // Output signature:
    //
    // Name                 Index   Mask Register SysValue Format   Used
    // -------------------- ----- ------ -------- -------- ------ ------
    // SV_POSITION              0   xyzw        0      POS  float   xyzw
    // COLOR                    0   xyzw        1     NONE  float   xyzw
    //
    vs_4_0
    dcl_input v0.xyz
    dcl_input v1.xyzw
    dcl_output_siv o0.xyzw, position
    dcl_output o1.xyzw
    mov o0.xyz, v0.xyzx
    mov o0.w, l(1.000000)
    mov o1.xyzw, v1.xyzw
    ret
    // Approximately 4 instruction slots used
    #endif

    static const BYTE g_vs_color[] =
    {
         68,  88,  66,  67, 109, 138, 105,  83,  86, 190,  83, 125,  72, 102, 194, 136,  46,  69,
         17, 121,   1,   0,   0,   0,  48,   2,   0,   0,   5,   0,   0,   0,  52,   0,   0,   0,
        140,   0,   0,   0, 220,   0,   0,   0,  48,   1,   0,   0, 180,   1,   0,   0,  82,  68,
         69,  70,  80,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         28,   0,   0,   0,   0,   4, 254, 255,   0,   1,   0,   0,  28,   0,   0,   0,  77, 105,
         99, 114, 111, 115, 111, 102, 116,  32,  40,  82,  41,  32,  72,  76,  83,  76,  32,  83,
        104,  97, 100, 101, 114,  32,  67, 111, 109, 112, 105, 108, 101, 114,  32,  57,  46,  50,
         57,  46,  57,  53,  50,  46,  51,  49,  49,  49,   0, 171, 171, 171,  73,  83,  71,  78,
         72,   0,   0,   0,   2,   0,   0,   0,   8,   0,   0,   0,  56,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   7,   7,   0,   0,
         65,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   1,   0,
          0,   0,  15,  15,   0,   0,  80,  79,  83,  73,  84,  73,  79,  78,   0,  67,  79,  76,
         79,  82,   0, 171,  79,  83,  71,  78,  76,   0,   0,   0,   2,   0,   0,   0,   8,   0,
          0,   0,  56,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   3,   0,   0,   0,
          0,   0,   0,   0,  15,   0,   0,   0,  68,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   3,   0,   0,   0,   1,   0,   0,   0,  15,   0,   0,   0,  83,  86,  95,  80,
         79,  83,  73,  84,  73,  79,  78,   0,  67,  79,  76,  79,  82,   0, 171, 171,  83,  72,
         68,  82, 124,   0,   0,   0,  64,   0,   1,   0,  31,   0,   0,   0,  95,   0,   0,   3,
        114,  16,  16,   0,   0,   0,   0,   0,  95,   0,   0,   3, 242,  16,  16,   0,   1,   0,
          0,   0, 103,   0,   0,   4, 242,  32,  16,   0,   0,   0,   0,   0,   1,   0,   0,   0,
        101,   0,   0,   3, 242,  32,  16,   0,   1,   0,   0,   0,  54,   0,   0,   5, 114,  32,
         16,   0,   0,   0,   0,   0,  70,  18,  16,   0,   0,   0,   0,   0,  54,   0,   0,   5,
        130,  32,  16,   0,   0,   0,   0,   0,   1,  64,   0,   0,   0,   0, 128,  63,  54,   0,
          0,   5, 242,  32,  16,   0,   1,   0,   0,   0,  70,  30,  16,   0,   1,   0,   0,   0,
         62,   0,   0,   1,  83,  84,  65,  84, 116,   0,   0,   0,   4,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0
    };

    #if 0
    //
    // Generated by Microsoft (R) HLSL Shader Compiler 9.29.952.3111
    //
    //
    //   fxc /Fhd3d11color.hlsl.ps.h /Eps_color /Tps_4_0 d3d11color.hlsl
    //
    //
    //
    // Input signature:
    //
    // Name                 Index   Mask Register SysValue Format   Used
    // -------------------- ----- ------ -------- -------- ------ ------
    // SV_POSITION              0   xyzw        0      POS  float
    // COLOR                    0   xyzw        1     NONE  float   xyzw
    //
    //
    // Output signature:
    //
    // Name                 Index   Mask Register SysValue Format   Used
    // -------------------- ----- ------ -------- -------- ------ ------
    // SV_TARGET                0   xyzw        0   TARGET  float   xyzw
    //
    ps_4_0
    dcl_input_ps linear v1.xyzw
    dcl_output o0.xyzw
    mov o0.xyzw, v1.xyzw
    ret
    // Approximately 2 instruction slots used
    #endif

    static const BYTE g_ps_color[] =
    {
         68,  88,  66,  67, 206, 120, 117, 238, 118, 127,  10,  87,  80,  75, 114, 198,  95,   2,
        120, 102,   1,   0,   0,   0, 208,   1,   0,   0,   5,   0,   0,   0,  52,   0,   0,   0,
        140,   0,   0,   0, 224,   0,   0,   0,  20,   1,   0,   0,  84,   1,   0,   0,  82,  68,
         69,  70,  80,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         28,   0,   0,   0,   0,   4, 255, 255,   0,   1,   0,   0,  28,   0,   0,   0,  77, 105,
         99, 114, 111, 115, 111, 102, 116,  32,  40,  82,  41,  32,  72,  76,  83,  76,  32,  83,
        104,  97, 100, 101, 114,  32,  67, 111, 109, 112, 105, 108, 101, 114,  32,  57,  46,  50,
         57,  46,  57,  53,  50,  46,  51,  49,  49,  49,   0, 171, 171, 171,  73,  83,  71,  78,
         76,   0,   0,   0,   2,   0,   0,   0,   8,   0,   0,   0,  56,   0,   0,   0,   0,   0,
          0,   0,   1,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,  15,   0,   0,   0,
         68,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   1,   0,
          0,   0,  15,  15,   0,   0,  83,  86,  95,  80,  79,  83,  73,  84,  73,  79,  78,   0,
         67,  79,  76,  79,  82,   0, 171, 171,  79,  83,  71,  78,  44,   0,   0,   0,   1,   0,
          0,   0,   8,   0,   0,   0,  32,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          3,   0,   0,   0,   0,   0,   0,   0,  15,   0,   0,   0,  83,  86,  95,  84,  65,  82,
         71,  69,  84,   0, 171, 171,  83,  72,  68,  82,  56,   0,   0,   0,  64,   0,   0,   0,
         14,   0,   0,   0,  98,  16,   0,   3, 242,  16,  16,   0,   1,   0,   0,   0, 101,   0,
          0,   3, 242,  32,  16,   0,   0,   0,   0,   0,  54,   0,   0,   5, 242,  32,  16,   0,
          0,   0,   0,   0,  70,  30,  16,   0,   1,   0,   0,   0,  62,   0,   0,   1,  83,  84,
         65,  84, 116,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
    };

    static Vertex const aVertices[] =
    {
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, },
        { {  0.0f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, },
        { {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, },
    };

    HRESULT hr = S_OK;

    HTEST(pDevice->CreateInputLayout(aVertexDesc, RT_ELEMENTS(aVertexDesc),
          &g_vs_color[0], sizeof(g_vs_color),
          &mpInputLayout));
    HTEST(pDevice->CreateVertexShader(g_vs_color, sizeof(g_vs_color), NULL, &mpVS));
    HTEST(pDevice->CreatePixelShader(g_ps_color, sizeof(g_ps_color), NULL, &mpPS));

    D3D11_BUFFER_DESC vbd;
    RT_ZERO(vbd);
    vbd.Usage               = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth           = sizeof(aVertices);
    vbd.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags      = 0;
    vbd.MiscFlags           = 0;
    vbd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vinitData;
    RT_ZERO(vinitData);
    vinitData.pSysMem = aVertices;

    HTEST(pDevice->CreateBuffer(&vbd, &vinitData, &mpVB));

    return hr;
}

HRESULT D3D11RenderTriangleShader::DoRender(D3D11DeviceProvider *pDP)
{
    ID3D11DeviceContext *pImmediateContext = pDP->ImmediateContext();

    HRESULT hr = S_OK;

    FLOAT aColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    pImmediateContext->ClearRenderTargetView(pDP->RenderTargetView(), aColor);

    pImmediateContext->IASetInputLayout(mpInputLayout);
    pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    pImmediateContext->IASetVertexBuffers(0, 1, &mpVB, &stride, &offset);

    pImmediateContext->VSSetShader(mpVS, NULL, 0);
    pImmediateContext->PSSetShader(mpPS, NULL, 0);

    D3D11_VIEWPORT Viewport;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width    = (float)800;
    Viewport.Height   = (float)600;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    pImmediateContext->RSSetViewports(1, &Viewport);

    pImmediateContext->Draw(3, 0);

    return S_OK;
}

/*
 * "Public" interface.
 */

D3D11Render *CreateRender(int iRenderId)
{
    switch (iRenderId)
    {
        case 0:
            return new D3D11RenderClear();
        case 1:
            return new D3D11RenderTriangleShader();
        default:
            break;
    }
    return 0;
}

void DeleteRender(D3D11Render *pRender)
{
    if (pRender)
        delete pRender;
}
