﻿
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <filesystem>

#include "../cpp/snNet.h"
#include "../cpp/snTensor.h"
#include "../cpp/snOperator.h"

#include "Lib/OpenCV_3.3.0/opencv2/core/core_c.h"
#include "Lib/OpenCV_3.3.0/opencv2/core/core.hpp"
#include "Lib/OpenCV_3.3.0/opencv2/imgproc/imgproc_c.h"
#include "Lib/OpenCV_3.3.0/opencv2/imgproc/imgproc.hpp"
#include "Lib/OpenCV_3.3.0/opencv2/highgui/highgui_c.h"
#include "Lib/OpenCV_3.3.0/opencv2/highgui/highgui.hpp"

using namespace std;
namespace sn = SN_API;

bool loadImage(string& imgPath, int classCnt, vector<vector<string>>& imgName, vector<int>& imgCntDir, map<string, cv::Mat>& images){

    for (int i = 0; i < classCnt; ++i){

        namespace fs = std::tr2::sys;

        if (!fs::exists(fs::path(imgPath + to_string(i) + "/"))) continue;

        fs::directory_iterator it(imgPath + to_string(i) + "/"); int cnt = 0;
        while (it != fs::directory_iterator()){

            fs::path p = it->path();
            if (fs::is_regular_file(p) && (p.extension() == ".png")){

                imgName[i].push_back(p.filename());
            }
            ++it;
            ++cnt; if (cnt > 1000) break;
        }

        imgCntDir[i] = cnt;
    }

    return true;
}

int main(int argc, char* argv[]){
       
    sn::Net snet;   
    snet.addNode("Input", sn::Input(), "C1")
        .addNode("C1", sn::Convolution(15, 3, -1, 1,  sn::batchNormType::beforeActive), "C2")
        .addNode("C2", sn::Convolution(15, 3, 0, 1, sn::batchNormType::beforeActive), "P1")
        .addNode("P1", sn::Pooling(), "C3")
     
        .addNode("C3", sn::Convolution(25, 3, -1, 1, sn::batchNormType::beforeActive), "C4")
        .addNode("C4", sn::Convolution(25, 3, 0, 1, sn::batchNormType::beforeActive), "P2")
        .addNode("P2", sn::Pooling(), "C5")
     
        .addNode("C5", sn::Convolution(40, 3, -1, 1, sn::batchNormType::beforeActive), "C6")
        .addNode("C6", sn::Convolution(40, 3, 0, 1, sn::batchNormType::beforeActive), "P3")
        .addNode("P3", sn::Pooling(), "FC1")
    
        .addNode("FC1", sn::FullyConnected(2048, sn::batchNormType::beforeActive), "FC2")
        .addNode("FC2", sn::FullyConnected(128, sn::batchNormType::beforeActive), "FC3")
        .addNode("FC3", sn::FullyConnected(10), "LS")
        .addNode("LS", sn::LossFunction(sn::lossType::softMaxToCrossEntropy), "Output");

    string imgPath = "c://cpp//other//sunnet//example//cifar10//images//";
    
    int batchSz = 100, classCnt = 10, w = 28, h = 28, d = 3; float lr = 0.001F;
    vector<vector<string>> imgName(classCnt);
    vector<int> imgCntDir(classCnt);
    map<string, cv::Mat> images;
       
    if (!loadImage(imgPath, classCnt, imgName, imgCntDir, images)){
        cout << "Error 'loadImage' path: " << imgPath << endl;
        system("pause");
        return -1;
    }

    sn::Tensor inLayer(sn::snLSize(w, h, d, batchSz));
    sn::Tensor targetLayer(sn::snLSize(classCnt, 1, 1, batchSz));
    sn::Tensor outLayer(sn::snLSize(classCnt, 1, 1, batchSz));
       
    size_t sum_metric = 0;
    size_t num_inst = 0;
    float accuratSumm = 0;
    for (int k = 0; k < 1000; ++k){

        targetLayer.clear();
       
        srand(clock());
                
        for (int i = 0; i < batchSz; ++i){

            // directory
            int ndir = rand() % classCnt;
            while (imgCntDir[ndir] == 0) ndir = rand() % classCnt;

            // image
            int nimg = rand() % imgCntDir[ndir];

            // read
            cv::Mat img; string nm = imgName[ndir][nimg];
            if (images.find(nm) != images.end())
                img = images[nm];
            else{
                img = cv::imread(imgPath + to_string(ndir) + "/" + nm, CV_LOAD_IMAGE_UNCHANGED);
                images[nm] = img;
            }

            float* refData = inLayer.data() + i * w * h * d;
           
            size_t nr = img.rows, nc = img.cols;
            for (size_t r = 0; r < nr; ++r){
                uchar* pt = img.ptr<uchar>(r);
                for (size_t c = 0; c < nc * 3; c += 3){
                    refData[r * nc + c] = pt[c];
                    refData[r * nc + c + 1] = pt[c + 1];
                    refData[r * nc + c + 2] = pt[c + 2];
                }
            } 

            float* tarData = targetLayer.data() + classCnt * i;

            tarData[ndir] = 1;
        }

        // training
        float accurat = 0;
        snet.training(lr,
            inLayer,
            outLayer,
            targetLayer,
            accurat);

        // calc error
        sn::snFloat* targetData = targetLayer.data();
        sn::snFloat* outData = outLayer.data();
        size_t accCnt = 0, bsz = batchSz;
        for (int i = 0; i < bsz; ++i){

            float* refTarget = targetData + i * classCnt;
            float* refOutput = outData + i * classCnt;

            int maxOutInx = distance(refOutput, max_element(refOutput, refOutput + classCnt)),
                maxTargInx = distance(refTarget, max_element(refTarget, refTarget + classCnt));

            if (maxTargInx == maxOutInx)
                ++accCnt;
        }
              
        accuratSumm += (accCnt * 1.F) / bsz;

        cout << k << " accurate " << accuratSumm / k << " " << snet.getLastErrorStr() << endl;        
    }
       
    system("pause");
    return 0;
}