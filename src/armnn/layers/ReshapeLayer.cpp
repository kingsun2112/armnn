//
// Copyright © 2017-2024 Arm Ltd and Contributors. All rights reserved.
// SPDX-License-Identifier: MIT
//
#include "ReshapeLayer.hpp"

#include "LayerCloneBase.hpp"

#include <armnn/utility/IgnoreUnused.hpp>
#include <armnn/TypesUtils.hpp>
#include <armnn/backends/WorkloadData.hpp>
#include <armnn/backends/WorkloadFactory.hpp>

namespace armnn
{

ReshapeLayer::ReshapeLayer(const ReshapeDescriptor& param, const char* name)
    : LayerWithParameters(1, 1, LayerType::Reshape, param, name)
{
}

std::unique_ptr<IWorkload> ReshapeLayer::CreateWorkload(const IWorkloadFactory& factory) const
{
    ReshapeQueueDescriptor descriptor;
    SetAdditionalInfo(descriptor);

    return factory.CreateWorkload(LayerType::Reshape, descriptor, PrepInfoAndDesc(descriptor));
}

ReshapeLayer* ReshapeLayer::Clone(Graph& graph) const
{
    return CloneBase<ReshapeLayer>(graph, m_Param, GetName());
}

std::vector<TensorShape> ReshapeLayer::InferOutputShapes(const std::vector<TensorShape>& inputShapes) const
{
    IgnoreUnused(inputShapes);
    return std::vector<TensorShape>({ m_Param.m_TargetShape });
}

void ReshapeLayer::ValidateTensorShapesFromInputs()
{
    VerifyLayerConnections(1, CHECK_LOCATION());

    const TensorShape& outputShape = GetOutputSlot(0).GetTensorInfo().GetShape();

    VerifyShapeInferenceType(outputShape, m_ShapeInferenceMethod);

    auto inferredShapes = InferOutputShapes({ GetInputSlot(0).GetTensorInfo().GetShape() });

    if (inferredShapes.size() != 1)
    {
        throw armnn::LayerValidationException("inferredShapes has "
                                              + std::to_string(inferredShapes.size()) +
                                              " elements - should only have 1.");
    }

    if (inferredShapes[0].GetDimensionality() != Dimensionality::Specified)
    {
        throw armnn::LayerValidationException("inferredShapes' dimensionality has not been specified.");
    }

    ValidateAndCopyShape(outputShape, inferredShapes[0], m_ShapeInferenceMethod, "ReshapeLayer");
}

void ReshapeLayer::ExecuteStrategy(IStrategy& strategy) const
{
    strategy.ExecuteStrategy(this, GetParameters(), {}, GetName());
}

} // namespace armnn
