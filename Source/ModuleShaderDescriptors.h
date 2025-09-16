#pragma once

#include "Module.h"

class SingleDescriptors;
class TableDescriptors;

class ModuleShaderDescriptors : public Module
{
public:

    ModuleShaderDescriptors();
    ~ModuleShaderDescriptors();

    bool init() override;
    void preRender() override;

    SingleDescriptors*    getSingle() { return singleDescriptors.get(); }
    TableDescriptors*     getTable() { return tableDescriptors.get(); }

    ID3D12DescriptorHeap* getHeap() { return heap.Get(); }

private:

    ComPtr<ID3D12DescriptorHeap> heap;
    std::unique_ptr<SingleDescriptors> singleDescriptors;
    std::unique_ptr<TableDescriptors>  tableDescriptors;
};
