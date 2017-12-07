#include "pch.h"

#include "CubeRenderer.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

CubeRenderer::CubeRenderer()
  : m_loadingComplete(false)
  , m_indexCount(0)
{
}

void CubeRenderer::CreateDeviceResources()
{
  Direct3DBase::CreateDeviceResources();

  auto loadVSTask = DX::ReadDataAsync("SimpleVertexShader.cso");
  auto loadPSTask = DX::ReadDataAsync("SimplePixelShader.cso");

  auto createVSTask =
    loadVSTask.then([this](Platform::Array<byte> ^ fileData) {
      DX::ThrowIfFailed(m_d3dDevice->CreateVertexShader(
        fileData->Data, fileData->Length, nullptr, &m_vertexShader));

      const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
      };

      DX::ThrowIfFailed(m_d3dDevice->CreateInputLayout(
        vertexDesc, ARRAYSIZE(vertexDesc), fileData->Data, fileData->Length,
        &m_inputLayout));
    });

  auto createPSTask =
    loadPSTask.then([this](Platform::Array<byte> ^ fileData) {
      DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader(
        fileData->Data, fileData->Length, nullptr, &m_pixelShader));

      CD3D11_BUFFER_DESC constantBufferDesc(
        sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
      DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr,
                                                  &m_constantBuffer));
    });

  auto createCubeTask = (createPSTask && createVSTask).then([this]() {
    VertexPositionColor cubeVertices[] = {
      { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
      { XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
      { XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
      { XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
      { XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
      { XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
      { XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
      { XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
    };

    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    vertexBufferData.pSysMem = cubeVertices;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices),
                                        D3D11_BIND_VERTEX_BUFFER);
    DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(
      &vertexBufferDesc, &vertexBufferData, &m_vertexBuffer));

    unsigned short cubeIndices[] = {
      0, 2, 1, // -x
      1, 2, 3,

      4, 5, 6, // +x
      5, 7, 6,

      0, 1, 5, // -y
      0, 5, 4,

      2, 6, 7, // +y
      2, 7, 3,

      0, 4, 6, // -z
      0, 6, 2,

      1, 3, 7, // +z
      1, 7, 5,
    };

    m_indexCount = ARRAYSIZE(cubeIndices);

    D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
    indexBufferData.pSysMem = cubeIndices;
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices),
                                       D3D11_BIND_INDEX_BUFFER);
    DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(
      &indexBufferDesc, &indexBufferData, &m_indexBuffer));
  });

  createCubeTask.then([this]() { m_loadingComplete = true; });
}

void CubeRenderer::CreateWindowSizeDependentResources()
{
  Direct3DBase::CreateWindowSizeDependentResources();

  float aspectRatio = m_windowBounds.Width / m_windowBounds.Height;
  float fovAngleY = 70.0f * XM_PI / 180.0f;
  if (aspectRatio < 1.0f) {
    fovAngleY /= aspectRatio;
  }

  // Note that the m_orientationTransform3D matrix is post-multiplied here
  // in order to correctly orient the scene to match the display orientation.
  // This post-multiplication step is required for any draw calls that are
  // made to the swap chain render target. For draw calls to other targets,
  // this transform should not be applied.
  XMStoreFloat4x4(
    &m_constantBufferData.projection,
    XMMatrixTranspose(XMMatrixMultiply(
      XMMatrixPerspectiveFovRH(fovAngleY, aspectRatio, 0.01f, 100.0f),
      XMLoadFloat4x4(&m_orientationTransform3D))));
}

void CubeRenderer::Update(float timeTotal, float timeDelta)
{
  (void)timeDelta; // Unused parameter.

  XMVECTOR eye = XMVectorSet(0.0f, 0.7f, 1.5f, 0.0f);
  XMVECTOR at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.0f);
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  XMStoreFloat4x4(&m_constantBufferData.view,
                  XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
  XMStoreFloat4x4(&m_constantBufferData.model,
                  XMMatrixTranspose(XMMatrixRotationY(timeTotal * XM_PIDIV4)));
}

void CubeRenderer::Render()
{
  const float midnightBlue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
  m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), midnightBlue);

  m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(),
                                      D3D11_CLEAR_DEPTH, 1.0f, 0);

  // Only draw the cube once it is loaded (loading is asynchronous).
  if (!m_loadingComplete) {
    return;
  }

  m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
                                   m_depthStencilView.Get());

  m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, NULL,
                                  &m_constantBufferData, 0, 0);

  UINT stride = sizeof(VertexPositionColor);
  UINT offset = 0;
  m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(),
                                   &stride, &offset);

  m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

  m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  m_d3dContext->IASetInputLayout(m_inputLayout.Get());

  m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);

  m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

  m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

  m_d3dContext->DrawIndexed(m_indexCount, 0, 0);
}
