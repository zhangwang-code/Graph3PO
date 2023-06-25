#!/usr/bin/env python
# coding: utf-8

#延迟数据预览

from cProfile import label
from threading import local
from turtle import color, width
from matplotlib import markers
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
from matplotlib.pyplot import MultipleLocator, plot
from scipy.optimize import curve_fit
import os
import math

EPS = 1e-9

csvfile = 'latency002.csv'

all_start = [] #所有请求的开始时间
latency = [] #所有请求的延迟
title1 = open(csvfile,"r",encoding="utf-8").readline().strip() #读取表头信息

#读取文件
csv_data = pd.read_csv(csvfile,header=1)
csv_values = csv_data.values
all_start = csv_values[:,0].tolist()
latency =  csv_values[:,1].tolist()

# 请求延迟分布情况

fig1 = plt.figure(figsize=(10,6))
ave_ = []  #计算每个窗口的平均延迟
ave__x = []
start = 0
sampleSize = 1000  # 窗口大小
for i in range(len(latency)):
    
    if i-start+1 == sampleSize or i == len(latency)-1:
        # print(i)
        ave_.append( sum(latency[start:i])/len(latency[start:i]) )
        start = i +1    
        ave__x.append(i)
        
# 计算尾延迟指标值
latencyList = sorted(latency)   #排序后的延迟列表
x = latencyList
y = []
# print('x=',x)

# 计算到达率 服务率
arrival_rate = 1000*len(all_start)/(max(all_start) - min(all_start))
server_rate = 1000.0/( sum(latency)/len(latency) )
print(max(all_start),min(all_start))

#计算尾延迟指标
ave = sum(x)/len(x)
p50 = x[int(len(x)*0.5)]
p90 = x[int(len(x)*0.9)]
p99 = x[int(len(x)*0.99)]
p999 = x[int(len(x)*0.999)]
p9999 = x[int(len(x)*0.9999)]
 
title2 = title1 + "\nArrival:%5.3f Server_rate:%5.3f P50:%5.3f P90:%5.3f P99:%5.3f P999:%5.3f P9999:%5.3f Max:%5.3f\n"%(arrival_rate,server_rate,p50,p90,p99,p999,p9999,max(x))        

x = np.arange(0,len(latency),1)
# plt.scatter(x,latency,s=5,label='real latency')
plt.title(title2)
plt.plot(latency,label='real latency')
plt.plot(ave__x,ave_,label='ave latency/%4d requests'%sampleSize,color='red',marker='.')
plt.xlabel('request')
plt.ylabel('Latency/ms')
plt.legend()
plt.grid()
print('result save at ceph-Latency-Perview.svg')
fig1.savefig('ceph-Latency-Perview.svg',format='svg')
plt.show()
