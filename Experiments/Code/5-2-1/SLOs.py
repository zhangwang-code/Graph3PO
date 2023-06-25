#!/usr/bin/env python
# coding: utf-8

#解析模型预测 SLA

from cProfile import label
from turtle import color
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
from matplotlib.pyplot import MultipleLocator, plot

EPS = 1e-9

# 百分比换算
def to_percent(y, position):
    return str(100 * round(y, 2))


csvfile = 'SLOs.csv'
all_start = [] #所有请求的开始时间
latency = [] #所有请求的延迟
title1 = open(csvfile,"r",encoding="utf-8").readline().strip() #读取表头信息

#读取文件
csv_data = pd.read_csv(csvfile,header=1)
csv_values = csv_data.values
all_start = csv_values[:,0].tolist()
latency =  csv_values[:,1].tolist()

# 用排队论模型来拟合实测数据
#----------------------------------------------------------

#图2 曲线拟合
fig2 = plt.figure(figsize=(10,6)) 

# 设置纵轴为百分比
fomatter = FuncFormatter(to_percent)
ax = plt.gca()
# ax.xaxis.set_major_locator(MultipleLocator(5))
ax.yaxis.set_major_formatter(fomatter)
# 避免横轴数据起始位置与纵轴重合，调整合适座标范围
x_min = max(min(latency) * 0.9, min(latency) - 10)
x_max = max(max(latency)*1.1,max(latency)+10)

plt.xlim(x_min, x_max)
# 绘制实际百分位延迟
# plt.hist(latency, cumulative=True, histtype='step', weights=[1./ len(latency)] * len(latency))

# 计算拟合参数

latencyList = sorted(latency)   #排序后的延迟列表
x = latencyList
y = []
# print('x=',x)

#计算尾延迟指标
p50 = x[int(len(x)*0.5)]
p90 = x[int(len(x)*0.9)]
p99 = x[int(len(x)*0.99)]
p999 = x[int(len(x)*0.999)]
p9999 = x[int(len(x)*0.9999)]

# 计算百分比 作为y轴
for i in range(len(x)):
    y.append(i/len(x))
# print('y=',y)
plt.plot(x,y, linewidth = 3.0)

XMIN = min(x)  #最小值 用于移动横坐标

vline_indx = [p90]
plt.vlines(vline_indx, 0,0.9,colors='b' ,linestyles='dashed', label='P90')
vline_indx = [p99]
plt.vlines(vline_indx, 0,0.99,colors='g', linestyles='dashed', label='P99')
vline_indx = [p999]
plt.vlines(vline_indx, 0,0.999, colors='r',linestyles='dashed', label='P999')
vline_indx = [p9999]
plt.vlines(vline_indx, 0,0.9999, colors='y',linestyles='dashed', label='P9999')

print('min latency:',min(x),'max latency:',max(x))


print(title1)
plt.grid() #显示网格
#plt.title(title1)
plt.subplots_adjust(bottom=0.15, left=0.14)

plt.legend(loc=4, bbox_to_anchor=(1.005 , 0.025), fontsize = 20) #设置图例位置
plt.xlabel('Latency(ms)', fontsize = 26, weight = 'bold')
plt.ylabel('CDF(%)', fontsize = 26, weight = 'bold')
plt.xticks(fontsize = 22)
plt.yticks(fontsize = 22)
fig2.savefig('SLOs.svg')
plt.show()


