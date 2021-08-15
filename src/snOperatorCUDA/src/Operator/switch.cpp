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
#include "snOperatorCUDA/src/Operator/switch.h"
#include "snAux/auxFunc.h"

using namespace std;
using namespace SN_Base;

/// The choice of the path
Switch::Switch(void* net, const string& name, const string& node, std::map<std::string, std::string>& prms) :
OperatorBase(net, name, node, prms){
}

std::vector<std::string> Switch::Do(const operationParam& operPrm, const std::vector<OperatorBase*>& neighbOpr){
            
    if (operPrm.action == snAction::forward){

        if (neighbOpr.size() > 1){
            ERROR_MESS("neighbOpr.size() > 1");
            return std::vector < std::string > {"noWay"};
        }

        baseOut_ = neighbOpr[0]->getOutput();
    }
    else{
       
        baseGrad_ = neighbOpr[0]->getGradient();

        size_t sz = neighbOpr.size();
        for (size_t i = 1; i < sz; ++i){

            if (baseGrad_ != neighbOpr[i]->getGradient()){
                ERROR_MESS("operators size is not equals");
                return std::vector < std::string > {"noWay"};
            }
            baseGrad_ += neighbOpr[i]->getGradient();
        }           
    }
       
    vector<string> nw;
    if (basePrms_.find("nextWay") != basePrms_.end()){
        nw = SN_Aux::split(basePrms_["nextWay"], " ");
    }

    return nw;
}
