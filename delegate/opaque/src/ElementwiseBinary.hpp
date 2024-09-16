//
// Copyright © 2023-2024 Arm Ltd and Contributors. All rights reserved.
// SPDX-License-Identifier: MIT
//
#pragma once

#include <OpaqueDelegateUtils.hpp>

namespace armnnOpaqueDelegate
{

TfLiteStatus ValidateAddOperator(DelegateData& delegateData,
                                 TfLiteOpaqueContext* tfLiteContext,
                                 const armnn::TensorInfo& inputInfo1,
                                 const armnn::TensorInfo& inputInfo2,
                                 const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        std::vector<armnn::TensorInfo> infos { inputInfo1, inputInfo2, outputInfo };
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("ADD",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputInfo,
                                          armnn::BinaryOperation::Add);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}


TfLiteStatus ValidateDivOperator(DelegateData& delegateData,
                                 TfLiteOpaqueContext* tfLiteContext,
                                 const armnn::TensorInfo& inputInfo1,
                                 const armnn::TensorInfo& inputInfo2,
                                 const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("DIV",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputTensorInfo,
                                          armnn::BinaryOperation::Div);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus ValidateCastOperator(const DelegateData& delegateData,
                                  TfLiteOpaqueContext* tfLiteContext,
                                  const armnn::TensorInfo& inputInfo,
                                  const armnn::TensorInfo& outputInfo)
{
    bool isSupported  = false;
    auto validateFunc = [&](const armnn::TensorInfo& outInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("CAST",
                                          tfLiteContext,
                                          IsCastSupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo,
                                          outInfo);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus ValidateFloorDivOperator(DelegateData& delegateData,
                                      TfLiteOpaqueContext* tfLiteContext,
                                      const armnn::TensorInfo& inputInfo1,
                                      const armnn::TensorInfo& inputInfo2,
                                      const armnn::TensorInfo& outputInfo)
{
    // need first to validate that the div operator is supported
    // then that the floor operator is supported, unless types are of Signed32.
    TfLiteStatus status = ValidateDivOperator(delegateData, tfLiteContext, inputInfo1, inputInfo2, outputInfo);
    if (status != kTfLiteOk)
    {
        return status;
    }

    // if the Inputs of FloorDiv are Signed32 we need to cast the inputs to Float32 to Floor correctly.
    if (AreAllTensorsSigned32({ inputInfo1, inputInfo2, outputInfo }))
    {
        // Converting to Float32 for Cast Outputs
        auto castOutputInfo0 = ConvertTensorInfoToFloat32(inputInfo1);
        auto castOutputInfo1 = ConvertTensorInfoToFloat32(inputInfo2);

        // We need to cast specifically for negative values, which floor towards -(infinity) and not
        // truncate towards zero, which without cast -> div -> floor, will happen.
        status = ValidateCastOperator(delegateData, tfLiteContext, inputInfo1, castOutputInfo0);
        if (status != kTfLiteOk)
        {
            return status;
        }

        status = ValidateCastOperator(delegateData, tfLiteContext, inputInfo2, castOutputInfo1);
        if (status != kTfLiteOk)
        {
            return status;
        }

        // No need for conversion for the output as the last cast layer will output the originally intended output type
        status = ValidateCastOperator(delegateData, tfLiteContext, castOutputInfo0, outputInfo);
        if (status != kTfLiteOk)
        {
            return status;
        }
        return status;
    }

    // in case broadcasting is being done from one of the inputs to the div
    // choose the full sized input tensor to pass to the floor validation routine
    armnn::TensorInfo floorInputInfo = inputInfo1;
    if (inputInfo1.GetNumDimensions() < inputInfo2.GetNumDimensions())
    {
        floorInputInfo = inputInfo2;
    }
    status = ValidateFloorOperator(delegateData, tfLiteContext, floorInputInfo, outputInfo);
    return status;
}

TfLiteStatus ValidateMaximumOperator(DelegateData& delegateData,
                                     TfLiteOpaqueContext* tfLiteContext,
                                     const armnn::TensorInfo& inputInfo1,
                                     const armnn::TensorInfo& inputInfo2,
                                     const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("MAXIMUM",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputTensorInfo,
                                          armnn::BinaryOperation::Maximum);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus ValidateMinimumOperator(DelegateData& delegateData,
                                     TfLiteOpaqueContext* tfLiteContext,
                                     const armnn::TensorInfo& inputInfo1,
                                     const armnn::TensorInfo& inputInfo2,
                                     const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("MINIMUM",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputTensorInfo,
                                          armnn::BinaryOperation::Minimum);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus ValidateMulOperator(DelegateData& delegateData,
                                 TfLiteOpaqueContext* tfLiteContext,
                                 const armnn::TensorInfo& inputInfo1,
                                 const armnn::TensorInfo& inputInfo2,
                                 const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("MUL",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputTensorInfo,
                                          armnn::BinaryOperation::Mul);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus ValidatePowerOperator(DelegateData& delegateData,
                                   TfLiteOpaqueContext* tfLiteContext,
                                   const armnn::TensorInfo& inputInfo1,
                                   const armnn::TensorInfo& inputInfo2,
                                   const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("POWER",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputTensorInfo,
                                          armnn::BinaryOperation::Power);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus ValidateSquaredDifferenceOperator(DelegateData& delegateData,
                                               TfLiteOpaqueContext* tfLiteContext,
                                               const armnn::TensorInfo& inputInfo1,
                                               const armnn::TensorInfo& inputInfo2,
                                               const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("SQUAREDDIFFERENCE",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputTensorInfo,
                                          armnn::BinaryOperation::SqDiff);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus ValidateSubOperator(DelegateData& delegateData,
                                 TfLiteOpaqueContext* tfLiteContext,
                                 const armnn::TensorInfo& inputInfo1,
                                 const armnn::TensorInfo& inputInfo2,
                                 const armnn::TensorInfo& outputInfo)
{
    bool isSupported = false;
    auto validateFunc = [&](const armnn::TensorInfo& outputTensorInfo, bool& isSupported)
    {
        FORWARD_LAYER_OPAQUE_SUPPORT_FUNC("SUB",
                                          tfLiteContext,
                                          IsElementwiseBinarySupported,
                                          delegateData.m_Backends,
                                          isSupported,
                                          armnn::BackendId(),
                                          inputInfo1,
                                          inputInfo2,
                                          outputTensorInfo,
                                          armnn::BinaryOperation::Sub);
    };

    validateFunc(outputInfo, isSupported);
    return isSupported ? kTfLiteOk : kTfLiteError;
}

TfLiteStatus VisitElementwiseBinaryOperator(DelegateData& delegateData,
                                            TfLiteOpaqueContext* tfLiteContext,
                                            TfLiteOpaqueNode* tfLiteNode,
                                            int nodeIndex,
                                            int32_t elementwiseBinaryOperatorCode)
{
    TF_LITE_ENSURE_STATUS(ValidateNumInputs(tfLiteContext, tfLiteNode, 2, nodeIndex));
    TF_LITE_ENSURE_STATUS(ValidateNumOutputs(tfLiteContext, tfLiteNode, 1, nodeIndex));

    // Gather input indices and use to get Input Tensors
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
    const TfLiteOpaqueTensor* tfLiteInputTensor0 = TfLiteOpaqueContextGetOpaqueTensor(tfLiteContext, inputTensors[0]);
    if (!IsValid(tfLiteContext, tfLiteInputTensor0, elementwiseBinaryOperatorCode, nodeIndex))
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
                tfLiteContext,
                "TfLiteArmnnOpaqueDelegate: Invalid input tensor in operator #%d node #%d: ",
                elementwiseBinaryOperatorCode, nodeIndex);
        return kTfLiteError;
    }
    // Use input indices to get filter tensor.
    const TfLiteOpaqueTensor* tfLiteInputTensor1 = TfLiteOpaqueContextGetOpaqueTensor(tfLiteContext, inputTensors[1]);
    if(!IsValid(tfLiteInputTensor1))
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
                tfLiteContext,
                "TfLiteArmnnOpaqueDelegate: Invalid input tensor in operator #%d node #%d: ",
                elementwiseBinaryOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    // Gather output indices and use to get output tensors.
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
    const TfLiteOpaqueTensor* tfLiteOutputTensor = TfLiteOpaqueContextGetOpaqueTensor(tfLiteContext, outputTensors[0]);
    if (!IsValid(tfLiteContext, tfLiteOutputTensor, elementwiseBinaryOperatorCode, nodeIndex))
    {
        return kTfLiteError;
    }

    armnn::TensorInfo inputTensorInfo0  = GetTensorInfoForTfLiteOpaqueTensor(tfLiteInputTensor0);
    armnn::TensorInfo inputTensorInfo1 = GetTensorInfoForTfLiteOpaqueTensor(tfLiteInputTensor1);
    const armnn::TensorInfo& outputTensorInfo = GetTensorInfoForTfLiteOpaqueTensor(tfLiteOutputTensor, true);

    // Check for unspecified dimensions in the output tensor
    if (outputTensorInfo.GetShape().GetDimensionality() == armnn::Dimensionality::NotSpecified)
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate: Shape dimensionality is not specified in operator #%d node #%d: ",
            elementwiseBinaryOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    // Check for unsupported 0-size dimensions in the tensor shapes
    if(ZeroDimPresent({inputTensorInfo0, inputTensorInfo1, outputTensorInfo}))
    {
        TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
            tfLiteContext,
            "TfLiteArmnnOpaqueDelegate: Zero dimension tensors are not supported in operator #%d node #%d",
            elementwiseBinaryOperatorCode, nodeIndex);
        return kTfLiteError;
    }

    // Check if we need to expand the dims of the input tensor infos.
    // This is required for a few of the backends.
    if(inputTensorInfo0.GetNumDimensions() != inputTensorInfo1.GetNumDimensions())
    {
        ExpandTensorRankToEqual(inputTensorInfo0, inputTensorInfo1);
    }

    auto* tfLiteNodeParameters = reinterpret_cast<TfLiteAddParams*>(TfLiteOpaqueNodeGetBuiltinData(tfLiteNode));
    TfLiteFusedActivation activationType = kTfLiteActNone;
    if (tfLiteNodeParameters)
    {
        activationType = tfLiteNodeParameters->activation;
        TfLiteStatus activationStatus = ValidateFusedActivationOperator(delegateData,
                                                                        tfLiteContext,
                                                                        outputTensorInfo,
                                                                        outputTensorInfo,
                                                                        activationType);
        if(activationStatus != kTfLiteOk)
        {
            return kTfLiteError;
        }
    }

    if (!delegateData.m_Network)
    {
        switch(elementwiseBinaryOperatorCode)
        {
            case kTfLiteBuiltinAdd:
                return ValidateAddOperator(delegateData,
                                           tfLiteContext,
                                           inputTensorInfo0,
                                           inputTensorInfo1,
                                           outputTensorInfo);
            case kTfLiteBuiltinDiv:
                return ValidateDivOperator(delegateData,
                                           tfLiteContext,
                                           inputTensorInfo0,
                                           inputTensorInfo1,
                                           outputTensorInfo);
            case kTfLiteBuiltinFloorDiv:
                return ValidateFloorDivOperator(delegateData,
                                                tfLiteContext,
                                                inputTensorInfo0,
                                                inputTensorInfo1,
                                                outputTensorInfo);
            case kTfLiteBuiltinMaximum:
                return ValidateMaximumOperator(delegateData,
                                               tfLiteContext,
                                               inputTensorInfo0,
                                               inputTensorInfo1,
                                               outputTensorInfo);
            case kTfLiteBuiltinMinimum:
                return ValidateMinimumOperator(delegateData,
                                               tfLiteContext,
                                               inputTensorInfo0,
                                               inputTensorInfo1,
                                               outputTensorInfo);
            case kTfLiteBuiltinMul:
                return ValidateMulOperator(delegateData,
                                           tfLiteContext,
                                           inputTensorInfo0,
                                           inputTensorInfo1,
                                           outputTensorInfo);
            case kTfLiteBuiltinPow:
                return ValidatePowerOperator(delegateData,
                                             tfLiteContext,
                                             inputTensorInfo0,
                                             inputTensorInfo1,
                                             outputTensorInfo);
            case kTfLiteBuiltinSquaredDifference:
                return ValidateSquaredDifferenceOperator(delegateData,
                                                         tfLiteContext,
                                                         inputTensorInfo0,
                                                         inputTensorInfo1,
                                                         outputTensorInfo);
            case kTfLiteBuiltinSub:
                return ValidateSubOperator(delegateData,
                                           tfLiteContext,
                                           inputTensorInfo0,
                                           inputTensorInfo1,
                                           outputTensorInfo);
            default:
                return kTfLiteError;
        }
    }

    armnn::IConnectableLayer* elementwiseBinaryLayer = nullptr;
    std::string layerName;
    switch(elementwiseBinaryOperatorCode)
    {
        case kTfLiteBuiltinAdd:
            layerName = GetName(armnn::BinaryOperation::Add, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::Add,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinDiv:
            layerName = GetName(armnn::BinaryOperation::Div, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::Div,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinFloorDiv:
            layerName = GetName(armnn::BinaryOperation::FloorDiv, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::FloorDiv,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinMaximum:
            layerName = GetName(armnn::BinaryOperation::Maximum, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::Maximum,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinMinimum:
            layerName = GetName(armnn::BinaryOperation::Minimum, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::Minimum,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinMul:
            layerName = GetName(armnn::BinaryOperation::Mul, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::Mul,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinPow:
            layerName = GetName(armnn::BinaryOperation::Power, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::Power,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinSquaredDifference:
            layerName = GetName(armnn::BinaryOperation::SqDiff, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::SqDiff,
                                                                                       layerName.c_str());
            break;
        case kTfLiteBuiltinSub:
            layerName = GetName(armnn::BinaryOperation::Sub, nodeIndex);
            elementwiseBinaryLayer = delegateData.m_Network->AddElementwiseBinaryLayer(armnn::BinaryOperation::Sub,
                                                                                       layerName.c_str());
            break;
        default:
            return kTfLiteError;
    }
    ARMNN_ASSERT(elementwiseBinaryLayer != nullptr);
    armnn::IOutputSlot& outputSlot = elementwiseBinaryLayer->GetOutputSlot(0);
    outputSlot.SetTensorInfo(outputTensorInfo);

    auto inputsTensorsProcess = ProcessInputs(elementwiseBinaryLayer,
                                              delegateData,
                                              tfLiteContext,
                                              tfLiteNode,
                                              nodeIndex);
    if (inputsTensorsProcess == kTfLiteError)
    {
        return inputsTensorsProcess;
    }

    if(Connect(elementwiseBinaryLayer, tfLiteContext, tfLiteNode, delegateData) != kTfLiteOk)
    {
        return kTfLiteError;
    }

    if (!tfLiteNodeParameters)
    {
        // No Activation
        return kTfLiteOk;
    }
    // Check and Create Activation
    return FusedActivation(tfLiteContext, tfLiteNode, activationType, elementwiseBinaryLayer, 0, delegateData,
                           nodeIndex);
}

} // namespace armnnOpaqueDelegate
