#include "pch.h"
#include "d3dUtil.h"


bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

ID3DBlob* d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ID3DBlob* blob;
    HR(D3DCreateBlob(size, &blob));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

ID3D12Resource* d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    ID3D12Resource*& uploadBuffer)
{
    //ID3D12Resource* defaultBuffer;

    //D3D12_HEAP_PROPERTIES defaultheapProperties;
    //defaultheapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    //defaultheapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    //defaultheapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    //defaultheapProperties.CreationNodeMask = 1;
    //defaultheapProperties.VisibleNodeMask = 1;

    //D3D12_RESOURCE_DESC resourceDecs;
    //resourceDecs.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    //resourceDecs.Alignment = 0;
    //resourceDecs.Width = byteSize;
    //resourceDecs.Height = 1;
    //resourceDecs.DepthOrArraySize = 1;
    //resourceDecs.MipLevels = 1;
    //resourceDecs.Format = DXGI_FORMAT_UNKNOWN;
    //resourceDecs.SampleDesc.Count = 1;
    //resourceDecs.SampleDesc.Quality = 0;
    //resourceDecs.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    //resourceDecs.Flags = D3D12_RESOURCE_FLAG_NONE;

    //D3D12_HEAP_PROPERTIES uploadheapProperties;
    //uploadheapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    //uploadheapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    //uploadheapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    //uploadheapProperties.CreationNodeMask = 1;
    //uploadheapProperties.VisibleNodeMask = 1;

    //// Create the actual default buffer resource.
    //HR(device->CreateCommittedResource(
    //    &uploadheapProperties,
    //    D3D12_HEAP_FLAG_NONE,
    //    &resourceDecs,
    //    D3D12_RESOURCE_STATE_COMMON,
    //    nullptr,
    //    IID_PPV_ARGS(&defaultBuffer)));

    //// In order to copy CPU memory data into our default buffer, we need to create
    //// an intermediate upload heap. 
    //HR(device->CreateCommittedResource(
    //    &uploadheapProperties,
    //    D3D12_HEAP_FLAG_NONE,
    //    &resourceDecs,
    //    D3D12_RESOURCE_STATE_GENERIC_READ,
    //    nullptr,
    //    IID_PPV_ARGS(&uploadBuffer)));


    //// Describe the data we want to copy into the default buffer.
    //D3D12_SUBRESOURCE_DATA subResourceData = {};
    //subResourceData.pData = initData;
    //subResourceData.RowPitch = byteSize;
    //subResourceData.SlicePitch = subResourceData.RowPitch;

    //auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
    //    D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

    //cmdList->ResourceBarrier(1, &barrier0);

    //auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
    //    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    //UpdateSubresources<1>(cmdList, defaultBuffer, uploadBuffer, 0, 0, 1, &subResourceData);
    //cmdList->ResourceBarrier(1, &barrier1);

    //return defaultBuffer;

    ID3D12Resource* defaultBuffer;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    // Create the actual default buffer resource.
    HR(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&defaultBuffer)));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 

    auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc1 = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    HR(device->CreateCommittedResource(
        &uploadProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc1,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));


    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    auto b0 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &b0);

    UpdateSubresources<1>(cmdList, defaultBuffer, uploadBuffer, 0, 0, 1, &subResourceData);

    auto b1 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &b1);

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.


    return defaultBuffer;
}

ID3DBlob* d3dUtil::CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ID3DBlob* byteCode = nullptr;
    ID3DBlob* errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());

    //HR(hr);

    return byteCode;
}