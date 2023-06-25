# 用于处理readTestRes系列结果 计算尾延迟 计算拟合曲线 计算预测精度 写在一个文件里

#使用非线性最小二乘法拟合
# 
# #用指数形式来拟合

from typing import IO
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import FuncFormatter
from scipy.optimize import curve_fit
import os
import math

XMIN = 0

#将csv文件中的延迟数据转换成浮点数列表
def getlatencylist(latencyFile):

    latency = pd.read_csv(latencyFile).apply(pd.to_numeric).values
    latencyList = []
    for lat in latency:
        for la in lat:
            latencyList.append(float(la))
    return sorted(latencyList)

#拟合函数
def func(x,a):
    return 1 - np.exp(a * (XMIN - x))



def main():

    resfilename = 'readTestRes.csv'
    readpath = 'readTestResBG0'
    
    resfile = open(resfilename,'a')
    head = 'AveWait(s) AveSize() SampleNum ClientNum BgWrite p50(ms) p90(ms) p99(ms) p999(ms) p_p90(ms) p_p99(ms) p_p999(ms) pre_p90(%) pre_p99(%) pre_p999(%) file'
    head = head.replace(' ',',')
    #       平均等待时间 平均请求大小 样例数量 并发数 背景写压力 实际p50-p999 预测p90-p999 预测误差p90-p999 原始文件
    resfile.write(head + os.linesep  )   # os.linesep代表当前操作系统上的换行符

    latencyfiles = []  #存放不同BG下的csv数据
    for root,dirs,files in os.walk(readpath): #返回一个三元组:当前遍历的路径名，当前遍历路径下的目录列表，当前遍历路径下的文件列表
        for name in files:
            latencyfiles.append(os.path.join(root,name))   

    count = 0
    for latencyFile in latencyfiles:

        # # 用于测试
        # if count < 0:
        #     break
        # count = count -1 

        print(latencyFile)
        latencyList = getlatencylist(latencyFile=latencyFile)
        x = latencyList
        y = []
        # 计算百分比 作为y轴
        for i in range(len(x)):
            y.append(i/len(x))

        title = str(open(latencyFile).readline()).replace('\n','') 

        #计算尾延迟指标
        p50 = x[int(len(x)*0.5)]
        p90 = x[int(len(x)*0.9)]
        p99 = x[int(len(x)*0.99)]
        p999 = x[int(len(x)*0.999)]

        global XMIN
        XMIN = min(x)  #最小值 用于移动横坐标

        popt, pcov = curve_fit(func, x, y)
        #a里面是拟合系数 
        a=popt[0]
        print('min latency:',min(x),'max latency:',max(x))

        # 排队论模型 MM1
        # F(t)=1-e^(-1*a*t)
        # 具体移动坐标后 F(t)=1-e^(XMIN-1*a*t)

        #a 拟合出来后 反过来计算尾延迟
        # x = XMIN - ( ln(1-y) ) / a
        # code : x = XMIN - ( math.log(1-y) ) / a

        #根据曲线的预测值
        # p_p50   =  XMIN - ( math.log(1-0.5   ))  / a
        p_p90   =  XMIN - ( math.log(1-0.9   ))  / a
        p_p99   =  XMIN - ( math.log(1-0.99  ))  / a
        p_p999  =  XMIN - ( math.log(1-0.999 ))  / a

        datawide = 7 #float 数据取出来的位数

        #预测偏差
        pre_p90 = abs(float(p_p90-p90)/(p90))
        pre_p99 = abs(float(p_p99-p99)/(p99))
        pre_p999 =abs(float(p_p999-p999)/(p999))

        # 元数据表头示例 readLatency at ReqAveWait:0.010 ReqSize:1KB SampleNum:1000 ClientNum:1 BG:0MB/S
        title_split = title.split(' ')
        ReqAveWait = title_split[2][title_split[2].find(':')+1:]
        ReqSize = title_split[3][title_split[3].find(':')+1:]
        SampleNum = title_split[4][title_split[4].find(':')+1:]
        ClientNum = title_split[5][title_split[5].find(':')+1:]
        BGw = title_split[6][title_split[6].find(':')+1:]

        #       平均等待时间 平均请求大小 样例数量 并发数 背景写压力 实际p50-p999 预测p90-p999 预测偏差p90-p999

        tempstr = ReqAveWait + ' ' + ReqSize + ' ' + SampleNum + ' '+  ClientNum + ' ' + BGw+' '+ str(p50)[:datawide]+' '+ str(p90)[:datawide]+' '+str(p99)[:datawide]+' '+str(p999)[:datawide]+' '
        tempstr = tempstr +str(p_p90)[:datawide]+' '+str(p_p99)[:datawide]+' '+str(p_p999)[:datawide]+' '+str(pre_p90)[:datawide]+' '+str(pre_p99)[:datawide]+' '+str(pre_p999)[:datawide]+' '+latencyFile
        tempstr = tempstr.replace(' ',',')
        try:
            resfile.write(tempstr+os.linesep)
        except Exception as e :
            print(tempstr,'write error')
            print(e)  
            

    print('结果写入'+resfilename )
    resfile.close()




    
 
if __name__ == '__main__':
    main()    


