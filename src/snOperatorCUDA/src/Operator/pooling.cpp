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
#include "../stdafx.h"
#include "snOperatorCUDA/src/Operator/pooling.h"
#include "snAux/auxFunc.h"
#include "snOperatorCUDA/src/structurs.h"
#include "snOperatorCUDA/src/cudaCommon.h"

using namespace std;
using namespace SN_Base;

/// pooling layer
Pooling::Pooling(void* net, const string& name, const string& node, std::map<std::string, std::string>& prms) :
    OperatorBase(net, name, node, prms){
        
    load(prms);
}

Pooling::~Pooling(){

    cuSetDeviceId(gpuDeviceId_);

    freeParamCUDA(gpuParams_);   
}

void Pooling::load(std::map<std::string, std::string>& prms){
   
       
    auto setIntParam = [&prms, this](const string& name, bool isZero, bool checkExist, size_t& value){

        if ((prms.find(name) != prms.end()) && SN_Aux::is_number(prms[name])){

            size_t v = stoi(prms[name]);
            if ((v > 0) || (isZero && (v == 0)))
                value = v;
            else
                ERROR_MESS("param '" + name + (isZero ? "' < 0" : "' <= 0"));
        }
        else if (checkExist)
            ERROR_MESS("not found (or not numder) param '" + name + "'");
    };
    
    setIntParam("kernel", false, false, poolPrms_.kernel);
    
    if (prms.find("stride") != prms.end())
        setIntParam("stride", false, false, poolPrms_.stride);
    else
        poolPrms_.stride = poolPrms_.kernel;
        
    if (prms.find("gpuDeviceId") != prms.end())
        gpuDeviceId_ = stoi(prms["gpuDeviceId"]);

    if (prms.find("pool") != prms.end()){

        string atype = prms["pool"];
        if (atype == "max") poolPrms_.type = poolType::max;
        else if (atype == "avg") poolPrms_.type = poolType::avg;
        else
            ERROR_MESS("param 'pool' = " + atype + " indefined");
    }
        
    basePrms_ = prms;
}

std::vector<std::string> Pooling::Do(const operationParam& operPrm, const std::vector<OperatorBase*>& neighbOpr){
    
    cuSetDeviceId(gpuDeviceId_);

    if (operPrm.action == snAction::forward){

        if (neighbOpr.size() > 1){
            ERROR_MESS("neighbOpr.size() > 1");
            return std::vector < std::string > {"noWay"};
        }

        forward(neighbOpr[0]->getOutput(), operPrm);
    }
    else{
        if (neighbOpr.size() == 1){
            backward(neighbOpr[0]->getGradient(), operPrm);
        }
        else{
            Tensor tns = neighbOpr[0]->getGradient();
            for (size_t i = 1; i < neighbOpr.size(); ++i){

                if (tns != neighbOpr[i]->getGradient()){
                    ERROR_MESS("operators size is not equals");
                    return std::vector < std::string > {"noWay"};
                }
                tns += neighbOpr[i]->getGradient();
            }
            backward(tns, operPrm);
        }
    }
    return std::vector<std::string>();
}

void Pooling::forward(const SN_Base::Tensor& inTns, const SN_Base::operationParam& operPrm){

    snSize insz = inTns.size();
    inputMem_ = &inTns;

    if (insz != inSzMem_){
        inSzMem_ = insz;
        updateConfig(operPrm.isLerning, insz);
    }
       
    snFloat* in = inputMem_->getDataGPU(),
           * out = baseOut_.getDataGPU();
   
    // calculation
    forwardCUDA(poolPrms_, insz, in, baseOut_.size(), out, gpuParams_);
    
}

void Pooling::backward(const SN_Base::Tensor& inTns, const operationParam& operPrm){

    snFloat* gradIn = inTns.getDataGPU();
        
    snFloat* input = inputMem_->getDataGPU(),
           * gradOut = baseGrad_.getDataGPU();    

    /// grad calculation
    backwardCUDA(poolPrms_, baseOut_.size(), baseOut_.getDataGPU(), gradIn, inSzMem_, input, gradOut, gpuParams_);
}

void Pooling::updateConfig(bool isLern, const snSize& newsz){
    
    size_t& kernel = poolPrms_.kernel,
          & paddingW = poolPrms_.paddingW,
          & paddingH = poolPrms_.paddingH,
          & stride = poolPrms_.stride;

    snSize outSz(0, 0, newsz.d, newsz.n);
   
    outSz.w = (newsz.w - kernel) / stride + 1;
    outSz.h = (newsz.h - kernel) / stride + 1;

    // check correct
    size_t resW = (newsz.w - kernel) % stride,
           resH = (newsz.h - kernel) % stride;
    
    isPadding_ = (resW != 0) || (resH != 0);
    
    if (isPadding_){   
       
        paddingW = 1;
        paddingH = 1;

        outSz.w = (newsz.w + paddingW * 2 - kernel) / stride + 1;
        outSz.h = (newsz.h + paddingH * 2 - kernel) / stride + 1;

    }
        
    baseOut_.resize(outSz);
        
    if (isLern)
       baseGrad_.resize(newsz);
    
    iniParamCUDA(isLern, newsz, outSz, poolPrms_, &gpuParams_);
}