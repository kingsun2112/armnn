//
// Copyright © 2024 Arm Ltd and Contributors. All rights reserved.
// SPDX-License-Identifier: MIT
//
//
// Copyright © 2020 The TensorFlow Authors. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
//

#include "ExpOperator.hpp"
#include "TosaTableUtils.hpp"

TosaSerializationBasicBlock* ConvertExpOperator(const Layer* layer,
                                                const std::vector<const TensorInfo*>& inputs,
                                                const std::vector<const TensorInfo*>& outputs,
                                                const ElementwiseUnaryDescriptor* unaryDescriptor)
{
    if (unaryDescriptor->m_Operation != UnaryOperation::Exp)
    {
        throw armnn::Exception("ConvertExpOperator: Unsupported elementwise unary operation in descriptor.");
    }

    std::string inputName = std::string("input_");
    std::string outputName = std::string("output0_");
    std::string blockName  = std::string("Op_EXP_block_") + GetUniqueTosaMappingID();

    // If a layer is present then the block will be used for execution, so input and output names need to be determined
    // using the previous and following layers so the graph is connected correctly. For validation this doesn't matter.
    if(layer != nullptr)
    {
        inputName = GenerateUniqueInputName(layer->GetInputSlot(0));
        outputName = GenerateUniqueOutputName(*layer);
    }

    std::vector<TosaSerializationTensor*> tensors;
    std::vector<TosaSerializationOperator*> operators;

    float input_scale = inputs[0]->GetQuantizationScale();
    float output_scale = outputs[0]->GetQuantizationScale();
    int32_t input_zp = inputs[0]->GetQuantizationOffset();
    int32_t output_zp = outputs[0]->GetQuantizationOffset();
    DataType inputDType = inputs[0]->GetDataType();
    if (inputDType == DataType::QAsymmS8 ||
        inputDType == DataType::QSymmS8)
    {
        auto exp_func = [](float x) -> float { return std::exp(x); };
        TosaTableAttribute attribute(
            getTosaConst8bitTable(input_scale, input_zp, output_scale, output_zp, exp_func));
        operators.push_back(new TosaSerializationOperator(tosa::Op_TABLE,
                                                          Attribute_TableAttribute,
                                                          &attribute,
                                                          {inputName},
                                                          {outputName}));
    }
    else if (inputDType == DataType::QSymmS16)
    {
        throw Exception("ConvertExpOperator() unsupported int 16 not implemented yet.");
        // The following generates the table, tosa attribute and operator for int16 exponential.
        // However, running the int16 EXP EndToEnd test causes incorrect output values.
        // At the time of writing the EXP operator there is no requirment for int16 support.
        // Points to enable int16 in the future:
        //     - TOSA specifies EXP int16 input must have int32 output
        //     - We potentially need a rescale after the int32 EXP output to convert back to int16.
        /*
        auto exp_func = [](float x) -> float { return std::exp(x); };
        TosaTableAttribute attribute(
            getTosaConst16bitTable<float>(input_scale, input_zp, output_scale, output_zp, exp_func));
        operators.push_back(new TosaSerializationOperator(tosa::Op_TABLE,
                                                          Attribute_TableAttribute,
                                                          &attribute,
                                                          {inputName},
                                                          {outputName}));
        */
    }
    else if (inputDType == DataType::Signed32 ||
             inputDType == DataType::Signed64)
    {
        throw Exception(
            "ConvertExpOperator() unsupported int 32. Only int 8 and int 16 quantized types are supported.");
    }
    // Floating point EXP operator
    else
    {
        operators.push_back(new TosaSerializationOperator(tosa::Op_EXP,
                                                          Attribute_NONE,
                                                          nullptr,
                                                          {inputName},
                                                          {outputName}));
    }

    // Only add input tensor if connected layer is an input layer.
    // As intermediate or constant tensors will be created separately.
    // There also can't be duplicate tensor.
    if(inputName.find("input_") != std::string::npos)
    {
        std::vector<int32_t> inputShape0 = GetTosaTensorShape(inputs[0]->GetShape());
        DType inputDType0 = ArmNNToDType(inputDType);
        tensors.push_back(new TosaSerializationTensor(inputName, inputShape0, inputDType0, {}));
    }

    std::vector<int32_t> outputShape0 = GetTosaTensorShape(outputs[0]->GetShape());

    // Re-enable below line for int16 EXP support which requires int32 output in TOSA and remove second line.
    // DType outputDType0 =
    //     (inputDType == DataType::QSymmS16) ? DType::DType_INT32 : ArmNNToDType(outputs[0]->GetDataType());
    DType outputDType0 = ArmNNToDType(outputs[0]->GetDataType());

    tensors.push_back(new TosaSerializationTensor(outputName, outputShape0, outputDType0, {}));

    // operatorInputNames/operatorOutputNames ends up being the same as
    // blockInputNames/blockOutputNames for one-to-one ArmNN to Tosa mappings
    return new TosaSerializationBasicBlock(blockName, // name
                                           mainName, // region name
                                           operators, // operators
                                           tensors, // tensors
                                           {inputName}, // inputs
                                           {outputName}); // outputs
}