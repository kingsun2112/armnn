//
// Copyright © 2023 Arm Ltd and Contributors. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <OpaqueDelegateUtils.hpp>

namespace armnnOpaqueDelegate
{
std::vector<int32_t> getMultiplesData(armnn::DataType dataType, const TfLiteOpaqueTensor* tfLiteTensor)
{
    auto multiplesTensorNum = TfLiteOpaqueTensorDim(tfLiteTensor, 0);
    // If the Multiples Datatype is Signed64 we need to cast the values different otherwise an error is thrown.
    if(dataType == armnn::DataType::Signed64)
    {
        auto multiplesTensorDataPtr = static_cast<int64_t*>(TfLiteOpaqueTensorData(tfLiteTensor));
        return {multiplesTensorDataPtr, multiplesTensorDataPtr + multiplesTensorNum};
    }
    else
    {
        auto multiplesTensorDataPtr = static_cast<int32_t*>(TfLiteOpaqueTensorData(tfLiteTensor));
        return {multiplesTensorDataPtr, multiplesTensorDataPtr + multiplesTensorNum};
    }
}

TfLiteStatus ValidateTileOperator(DelegateData& delegateData,
                                  TfLiteOpaqueContext *tfLiteContext,
                                  const armnn::TensorInfo& inputInfo,
                                  const armnn::TensorInfo& outputInfo,
                                  const armnn::TileDescriptor& descriptor)
{
    bool isSupported = false;
    FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("TILE",
                                      tfLiteContext,
                                      IsTileSupported,
                                      delegateData.m_Backends,
                                      isSupported,
                                      armnn::BackendId(),
                                      inputInfo,
                                      outputInfo,
                                      descriptor);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus VisitTileOperator(DelegateData& delegateData,
                               TfLiteOpaqueContext* tfLiteContext,
                               TfLiteOpaqueNode* tfLiteNode,
                               int nodeIndex,
                               int32_t tileOperatorCode)
{
    TF_LITE_ENSURE_STATUS(ValidateNumInputs(tfLiteContext, tfLiteNode, 2, nodeIndex));
    TF_LITE_ENSURE_STATUS(ValidateNumOutputs(tfLiteContext, tfLiteNode, 1, nodeIndex));

    // Gather input tensors
    auto numInputs = TfLiteOpaqueNodeNumberOfInputs(tfLiteNode);
    const int* inputTensors;
    if (TfLiteOpaqueNodeInputs(tfLiteNode, &inputTensors, &numInputs) != kTfLiteOk)
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate: Unable to gather input tensor indices from node #%d: ",
            nodeIndex);
        return kTfLiteError;
    }

    // Gather output tensors
    int numOutputs = 0;
    const int* outputTensors;
    if (TfLiteOpaqueNodeOutputs(tfLiteNode, &outputTensors, &numOutputs) != kTfLiteOk)
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate: Unable to gather output tensor indices from node #%d: ",
            nodeIndex);
        return kTfLiteError;
    }

    // The input contains the data that should be tiled
    const TfLiteOpaqueTensor* tfLiteInputTensor =
            TfLiteOpaqueContextGetOpaqueTensor(tfLiteContext, inputTensors[0]);
    if (IsDynamicTensor(tfLiteInputTensor))
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate: Dynamic input tensors are not supported in operator #%d node #%d: ",
            tileOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    // The multiples tensor contains the number of copies for each axis
    const TfLiteOpaqueTensor* tfLiteMultiplesTensor =
            TfLiteOpaqueContextGetOpaqueTensor(tfLiteContext, inputTensors[1]);;
    if (IsDynamicTensor(tfLiteMultiplesTensor))
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate: Dynamic input tensors are not supported in operator #%d node #%d: ",
            tileOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    // The output tensor
    const TfLiteOpaqueTensor* tfLiteOutputTensor =
            TfLiteOpaqueContextGetOpaqueTensor(tfLiteContext, outputTensors[0]);
    if (IsDynamicTensor(tfLiteOutputTensor))
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate: Dynamic output tensors are not supported in operator #%d node #%d: ",
            tileOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    const armnn::TensorInfo& inputTensorInfo = GetTensorInfoForTfLiteOpaqueTensor(tfLiteInputTensor);
    const armnn::TensorInfo& multiplesTensorInfo = GetTensorInfoForTfLiteOpaqueTensor(tfLiteMultiplesTensor);
    const armnn::TensorInfo& outputTensorInfo = GetTensorInfoForTfLiteOpaqueTensor(tfLiteOutputTensor, true);

    // Multiples length must be the same as the number of dimension in input tensor
    if (multiplesTensorInfo.GetNumElements() != inputTensorInfo.GetNumDimensions())
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate:",
            "The Multiples length must be the same as the number of dimension in input tensor",
            "Operator: #%d node #%d: ",
            tileOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    // Get the Multiples data: In armnn, the values of the multiples input tensor is saved in the operator descriptor
    // We have to read it from the input tensor and write it the descriptor
    auto multiplesIntData = getMultiplesData(multiplesTensorInfo.GetDataType(), tfLiteMultiplesTensor);

    // The multiples must be positive
    for (auto multiple : multiplesIntData)
    {
        if (multiple < 0)
        {
            TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
                tfLiteContext,
                "TfLiteArmnnOpaqueDelegate: The Multiples must be positive values",
                "Operator: #%d node #%d: ",
                tileOperatorCode, nodeIndex);
            return kTfLiteError;
        }
    }

    // The original input from TFLite is int32, and we have to make it as uint32 for our descriptor
    std::vector<uint32_t> multiplesUintData;
    std::transform(multiplesIntData.begin(),
                   multiplesIntData.end(),
                   std::back_inserter(multiplesUintData),
                   [] (const int value)
                   {
                       return static_cast<uint32_t>(value);
                   });

    armnn::TileDescriptor tileDescriptor;
    tileDescriptor.m_Multiples = multiplesUintData;

    // Check output dimensions
    if (inputTensorInfo.GetNumDimensions() != outputTensorInfo.GetNumDimensions())
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
                tfLiteContext,
                "TfLiteArmnnOpaqueDelegate: Input tensor dimension and output tensor dimension differ",
                "Operator: #%d node #%d: ",
                tileOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    // No network pointer indicates that only support for this operator should be checked
    if (!delegateData.m_Network)
    {
        return ValidateTileOperator(delegateData,
                                    tfLiteContext,
                                    inputTensorInfo,
                                    outputTensorInfo,
                                    tileDescriptor);
    }

    auto layerName = GetName(armnn::LayerType::Tile, nodeIndex);
    armnn::IConnectableLayer* layer = delegateData.m_Network->AddTileLayer(tileDescriptor, layerName.c_str());

    if (layer == nullptr)
    {
        return kTfLiteError;
    }

    layer->GetOutputSlot(0).SetTensorInfo(outputTensorInfo);

    if (ProcessInputs(layer, delegateData, tfLiteContext, tfLiteNode, nodeIndex) != kTfLiteOk)
    {
        return kTfLiteError;
    }

    return Connect(layer, tfLiteContext, tfLiteNode, delegateData);
}

} // namespace armnnOpaqueDelegate