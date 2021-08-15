//
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

/// тип ф-ии активации
enum class activeType{
    none = -1,
    sigmoid = 0,
    relu = 1,
    leakyRelu = 2,
    elu = 3,
};

/// тип инициализации весов
enum class weightInitType{
    uniform = 0,
    he = 1,
    lecun = 2,
    xavier = 3,
};

/// тип оптимизации весов
enum class optimizerType{
    sgd = 0,
    sgdMoment = 1,
    adagrad = 2,
    RMSprop = 3,
    adam = 4,
};

/// batchNorm
enum class batchNormType{
    none = -1,
    beforeActive = 0,
    postActive = 1,   
};

/// pooling
enum class poolType{
    max = 0,
    avg = 1,
};

// loss
enum class lossType{
    softMaxACrossEntropy = 0,
    binaryCrossEntropy = 1,
    regressionMSE = 2,
    userLoss = 3,
};

struct roi{
    size_t x, y, w, h;

    roi(size_t x_ = 0, size_t y_ = 0, size_t w_ = 0, size_t h_ = 0) :
        x(x_), y(y_), w(w_), h(h_){}
};