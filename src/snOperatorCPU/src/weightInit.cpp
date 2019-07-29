﻿//
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
#include "snBase/snBase.h"
#include "random.h"
#include "structurs.h"
#include "weightInit.h"

using namespace std;
using namespace SN_Base;

// РёРЅРёС†РёР°Р»РёР·Р°С†РёСЏ РІРµСЃРѕРІ

void weightInit(snFloat* ioW, size_t sz, size_t fan_in, size_t fan_out, weightInitType wtype){

    switch (wtype){
    case weightInitType::uniform: wi_uniform(ioW, sz); break;
    case weightInitType::he: wi_he(ioW, sz, fan_in); break;
    case weightInitType::lecun:wi_lecun(ioW, sz, fan_out); break;
    case weightInitType::xavier:wi_xavier(ioW, sz, fan_in, fan_out); break;
    }
}

void wi_uniform(snFloat* ioW, size_t sz){
    
    rnd_uniformReal(ioW, sz, -1.F, 1.F);
};

void wi_xavier(snFloat* ioW, size_t sz, size_t fan_in, size_t fan_out){
    snFloat wbase = std::sqrt(6.F / (fan_in + fan_out));

    rnd_uniformReal(ioW, sz, -wbase, wbase);
};

void wi_lecun(snFloat* ioW, size_t sz, size_t fan_out){
        
    snFloat wbase = 1.F / std::sqrt(snFloat(fan_out));

    rnd_uniformReal(ioW, sz, -wbase, wbase);
}

void wi_he(snFloat* ioW, size_t sz, size_t fan_in){

    snFloat sigma = std::sqrt(2.F / fan_in);

    rnd_gaussian(ioW, sz, 0.0F, sigma);
}

