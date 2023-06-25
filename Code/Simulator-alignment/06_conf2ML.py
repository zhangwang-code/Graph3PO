#-*-coding:utf-8-*-
# ns3-sim.csv 的结果提取出指定的列，同时与相似度绑定，作为回归算法的输入

from distutils.command.config import config
from numpy import float64
import pandas as pd
import numpy as np


# path 是每个文件sim的值，floder是保存配置的文件夹名字
def makeRegData(path,floder):
    ns3_sim = pd.read_csv(path)
    print(ns3_sim)
    count = 1
    result = []
    
    for row_id in range( ns3_sim.shape[0] ):
        # if count < 1:
        #     break
        # count = count -1
        
        temp_map = {}
        sim = ns3_sim.loc[row_id]['Similarity']
        clusterFile = ns3_sim.loc[row_id]['clusterMapFile']
        # print(sim,clusterFile)
        curFile = floder + clusterFile.split('/')[-1]
        # print(curFile)
        config_pd = pd.read_csv(curFile)
        # print(config_pd)
        
        # print(disk_read_latency)
        temp_map['sim'] = sim
        temp_map['hdd_latency'] = config_pd.loc[22,'disk_read_latency(ms)']
        temp_map['ssd_latency'] = config_pd.loc[8,'disk_read_latency(ms)']
        temp_map['hdd_rate'] = config_pd.loc[22,'disk_read_rate(MB/S)']
        temp_map['ssd_rate'] = config_pd.loc[8,'disk_read_rate(MB/S)']
        result.append(temp_map)
        # for i in range( config_pd.shape[0] ):
        #     disk_read_latency = config_pd.iloc[23]['disk_read_latency(ms)']
        #     ns3_sim.loc[row_id]['disk_read_latency']   = float(disk_read_latency) 
        
    # print(result)
    res = pd.DataFrame(result)
    print(res)
    res.to_csv(r"RegData-latest.csv",index=None)

def main():
    #  2022-10-9 版本代码生成的数据集 
    path = 'ns3-sim-10-latest.csv'
    makeRegData(path=path,floder='cluster_map/')
    
    #  2022-11-2 版本代码生成的数据集
    # path = 'ns3-sim-rand-latest.csv'
    # makeRegData(path=path,floder='cluster_map_rand/')   # 不同的数据集
    
 
if __name__ == '__main__':
    main()    