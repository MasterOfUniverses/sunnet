﻿//
// sunnet project
// Copyright (C) 2018 by Contributors <https://github.com/Tyill/sunnet>
//
// This code is licensed under the MIT License.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include "snBase/snBase.h"
#include "snOperatorCUDA/src/structurs.h"

/// convolution layer
class Convolution final : SN_Base::OperatorBase{

public:

    Convolution(void* net, const std::string& name, const std::string& node, std::map<std::string, std::string>& prms);

    ~Convolution();

    std::vector<std::string> Do(const SN_Base::operationParam&, const std::vector<OperatorBase*>& neighbOpr) override;
        
    bool setInternPrm(std::map<std::string, std::string>& prms) override;
    
    bool setBatchNorm(const SN_Base::batchNorm& bn) override;

    SN_Base::batchNorm getBatchNorm()const override;

private:
    
    struct convParams{
        size_t kernel = 10;                                      ///< number of output convolution layers
        size_t fWidth = 3;                                       ///< width mask
        size_t fHeight = 3;                                      ///< height mask 
        size_t dilate = 1;                                       ///< expansion mask
        size_t stride = 1;                                       ///< step mask
        size_t paddingSet = 0, paddingH = 0, paddingW = 0;       ///< padding layer
        bool useBias_ = true;
    };

    convParams convPrms_;
    bool isPaddingSame_ = false, isCheckPadding_ = false;

    activeType activeType_ = activeType::relu;              ///< active type
    optimizerType optimizerType_ = optimizerType::adam;     ///< optimizer type
    weightInitType weightInitType_ = weightInitType::he;    ///< init weight type
    batchNormType batchNormType_ = batchNormType::none;     ///< batchNorm 
    SN_Base::snSize inSzMem_;                               ///< insz mem
   
    const SN_Base::Tensor* inputMem_ = nullptr;
         
    bool isFreeze_ = false;                                 ///< not change weight

    uint32_t gpuDeviceId_ = 0;                              ///< gpu id
 
    SN_Base::snFloat dropOut_ = 0.F;                        ///< random off out

    SN_Base::snFloat optDecayMomentDW_ = 0.9F,              ///< optimiz weight
                     optDecayMomentWGr_ = 0.99F,
                     optLmbRegular_ = 0.001F;
        
    void* convGPUParams_ = nullptr;                         ///< gpu aux params 
        
    std::map<std::string, SN_Base::snFloat*> auxGPUParams_; ///< aux data 
    mutable std::map<std::string, std::vector<SN_Base::snFloat>> auxCPUParams_;

    void load(std::map<std::string, std::string>& prms);

    void updateConfig(bool isLern, const SN_Base::snSize& newSz);
         
        
    void forward(const SN_Base::Tensor& inTns, const SN_Base::operationParam& operPrm);
    void backward(const SN_Base::Tensor& inTns, const SN_Base::operationParam& operPrm);
       
  
    /// CUDA ///////////////////////////

    /// init aux params
    void iniParamCUDA(bool isLern, const SN_Base::snSize& insz, const SN_Base::snSize& outsz,
        const convParams&, void** gpuPrm);
   
    /// free aux params
    void freeParamCUDA(void* gpuPrm);

    void forwardCUDA(const convParams&,
        const SN_Base::snFloat* weight,
        const SN_Base::snSize& insz,   
        const SN_Base::snFloat* input,
        const SN_Base::snSize& outsz,  
        SN_Base::snFloat* output,      
        void* gpuPrm);

    /// calc grad and weight
    void backwardCUDA_GW(const convParams&,
        const SN_Base::snFloat* weight,
        const SN_Base::snSize& insz,   
        const SN_Base::snFloat* input,
        const SN_Base::snSize& outsz,  
        const SN_Base::snFloat* gradIn,
        SN_Base::snFloat* gradOut,     
        SN_Base::snFloat* dWeightOut,  
        void* gpuPrm);

    /// calc grad
    void backwardCUDA_G(const convParams&,
        const SN_Base::snFloat* weight,
        const SN_Base::snSize& insz,   
        const SN_Base::snSize& outsz,  
        const SN_Base::snFloat* gradIn,
        SN_Base::snFloat* gradOut,     
        void* gpuPrm);
};