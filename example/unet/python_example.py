
import os

from libsunnetimport*
import numpy as np
import imageio
import random
import ctypes
import datetime


# create net
net = snNet.Net()
net.addNode("In", snOperator.Input(), "C1") \
   .addNode("C1", snOperator.Convolution(10, (3, 3), -1), "C2") \
   .addNode("C2",snOperator.Convolution(10,(3, 3), 0), "P1 Crop1") \
   .addNode("Crop1", snOperator.Crop(snType.rect(0, 0, 487, 487)), "Rsz1") \
   .addNode("Rsz1", snOperator.Resize(snType.diap(0, 10), snType.diap(0, 10)), "Conc1") \
   .addNode("P1", snOperator.Pooling(), "C3") \
   \
   .addNode("C3", snOperator.Convolution(10,(3, 3), -1), "C4") \
   .addNode("C4", snOperator.Convolution(10,(3, 3), 0), "P2 Crop2") \
   .addNode("Crop2", snOperator.Crop(snType.rect(0, 0, 247, 247)), "Rsz2") \
   .addNode("Rsz2", snOperator.Resize(snType.diap(0, 10), snType.diap(0, 10)), "Conc2") \
   .addNode("P2", snOperator.Pooling(), "C5") \
   \
   .addNode("C5", snOperator.Convolution(10,(3, 3), 0), "C6") \
   .addNode("C6", snOperator.Convolution(10,(3, 3), 0), "DC1") \
   .addNode("DC1", snOperator.Deconvolution(10), "Rsz3") \
   .addNode("Rsz3", snOperator.Resize(snType.diap(0, 10), snType.diap(10, 20)), "Conc2") \
   \
   .addNode("Conc2", snOperator.Concat("Rsz2 Rsz3"), "C7") \
   \
   .addNode("C7", snOperator.Convolution(10,(3, 3), 0), "C8") \
   .addNode("C8", snOperator.Convolution(10,(3, 3), 0), "DC2") \
   .addNode("DC2", snOperator.Deconvolution(10), "Rsz4") \
   .addNode("Rsz4", snOperator.Resize(snType.diap(0, 10), snType.diap(10, 20)), "Conc1") \
   \
   .addNode("Conc1", snOperator.Concat("Rsz1 Rsz4"), "C9") \
   \
   .addNode("C9", snOperator.Convolution(10,(3, 3), 0), "C10")

convOut = snOperator.Convolution(1, (3, 3), 0)
convOut.act = snType.active.sigmoid;
net.addNode("C10", convOut, "LS") \
   .addNode('LS', snOperator.LossFunction(snType.lossType.binaryCrossEntropy), 'Output')

# loadImg

pathImg = 'c:/cpp/other/sunnet/example/unet/images/'
imgList = os.listdir(pathImg)

pathLabel= 'c:/cpp/other/sunnet/example/unet/labels/'
labelsList = os.listdir(pathLabel)

bsz = 5
lr = 0.001
accuratSumm = 0.
inLayer = np.zeros((bsz, 1, 512, 512), ctypes.c_float)
outLayer = np.zeros((bsz, 1, 483, 483), ctypes.c_float)
targLayer = np.zeros((bsz, 1, 483, 483), ctypes.c_float)

# cycle lern
for n in range(1000):

    targLayer[...] = 0

    for i in range(bsz):
        nimg = random.randint(0, len(imgList) - 1)
        inLayer[i] = imageio.imread(pathImg + imgList[nimg])

        targLayer[i] = np.resize(imageio.imread(pathLabel + labelsList[nimg]), (1, 483, 483)) / 255.

    acc = [0]  # do not use default accurate
    net.training(lr, inLayer, outLayer, targLayer, acc)

    accuratSumm += acc[0]

    print(datetime.datetime.now().strftime('%H:%M:%S'), n, "accurate", accuratSumm / (n + 1))