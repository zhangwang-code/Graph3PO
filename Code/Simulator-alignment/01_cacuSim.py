# coding: utf-8

# 计算欧氏距离相似度
import numpy as np
import pandas as pd


def tail2Ave(filename):
    #读取文件
    csv_data = pd.read_csv(filename,header=0)
    csv_values = csv_data.values   # list
    print('origin data')
    print(csv_data)
    
    for i in range(4):
        csv_data.iloc[:,i+2] = csv_data.iloc[:,i+2]/csv_data.iloc[:,1]
    
    # print(csv_data)    
    
    return csv_data
    
    # print(csv_values)
 
def cacuSim(ns3_data,ceph):
    
    print(ns3_data)
    ns3_data.loc[:,'Similarity'] = 0
    cephnp = np.array( [ceph[1]/ceph[0],ceph[2]/ceph[0],ceph[3]/ceph[0],ceph[4]/ceph[0]  ]  )
    print(cephnp)
    
    for row_id in range( ns3_data.shape[0] ):
        # print(row_id)
        # print (ns3_data.iloc[row_id,:])
        ns3np = ns3_data.iloc[row_id][2:6].values
        # print('ceph',cephnp)
        # print('ns3',ns3np)
        distance = np.linalg.norm(ns3np - cephnp)  #欧氏距离 
        # print('sim',sim)
        ns3_data.loc[row_id,'Similarity'] = 1.0/distance  #距离的倒数，即距离越大，相似度越低
          
    print(ns3_data)
    ns3_data.to_csv('ns3-sim-10-latest.csv',index=None)
    

def main():
    filename = 'latencyPreview-latest.csv'
    # filename = 'c3_rand1_latencyPreview4.csv'
    #选择一组真机数据
    ceph = [47.3085,40.26302,78.99916,149.11614,221.20408]    # 到达率86的数据
    # ceph = [21.5475,14.615,54.43,71.02375,121.4025]   # 到达率50的数据
    # ceph = [21.68333333,14.65666667,54.81,70.43666667,111.31]  # 到达率为25的数据
    
    ns3 = tail2Ave(filename)
    
    cacuSim(ns3,ceph)
 
if __name__ == '__main__':
    main()    