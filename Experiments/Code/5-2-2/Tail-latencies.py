'''
Author: lzy
Date: 2023-02-23 12:25:20
LastEditors: lzy
LastEditTime: 2023-02-27 02:45:05
FilePath: /AlibabaProject/Ceph-python/041_latencyFit23-2-23.py
Description: 

Copyright (c) 2023 by ${git_name_email}, All Rights Reserved. 
'''
#!/usr/bin/env python
# coding: utf-8

#画图 不同情况下的尾延迟图

from cProfile import label
from turtle import color
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
from matplotlib.pyplot import MultipleLocator, plot
import math

EPS = 1e-9

# 百分比换算
def to_percent(y, position):
    return str(100 * round(y, 2)) 


csvfile1 =  '../' + '5_2_2/Arrival_Rates/2.SASLO/100/latency/latency_100_000.csv'
#csvfile1 =  '../' + '5_2_2/Arrival_Rates/1.SG-QoS/250/latency/latency_250_000.csv'
csvfile2 = '../' + '5_2_2/Arrival_Rates/5.GraphP3O/100/latency/latency000.csv'

label_1 = 'SASLO'
BaseLineColor = 'b'

#label_1 = 'SG-QoS'
#BaseLineColor = 'r'

label_2 = 'Graph3PO'
Graph3POColor = 'g'

all_start1 = [] #所有请求的开始时间
latency1 = [] #所有请求的延迟

title1 = open(csvfile1,"r",encoding="utf-8").readline().strip() #读取表头信息
title2 = open(csvfile2,"r",encoding="utf-8").readline().strip() #读取表头信息

#读取文件1
csv_data1 = pd.read_csv(csvfile1,header=1)
csv_values1 = csv_data1.values
all_start1 = csv_values1[:,0].tolist()
latency1 =  csv_values1[:,1].tolist()

#读取文件2
csv_data2 = pd.read_csv(csvfile2,header=1)
csv_values2 = csv_data2.values
all_start2 = csv_values2[:,0].tolist()
latency2 =  csv_values2[:,1].tolist()


#图2 曲线拟合
fig = plt.figure(figsize=(10,6)) 

# 设置纵轴为百分比
fomatter = FuncFormatter(to_percent)
ax = plt.gca()

# ax.xaxis.set_major_locator(MultipleLocator(5))
ax.yaxis.set_major_formatter(fomatter)
# 避免横轴数据起始位置与纵轴重合，调整合适座标范围
minlatency = min(min(latency1),min(latency2))
maxlatency = max(max(latency1),max(latency2))
x_min = max(minlatency* 0.9, minlatency - 10)
x_max = max(maxlatency*1.1,maxlatency+10)

plt.xlim(x_min, x_max)

# 尾延迟数据
latencyList1 = sorted(latency1)   #排序后的延迟列表
latencyList2 = sorted(latency2)   #排序后的延迟列表
x1 = []
x2 = []
y1 = []
y2 = []
x1 = latencyList1
x2 = latencyList2

# 对数化
# for l in latency1:
#     x1.append(10*math.log2(l))
    
# for l in latency2:
#     x2.append(10*math.log2(l))    



#计算尾延迟指标
p50_1 = x1[int(len(x1)*0.5)]
p90_1 = x1[int(len(x1)*0.9)]
p99_1 = x1[int(len(x1)*0.99)]
p999_1 = x1[int(len(x1)*0.999)]
p9999_1 = x1[int(len(x1)*0.9999)]

p50_2 = x2[int(len(x2)*0.5)]
p90_2 = x2[int(len(x2)*0.9)]
p99_2 = x2[int(len(x2)*0.99)]
p999_2 = x2[int(len(x2)*0.999)]
p9999_2 = x2[int(len(x2)*0.9999)]


# 原始数据
for i in range(len(x1)):
    y1.append(i/len(x1))
# print('y=',y)
plt.plot(x1,y1,color=BaseLineColor,label = label_1)

for i in range(len(x2)):
    y2.append(i/len(x2))
# print('y=',y)
plt.plot(x2,y2,color=Graph3POColor,label = label_2)


# 竖线
vline_indx = [p90_1]
plt.vlines(vline_indx, 0,0.9,colors=BaseLineColor ,linestyles='dashed', label=label_1+' P90')
vline_indx = [p99_1]
plt.vlines(vline_indx, 0,0.99,colors=BaseLineColor, linestyles='dashed', label=label_1+' P99')
vline_indx = [p999_1]
plt.vlines(vline_indx, 0,0.999, colors=BaseLineColor,linestyles='dashed', label=label_1+' P999')
vline_indx = [p9999_1]
plt.vlines(vline_indx, 0,0.9999, colors=BaseLineColor,linestyles='dashed', label=label_1+' P9999')

vline_indx = [p90_2]
plt.vlines(vline_indx, 0,0.9,colors=Graph3POColor ,linestyles=':', label=label_2+' P90')
vline_indx = [p99_2]
plt.vlines(vline_indx, 0,0.99,colors=Graph3POColor, linestyles=':', label=label_2+' P99')
vline_indx = [p999_2]
plt.vlines(vline_indx, 0,0.999, colors=Graph3POColor,linestyles=':', label=label_2+' P999')
vline_indx = [p9999_2]
plt.vlines(vline_indx, 0,0.9999, colors=Graph3POColor,linestyles=':', label=label_2+' P9999')

plt.subplots_adjust(bottom=0.15, left=0.14)
print(title1)
print(title2)
# plt.grid() #显示网格
# plt.title(title1)
plt.legend(loc=4,fontsize=13) #设置图例位置
plt.xlabel('Latency(ms)',fontsize=26,weight='bold')
plt.ylabel('CDF(%)',fontsize=26,weight='bold')
plt.tick_params(labelsize=22)
filename = 'tailLatency-%s-%s-100.pdf'%(label_1,label_2)
print("save at ",filename )
fig.savefig(filename)
plt.axis('off')
plt.show()