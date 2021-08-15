
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
   .addNode('C1', snOperator.Convolution(15, (3, 3)), 'C2') \
   .addNode('C2', snOperator.Convolution(25, (3, 3)), 'P1') \
   .addNode('P1', snOperator.Pooling(snType.poolType.max), 'F1') \
   .addNode('F1', snOperator.FullyConnected(256), 'F2') \
   .addNode('F2', snOperator.FullyConnected(10), 'LS') \
   .addNode('LS', snOperator.LossFunction(snType.lossType.softMaxToCrossEntropy), 'Output')

# load of weight
#if (net.loadAllWeightFromFile('c:/cpp/w.dat')):
 #   print('weight is load')
#else:
#    print('error load weight')

# loadImg
imgList = []
pathImg = 'c:\\cpp\\other\\sunnet\\example\\mnist\\images\\'
for i in range(10):
   imgList.append(os.listdir(pathImg + str(i)))

bsz = 100
lr = 0.001
accuratSumm = 0.
inLayer = np.zeros((bsz, 1, 28, 28), ctypes.c_float)
outLayer = np.zeros((bsz, 1, 1, 10), ctypes.c_float)
targLayer = np.zeros((bsz, 1, 1, 10), ctypes.c_float)
imgMem = {}

# cycle lern
for n in range(1000):

    targLayer[...] = 0

    for i in range(bsz):
        ndir = random.randint(0, 10 - 1)
        nimg = random.randint(0, len(imgList[ndir]) - 1)

        nm = pathImg + str(ndir) + '/' + imgList[ndir][nimg]
        if (nm in imgMem):
            inLayer[i][0] = imgMem[nm]
        else:
            inLayer[i][0] = imageio.imread(nm)
            imgMem[nm] = inLayer[i][0].copy()

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

# save weight
if (net.saveAllWeightToFile('c:/cpp/w.dat')):
    print('weight is save')
else:
    print('error save weight')