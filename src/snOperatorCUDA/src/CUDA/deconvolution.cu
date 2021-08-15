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

#include <cuda_runtime.h>
#include <cudnn.h>
#include "../stdafx.h"
#include "snOperatorCUDA/src/Operator/deconvolution.h"

using namespace std;
using namespace SN_Base;


struct gpuParams{

    cudnnHandle_t cudnn = 0;
    cudnnConvolutionDescriptor_t conv_desc = 0;
    cudnnTensorDescriptor_t in_desc = 0;
    cudnnTensorDescriptor_t out_desc = 0;
    cudnnTensorDescriptor_t grin_desc = 0;
    cudnnTensorDescriptor_t grout_desc = 0;
    cudnnFilterDescriptor_t w_desc = 0;
    cudnnFilterDescriptor_t dw_desc = 0;
    cudnnTensorDescriptor_t bias_desc = 0;

    cudnnConvolutionFwdAlgo_t algoFwd;
    cudnnConvolutionBwdDataAlgo_t algoBwdData;
    cudnnConvolutionBwdFilterAlgo_t algoBwdW;

    size_t wsFwdSz = 0;
    size_t wsBwdDataSz = 0;
    size_t wsBwdWSz = 0;
    size_t inszMem = 0;
  
    void* d_wsFwd = 0;
    void* d_wsBwdData = 0;
    void* d_wsBwdW = 0;

};

void Deconvolution::iniParamCUDA(bool isLern, const snSize& insz, const snSize& outsz,
    const deconvParams& prms, void** pGpuPrm){
   
    bool isFirst = false;

    gpuParams* gpuPrm = (gpuParams*)*pGpuPrm;
    if (!gpuPrm){

        cudaDeviceProp cu_deviceProps;

        cudaGetDeviceProperties(&cu_deviceProps, 0);
        if (cu_deviceProps.major < 3){
            ERROR_MESS("%s requires SM >= 3.0");
            return;
        }
        gpuPrm = new gpuParams();
        memset(gpuPrm, 0, sizeof(gpuParams));
        *pGpuPrm = gpuPrm;

        cudnnHandle_t cudnn = nullptr;
        cuCHECK(cudnnCreate(&cudnn));
        gpuPrm->cudnn = cudnn;

        isFirst = true;
    }
    
    // input
    cudnnTensorDescriptor_t in_desc = nullptr;
    cuCHECK(cudnnCreateTensorDescriptor(&in_desc));
    cuCHECK(cudnnSetTensor4dDescriptor(in_desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT, int(insz.n), int(insz.d), int(insz.h), int(insz.w)));
    if (!isFirst)
        cuCHECK(cudnnDestroyTensorDescriptor((cudnnTensorDescriptor_t)gpuPrm->in_desc));
    gpuPrm->in_desc = in_desc;
      
    // w      
    cudnnFilterDescriptor_t w_desc = nullptr;
    cuCHECK(cudnnCreateFilterDescriptor(&w_desc));
    cuCHECK(cudnnSetFilter4dDescriptor(w_desc, CUDNN_DATA_FLOAT, CUDNN_TENSOR_NCHW,
        int(insz.d), int(outsz.d), int(prms.fHeight), int(prms.fWidth)));
    if (!isFirst)
        cuCHECK(cudnnDestroyFilterDescriptor((cudnnFilterDescriptor_t)gpuPrm->w_desc));
    gpuPrm->w_desc = w_desc;
     
    // conv
    cudnnConvolutionDescriptor_t conv_desc = nullptr;
    cuCHECK(cudnnCreateConvolutionDescriptor(&conv_desc));
    cuCHECK(cudnnSetConvolution2dDescriptor(conv_desc, 0, 0, int(prms.stride), int(prms.stride), 1, 1,
        CUDNN_CROSS_CORRELATION, CUDNN_DATA_FLOAT));
    if (!isFirst)
        cuCHECK(cudnnDestroyConvolutionDescriptor((cudnnConvolutionDescriptor_t)gpuPrm->conv_desc));
    gpuPrm->conv_desc = conv_desc;

    // output
    cudnnTensorDescriptor_t out_desc;
    cuCHECK(cudnnCreateTensorDescriptor(&out_desc));
    cuCHECK(cudnnSetTensor4dDescriptor(out_desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
        int(outsz.n), int(outsz.d), int(outsz.h), int(outsz.w)));
    if (!isFirst)
        cuCHECK(cudnnDestroyTensorDescriptor((cudnnTensorDescriptor_t)gpuPrm->out_desc));
    gpuPrm->out_desc = out_desc;

    // algorithm
    cudnnConvolutionBwdDataAlgo_t algoBwdData;
    cuCHECK(cudnnGetConvolutionBackwardDataAlgorithm(gpuPrm->cudnn, w_desc, in_desc, conv_desc, out_desc,
        CUDNN_CONVOLUTION_BWD_DATA_PREFER_FASTEST, 0, &algoBwdData));
    gpuPrm->algoBwdData = algoBwdData;
  
    // workspace
    size_t wsBwdDataSz = 0;
    cuCHECK(cudnnGetConvolutionBackwardDataWorkspaceSize(gpuPrm->cudnn, w_desc, in_desc, conv_desc, out_desc, algoBwdData, &wsBwdDataSz));
    gpuPrm->wsBwdDataSz = wsBwdDataSz;


    size_t wsFwdSz = 0, wsBwdWSz = 0;
    if (isLern){
        // grin
        cudnnTensorDescriptor_t grin_desc;
        cuCHECK(cudnnCreateTensorDescriptor(&grin_desc));
        cuCHECK(cudnnSetTensor4dDescriptor(grin_desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
            int(outsz.n), int(outsz.d), int(outsz.h), int(outsz.w)));
        if (!isFirst)
            cuCHECK(cudnnDestroyTensorDescriptor((cudnnTensorDescriptor_t)gpuPrm->grin_desc));
        gpuPrm->grin_desc = grin_desc;

        // grout
        cudnnTensorDescriptor_t grout_desc;
        cuCHECK(cudnnCreateTensorDescriptor(&grout_desc));
        cuCHECK(cudnnSetTensor4dDescriptor(grout_desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT, int(insz.n), int(insz.d), int(insz.h), int(insz.w)));
        if (!isFirst)
            cuCHECK(cudnnDestroyTensorDescriptor((cudnnTensorDescriptor_t)gpuPrm->grout_desc));
        gpuPrm->grout_desc = grout_desc;

        // dw     
        cudnnFilterDescriptor_t dw_desc = nullptr;
        cuCHECK(cudnnCreateFilterDescriptor(&dw_desc));
        cuCHECK(cudnnSetFilter4dDescriptor(dw_desc, CUDNN_DATA_FLOAT, CUDNN_TENSOR_NCHW,
            int(insz.d), int(outsz.d), int(prms.fHeight), int(prms.fWidth)));
        if (!isFirst)
            cuCHECK(cudnnDestroyFilterDescriptor((cudnnFilterDescriptor_t)gpuPrm->dw_desc));
        gpuPrm->dw_desc = dw_desc;

        // bias
        cudnnTensorDescriptor_t bias_desc;
        cuCHECK(cudnnCreateTensorDescriptor(&bias_desc));
        cuCHECK(cudnnSetTensor4dDescriptor(bias_desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
            1, int(insz.d), 1, 1));
        if (!isFirst)
            cuCHECK(cudnnDestroyTensorDescriptor((cudnnTensorDescriptor_t)gpuPrm->bias_desc));
        gpuPrm->bias_desc = bias_desc;

        // algorithm
        cudnnConvolutionFwdAlgo_t algoFwd;
        cuCHECK(cudnnGetConvolutionForwardAlgorithm(gpuPrm->cudnn, grin_desc, w_desc, conv_desc, grout_desc,
            CUDNN_CONVOLUTION_FWD_PREFER_FASTEST, 0, &algoFwd));
        gpuPrm->algoFwd = algoFwd;

        cudnnConvolutionBwdFilterAlgo_t algoBwdW;
        cuCHECK(cudnnGetConvolutionBackwardFilterAlgorithm(gpuPrm->cudnn, grin_desc, in_desc, conv_desc, dw_desc,
            CUDNN_CONVOLUTION_BWD_FILTER_PREFER_FASTEST, 0, &algoBwdW));
        gpuPrm->algoBwdW = algoBwdW;

        // workspace       
        cuCHECK(cudnnGetConvolutionForwardWorkspaceSize(gpuPrm->cudnn, grin_desc, w_desc, conv_desc, grout_desc, algoFwd, &wsFwdSz));
        gpuPrm->wsFwdSz = wsFwdSz;
                
        cuCHECK(cudnnGetConvolutionBackwardFilterWorkspaceSize(gpuPrm->cudnn, grin_desc, in_desc, conv_desc, dw_desc, algoBwdW, &wsBwdWSz));
        gpuPrm->wsBwdWSz = wsBwdWSz;
    }

    if (isFirst){     
        cuCHECK(cudaMalloc(&gpuPrm->d_wsBwdData, wsBwdDataSz));

        if (isLern){          
            cuCHECK(cudaMalloc(&gpuPrm->d_wsFwd, wsFwdSz));
            cuCHECK(cudaMalloc(&gpuPrm->d_wsBwdW, wsBwdWSz));
        }
    }
    else if (gpuPrm->inszMem < insz.size()){      

        cuCHECK(cudaFree(gpuPrm->d_wsBwdData)); gpuPrm->d_wsBwdData = 0;  
        
        cuCHECK(cudaMalloc(&gpuPrm->d_wsBwdData, wsBwdDataSz));

        if (isLern){
          
            cuCHECK(cudaFree(gpuPrm->d_wsFwd));     gpuPrm->d_wsFwd = 0;
            cuCHECK(cudaFree(gpuPrm->d_wsBwdW));    gpuPrm->d_wsBwdW = 0;           
        
            cuCHECK(cudaMalloc(&gpuPrm->d_wsFwd, wsFwdSz));
            cuCHECK(cudaMalloc(&gpuPrm->d_wsBwdW, wsBwdWSz));           
        }
        gpuPrm->inszMem = insz.size();
    }
}

void Deconvolution::freeParamCUDA(void* gpuPrms){
       
    gpuParams* gpuPrm = (gpuParams*)gpuPrms;

    if (!gpuPrm) return;

    cuCHECK(cudnnDestroy(gpuPrm->cudnn));
    cuCHECK(cudnnDestroyConvolutionDescriptor(gpuPrm->conv_desc));
    cuCHECK(cudnnDestroyTensorDescriptor(gpuPrm->in_desc));
    cuCHECK(cudnnDestroyTensorDescriptor(gpuPrm->out_desc));   
    cuCHECK(cudnnDestroyFilterDescriptor(gpuPrm->w_desc));        
   
    cuCHECK(cudaFree(gpuPrm->d_wsBwdData));

    if (gpuPrm->grin_desc){ // isLern
        cuCHECK(cudnnDestroyTensorDescriptor(gpuPrm->grin_desc));
        cuCHECK(cudnnDestroyTensorDescriptor(gpuPrm->grout_desc));
        cuCHECK(cudnnDestroyFilterDescriptor(gpuPrm->dw_desc));
        cuCHECK(cudnnDestroyTensorDescriptor(gpuPrm->bias_desc));
      
        cuCHECK(cudaFree(gpuPrm->d_wsFwd));
        cuCHECK(cudaFree(gpuPrm->d_wsBwdW));
    }
}

void Deconvolution::forwardCUDA(const deconvParams& prms,
    const snFloat* weight, const snSize& insz, const snFloat* input, const snSize& outsz, snFloat* output, void* gpuPrms){
     
    gpuParams* gpuPrm = (gpuParams*)gpuPrms;
       
    // run
    snFloat alpha = 1.f, beta = 0.f;
    cuCHECK(cudnnConvolutionBackwardData(gpuPrm->cudnn,
        &alpha,
        gpuPrm->w_desc,
        weight,
        gpuPrm->in_desc,
        input,
        gpuPrm->conv_desc,
        gpuPrm->algoBwdData,
        gpuPrm->d_wsBwdData,
        gpuPrm->wsBwdDataSz,
        &beta,
        gpuPrm->out_desc,
        output));
   
}

__global__ void cuBwdBias(snSize insz, const snFloat* bias, snFloat* grout){

    size_t isz = insz.w * insz.h;

    snFloat* pGrOut = grout + isz * blockIdx.x + isz * insz.d * blockIdx.y;

    snFloat b = bias[blockIdx.x];

    unsigned int i = threadIdx.x;
    while (i < isz){
        
        pGrOut[i] += b;

        i += blockDim.x;
    }
}

void Deconvolution::backwardCUDA_GW(const deconvParams& prms,
    const snFloat* weight, const snSize& insz, const snFloat* input, const snSize& outsz, const snFloat* gradIn, snFloat* gradOut, snFloat* dWeightOut, void* gpuPrms){
 
    gpuParams* gpuPrm = (gpuParams*)gpuPrms;
    size_t wStepByN = prms.fWidth * prms.fHeight * insz.d * outsz.d;

    // run       
    snFloat alpha = 1.f, beta = 0.f;
    cuCHECK(cudnnConvolutionForward(gpuPrm->cudnn,
        &alpha,
        gpuPrm->grin_desc,
        gradIn,
        gpuPrm->w_desc,
        weight,
        gpuPrm->conv_desc,
        gpuPrm->algoFwd,
        gpuPrm->d_wsFwd,
        gpuPrm->wsFwdSz,
        &beta,
        gpuPrm->grout_desc,
        gradOut));

    cuCHECK(cudnnConvolutionBackwardFilter(gpuPrm->cudnn,
        &alpha,        
        gpuPrm->grin_desc,
        gradIn,
        gpuPrm->in_desc,
        input,
        gpuPrm->conv_desc,
        gpuPrm->algoBwdW,
        gpuPrm->d_wsBwdW,
        gpuPrm->wsBwdWSz,
        &beta,
        gpuPrm->dw_desc,
        dWeightOut));

    cuCHECK(cudnnConvolutionBackwardBias(gpuPrm->cudnn,
        &alpha,
        gpuPrm->in_desc,
        input,
        &beta,
        gpuPrm->bias_desc,
        dWeightOut + wStepByN));

    // +bias
    dim3 dimBlock(128);
    dim3 dimGrid(int(insz.d), int(insz.n));

    cuBwdBias << < dimGrid, dimBlock >> > (insz, weight + wStepByN, gradOut);
     
}

void Deconvolution::backwardCUDA_G(const deconvParams& prms,
    const snFloat* weight, const snSize& insz, const snSize& outsz, const snFloat* gradIn, snFloat* gradOut, void* gpuPrms){
    
    gpuParams* gpuPrm = (gpuParams*)gpuPrms;
    size_t wStepByN = prms.fWidth * prms.fHeight * insz.d * outsz.d;

    // run      
    snFloat alpha = 1.f, beta = 0.f;
    cuCHECK(cudnnConvolutionForward(gpuPrm->cudnn,
        &alpha,
        gpuPrm->grin_desc,
        gradIn,
        gpuPrm->w_desc,
        weight,
        gpuPrm->conv_desc,
        gpuPrm->algoFwd,
        gpuPrm->d_wsFwd,
        gpuPrm->wsFwdSz,
        &beta,
        gpuPrm->grout_desc,
        gradOut));

    // +bias
    cuBwdBias << < int(insz.n), 128 >> > (insz, weight + wStepByN, gradOut);
       
}
