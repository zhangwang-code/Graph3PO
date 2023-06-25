'''
Author: lzy
Date: 2023-04-14 06:58:05
LastEditors: lzy
LastEditTime: 2023-04-14 10:17:23
Description: auto keras

Copyright (c) 2023 by ziyi.liao@foxmail.com, All Rights Reserved. 
'''
# -*-coding:utf-8-*-

import tensorboard
import numpy as np
import pandas as pd
import tensorflow as tf
from sklearn.datasets import fetch_california_housing
from keras.models import load_model
from keras.utils import plot_model
import datetime
from sklearn.metrics import r2_score
from sklearn.model_selection import train_test_split

import autokeras as ak

# NS3的数据
path = "RegData-latest.csv"
# 读入数据
data = np.loadtxt(path, delimiter=",", dtype=np.float32, skiprows=1)

# print(data)
# 切片，分离数据，x,Y
x = data[:, 1:]
y = data[:, 0]

# 分割数据，分为训练集和测试集
x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=0.2)

# If you aim to find a better and more fitting model, you should consider increasing the values of the following two parameters.
# These values were used in our experiment, you can replace them in the source code to explore other options.
# train_trials = 200
# train_epoch = 100

train_trials = 10
train_epoch = 100

# It tries different models.
reg = ak.StructuredDataRegressor(overwrite=True, max_trials=train_trials)
# Feed the structured data regressor with training data.
reg.fit(x_train, y_train, epochs=train_epoch)
# Predict with the best model.
y_pred = reg.predict(x_test)
# Evaluate the best model with testing data.
print(reg.evaluate(x_test, y_test))

print("r2 score:", r2_score(y_test, y_pred))

# 导出我们生成的模型
model = reg.export_model()

try:
    model.save("model_autokeras", save_format="tf")
    print("model save at model_autokeras")
except Exception:
    model.save("model_autokeras.h5")
    print("model save at model_autokeras.h5")

# 将模型导出成可视化图片
try:
    plot_model(model, to_file="Model_plot.png")
except Exception as e:
    print(e)
else:
    print("model fig save at Model_plot.png")
