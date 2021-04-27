//
// Copyright © 2020 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <armnn/TypesUtils.hpp>

#include <backendsCommon/Workload.hpp>
#include <backendsCommon/WorkloadData.hpp>

namespace armnn
{

class RefQLstmWorkload : public BaseWorkload<QLstmQueueDescriptor>
{
public:
    explicit RefQLstmWorkload(const QLstmQueueDescriptor& descriptor, const WorkloadInfo& info);

    void Execute() const override;
    void ExecuteAsync(WorkingMemDescriptor& workingMemDescriptor)  override;

private:
    void Execute(std::vector<ITensorHandle*> inputs, std::vector<ITensorHandle*> outputs) const;
    std::unique_ptr<ScopedTensorHandle> m_InputToInputWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_InputToForgetWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_InputToCellWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_InputToOutputWeightsTensor;

    std::unique_ptr<ScopedTensorHandle> m_RecurrentToInputWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_RecurrentToForgetWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_RecurrentToCellWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_RecurrentToOutputWeightsTensor;

    std::unique_ptr<ScopedTensorHandle> m_CellToInputWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_CellToForgetWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_CellToOutputWeightsTensor;

    std::unique_ptr<ScopedTensorHandle> m_InputGateBiasTensor;
    std::unique_ptr<ScopedTensorHandle> m_ForgetGateBiasTensor;
    std::unique_ptr<ScopedTensorHandle> m_CellBiasTensor;
    std::unique_ptr<ScopedTensorHandle> m_OutputGateBiasTensor;

    std::unique_ptr<ScopedTensorHandle> m_ProjectionWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_ProjectionBiasTensor;

    std::unique_ptr<ScopedTensorHandle> m_InputLayerNormWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_ForgetLayerNormWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_CellLayerNormWeightsTensor;
    std::unique_ptr<ScopedTensorHandle> m_OutputLayerNormWeightsTensor;

    float m_LayerNormEpsilon = static_cast<float>(1e-8);
};

} //namespace armnn
