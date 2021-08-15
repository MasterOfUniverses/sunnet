
import os

from libsunnetimport*
import numpy as np
import imageio
import random
import ctypes
import datetime

# create net
net = snNet.Net()
net.addNode('In', snOperator.Input(), 'C1') \
   .addNode('C1', snOperator.Convolution(15, (3, 3), -1, 1, snType.batchNormType.beforeActive), 'C2') \
   .addNode('C2', snOperator.Convolution(15, (3, 3), 0, 1, snType.batchNormType.beforeActive), 'P1') \
   .addNode('P1', snOperator.Pooling(snType.poolType.max), 'C3') \
   .addNode('C3', snOperator.Convolution(25, (3, 3), -1, 1, snType.batchNormType.beforeActive), 'C4') \
   .addNode('C4', snOperator.Convolution(25, (3, 3), 0, 1, snType.batchNormType.beforeActive), 'P2') \
   .addNode('P2', snOperator.Pooling(snType.poolType.max), 'C5') \
   .addNode('C5', snOperator.Convolution(40, (3, 3), -1, 1, snType.batchNormType.beforeActive), 'C6') \
   .addNode('C6', snOperator.Convolution(40, (3, 3), 0, 1, snType.batchNormType.beforeActive), 'P3') \
   .addNode('P3', snOperator.Pooling(snType.poolType.max), 'F1') \
   .addNode('F1', snOperator.FullyConnected(2048), 'F2') \
   .addNode('F2', snOperator.FullyConnected(128), 'F3') \
   .addNode('F3', snOperator.FullyConnected(10), 'LS') \
   .addNode('LS', snOperator.LossFunction(snType.lossType.softMaxToCrossEntropy), 'Output')

# loadImg
imgList = []
pathImg = 'c:/cpp/other/sunnet/example/cifar10/images/'
for i in range(10):
   imgList.append(os.listdir(pathImg + str(i)))

bsz = 100
lr = 0.0001
accuratSumm = 0.
inLayer = np.zeros((bsz, 3, 32, 32), ctypes.c_float)
outLayer = np.zeros((bsz, 1, 1, 10), ctypes.c_float)
targLayer = np.zeros((bsz, 1, 1, 10), ctypes.c_float)

# cycle lern
for n in range(1000):

    targLayer[...] = 0

    for i in range(bsz):
        ndir = random.randint(0, 10 - 1)
        nimg = random.randint(0, len(imgList[ndir]) - 1)
        inLayer[i] = imageio.imread(pathImg + str(ndir) + '/' + imgList[ndir][nimg]).reshape(3,32,32)

        targLayer[i][0][0][ndir] = 1.

    acc = [0]  # do not use default accurate
    net.training(lr, inLayer, outLayer, targLayer, acc)

    # calc accurate
    acc[0] = 0
    for i in range(bsz):
        if (np.argmax(outLayer[i][0][0]) == np.argmax(targLayer[i][0][0])):
            acc[0] += 1

    accuratSumm += acc[0]/bsz

    print(datetime.datetime.now().strftime('%H:%M:%S'), n, "accurate", accuratSumm / (n + 1))