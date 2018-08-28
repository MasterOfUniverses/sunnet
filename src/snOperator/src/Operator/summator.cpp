//
// SkyNet Project
// Copyright (C) 2018 by Contributors <https://github.com/Tyill/skynet>
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
#include "summator.h"

using namespace std;
using namespace SN_Base;

/// сумматор 2х и более слоев
Summator::Summator(void* net, const string& name, const string& node, std::map<std::string, std::string>& prms) :
OperatorBase(net, name, node, prms){

    baseOut_ = new Tensor();
    baseGrad_ = new Tensor();

    if (basePrms_.find("type") != basePrms_.end()){
        if (basePrms_["type"] == "summ") sType_ = sType::summ;
        else if (basePrms_["type"] == "diff") sType_ = sType::diff;
        else if (basePrms_["type"] == "mean") sType_ = sType::mean;
        else
            ERROR_MESS("param 'type' indefined");
    }
}

std::vector<std::string> Summator::Do(const operationParam& operPrm, const std::vector<OperatorBase*>& neighbOpr){
        
    if (neighbOpr.size() == 1){
        if (operPrm.action == snAction::forward){
            auto nb = neighbOpr[0]->getOutput();
            baseOut_->setData(nb->getData(), nb->size());
        }
        else{
            auto nb = neighbOpr[0]->getGradient();
            baseGrad_->setData(nb->getData(), nb->size());
        }
    }
    else{
       
        if (operPrm.action == snAction::forward){

            *baseOut_ = *neighbOpr[0]->getOutput();

            size_t sz = neighbOpr.size();
            for (size_t i = 1; i < sz; ++i){
            
                if (*baseOut_ != *neighbOpr[i]->getOutput()){
                    ERROR_MESS("operators size is not equals");
                    return std::vector < std::string > {"noWay"};
                }  
                switch (sType_){
                case Summator::sType::summ: *baseOut_ += *neighbOpr[i]->getOutput(); break;
                case Summator::sType::diff: *baseOut_ -= *neighbOpr[i]->getOutput(); break;
                case Summator::sType::mean: mean(baseOut_, neighbOpr[i]->getOutput(), baseOut_); break;
                }                
            }
        }
        else{

            *baseGrad_ = *neighbOpr[0]->getGradient();

            size_t sz = neighbOpr.size();
            for (size_t i = 1; i < sz; ++i){
             
                if (*baseGrad_ != *neighbOpr[i]->getGradient()){
                    ERROR_MESS("operators size is not equals");
                    return std::vector < std::string > {"noWay"};
                }
                switch (sType_){
                case Summator::sType::summ: *baseGrad_ += *neighbOpr[i]->getGradient(); break;
                case Summator::sType::diff: *baseGrad_ -= *neighbOpr[i]->getGradient(); break;
                case Summator::sType::mean: mean(baseGrad_, neighbOpr[i]->getGradient(), baseGrad_); break;
                }
            }
        }
    }

    return std::vector<std::string>();
}

void Summator::mean(const Tensor* one, const Tensor* two, Tensor* out){
   
    snFloat* done = one->getData(),
           * dtwo = two->getData(),
           * dout = out->getData();

    size_t sz = one->size().size();
    for (size_t i = 0; i < sz; ++i){
        dout[i] = (done[i] + dtwo[i]) / 2;
    }
}