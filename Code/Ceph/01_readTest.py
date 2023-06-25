
# coding: utf-8

# 后台是安装在 conpute2上的RGW 集群由compute2 compute4 storage2 storage3组成
# 此代码实现读取ceph中的文件 得出延迟数据

# 此版本修改了多线程实现细节，用于解决每次请求开始前的连续长延迟问题
# 实验环境初始化


from ast import Return
from distutils.command.clean import clean
from operator import imod
import os
from sre_constants import JUMP
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from boto3.session import Session
import botocore
from tqdm import tqdm
# import throttle
import random
import logging
import shutil
import numpy as np


EPS = 0.0005

# 准备密钥
aws_access_key_id = 'C3K4C32HFH39Z02CIXKM'
aws_secret_access_key = 'vC7GUlDiBuNd5gEXQUjmCvGNE0ra9KRubWODUUFx'

# 本地S3服务地址
local_s3 = 'http://192.168.3.57:8000'

#删除文件夹下面的所有文件
def del_file(path):
    ls = os.listdir(path)
    for i in ls:
        c_path = os.path.join(path, i)
        if os.path.isdir(c_path):
            del_file(c_path)
        else:
            os.remove(c_path)
            # print ('delete',c_path)

# 获取下载的文件总大小
def walkfunc(folder):

    folderscount=0
    filescount=0
    size=0
    #walk(top,topdown=True,οnerrοr=None)
    #top表示需要遍历的目录树的路径
    #topdown的默认值是"True",表示首先返回目录树下的文件，然后在遍历目录树的子目录
    #参数onerror的默认值是"None",表示忽略文件遍历时产生的错误.如果不为空，则提供一个自定义函数提示错误信息后继续遍历或抛出异常中止遍历
    for root,dirs,files in os.walk(folder): #返回一个三元组:当前遍历的路径名，当前遍历路径下的目录列表，当前遍历路径下的文件列表
        folderscount+=len(dirs)
        filescount+=len(files)
        size+=sum([os.path.getsize(os.path.join(root,name)) for name in files])
    return folderscount,filescount,size

# 发起请求和计算系统停留时间
def request_timing(s3res, i): # 使用独立 session.resource 以保证线程安全
    # print(i)
    res = []  #返回结果
    temp_file = 'readTempFile//read_Test_' + str(i) +'.bin'
    obj_name = objlist[ random.randint(0,len(objlist)) ] #随机取一个
    start = time.time()
    # 下载obj
    s3res.Object(bucket_name, obj_name).download_file(temp_file)
    end = time.time()
    system_time = end - start
    if REQ_WIAT > 0:
        t = random.expovariate(1.0 / REQ_WIAT)
        # print('sleep: %6.4f s'%(t))
        if t > system_time:
            time.sleep(t-system_time)   #剩余时间
    res.append(start*1000)
    res.append(system_time * 1000)
    return res
    # return system_time  # 秒


# 按照请求到达率限制来执行和跟踪请求
def arrival_rate_max(s3res, i): 
    return request_timing(s3res,i)

# 配置信息 全局变量
REQ_WIAT = 0.5 #平均等待时间
sampleNum = 100 #样例数量
clientNum = 1  #并发线程数量
REQ_SIZE = 1024 * 1 #平均请求大小
bgwrite = 0  #背景写压力
count = 0   #运行编号
objlist = []    
bucket_name = ''

def readTest( count ):

    print("read test start: ReqAveWait:%6.4fms ReqSize:%6.4fB SampleNum:%d ClientNum:%d BG:%dMB/S Bucket_name:%s\n"%(REQ_WIAT,REQ_SIZE,sampleNum,clientNum,bgwrite,bucket_name))

    global objlist 

    # 准备实验环境
    try:
        # 建立会话
        session = Session(aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key)

        # 连接到服务
        s3 = session.resource('s3', endpoint_url=local_s3)

        # 新建一个实验用 bucket (注意："bucket name" 中不能有下划线)

        if s3.Bucket(bucket_name) in s3.buckets.all():
            print('获取bucket: ',bucket_name,'成功')

        # 查看此 bucket 下的所有 object (若之前实验没有正常结束，则不为空)
        bucket = s3.Bucket(bucket_name)
        for obj in bucket.objects.all():
            objlist.append(obj.key)
            # print('obj name:%s' % obj.key)

        # 清理工作目录
        try:
            folder = os.getcwd()+'/readTempFile/'
            del_file(folder)
        except Exception as e:
            logging.error(e)
        else:
            print('清理工作目录完成')        


    except Exception as e:
        logging.error(e)
        print('readtest实验环境创建失败')
    else:
        print('readtest实验环境创建成功')


    # 按照预设IAT发起请求

    latency = []
    failed_requests = []
    starttime = []
    pre_futures = []
    downloadstarttime = time.time() #实验开始时间
    with tqdm(desc="Accessing S3", total=sampleNum) as pbar:      # 进度条设置，合计执行 sampleNum 项上传任务 (见 submit 部分)，进度也设置为 sampleNum 步
        with ThreadPoolExecutor(max_workers=clientNum) as executor: # 通过 max_workers 设置并发线程数
            # print('集群预热')  
            # try:
            #     pre_futures = [    #集群预热
            #         executor.submit(
            #             arrival_rate_max,
            #             session.resource('s3', endpoint_url=local_s3), i) for i in range(int(sampleNum*0.2)) # 为保证线程安全，应给每个任务申请一个新 resource
            #         ]
            # except Exception as e:
            #     logging(e)
            # else:
            #     count_pre = []
            #     for f in as_completed(pre_futures):
            #         if f.exception():
            #                 failed_requests.append(f)
            #         else:
            #             count_pre.append(f.result()[1])
            #     print('预热延迟：%6.4fms'%(sum(count_pre)/len(count_pre)))
            #     print('集群预热完成')  

            # time.sleep(1) 

            request_resource = [session.resource('s3', endpoint_url=local_s3)] * sampleNum  # 为保证线程安全，应给每个任务申请一个新 resource
            futures = []
            random_wait = np.random.poisson(lam=REQ_WIAT*10000000,size=sampleNum) /10000000.0   #泊松分布的等待时间
            for i in range(int(sampleNum)):
                futures += [executor.submit(arrival_rate_max,request_resource[i], i)]
                pbar.update(1)
                if( random_wait[i]-EPS > 0 ):
                    time.sleep(random_wait[i]-EPS) 
                # if i % 2 ==0:   #每两个 才 sleep 相当于 每次发两个
                #     time.sleep(random_wait[i])   #间隔时间之后再提交

            for future in as_completed(futures):
                if future.exception():
                    failed_requests.append(future)
                else:
                    starttime.append(future.result()[0]) #开始时间
                    latency.append(future.result()[1]) # 正确完成的请求，采集延迟

    downloadstoptime = time.time() #实验结束时间

    time.sleep(1)

    # 记录延迟到CSV文件    
    try:
        
        floname = 'readTest1013'
        # floname = 'readTestResBG' + str(bgwrite)
        if not os.path.isdir(floname):

            os.mkdir(floname)

        savefile =  floname+'/%03dreadlatency'%count+'.csv' #批量测试
        # savefile = 'readlatency//readlatency-bg-' + str(bgwrite) + '.csv' #不同BG时候使用这条
        # latency = latency[int(len(latency)*0.2):]   #去掉前五分之一
        # starttime = starttime[int(len(latency)*0.2):]
        with open(savefile, "w+") as tracefile:
            # print(len(latency),len(failed_requests))
            tracefile.write("readLatency at ReqAveWait:%5.4f ReqSize:%dKB SampleNum:%d ClientNum:%d BG:%dMB/S Ave:%6.4fms\n"%(REQ_WIAT,REQ_SIZE/1024,sampleNum,clientNum,bgwrite,sum(latency)/len(latency)))
            tracefile.write("startTime,latency"+os.linesep)

            for i in range(len(latency)):
                tracefile.writelines(str(starttime[i]-min(starttime))+','+str(latency[i])+'\n')
    except IOError as e:
        print(e)
        print(savefile,'文件写入失败')
    else:
        print(savefile,'文件写入成功')     


    folder = os.getcwd()+'/readTempFile/'

    try:
        folderscount,filescount,size=walkfunc(folder)   
    except Exception as e:
        logging.error(e) 
        print('获取下载文件大小失败')
    else:
        print("filescount:%d sizesum:%6.4fKB runtime:%6.4fs average size:%6.4fKB server rate:%6.4f"%(filescount,size/1024,(downloadstoptime - downloadstarttime),size/1024/filescount,filescount/(sum(latency))))        
        # 计算吞吐量
        thp = size/1024/1024 / (downloadstoptime - downloadstarttime) 
        print ('吞吐量：%7.5fMB/S'%thp)



    #多线程异常信息
    print('多线程异常数量：',len(failed_requests))       
    # for f in failed_requests:
    #     for ff in f:
    #         print(f)



def main():

    random.seed(time.time())   #随机数种子

    global REQ_WIAT 
    global sampleNum 
    global clientNum 
    global REQ_SIZE 
    global bgwrite 
    global objlist
    global bucket_name
 
    # req_waits = [0.0005,0.001,0.005,0.01,0.02,0.1  ]# 间隔时间 s 代表每秒发 2000 1000 200 100 50 10 个请求
    # sampleNums = [1000 ]
    # clientNums = [1,2,4,8]
    # req_sizes = [4,8,16,32,64  ] #KB

    # req_waits = np.arange(0.1,0.5,(0.5-0.1)/8)[::-1] # 间隔时间 s 
    # req_waits = np.arange(0.0005,0.04,0.0005)[::-1]

    # req_waits = np.arange(0.001,0.015,0.00025).tolist() + np.arange(0.015,0.04,0.001).tolist() 
    sampleNums = [20000]
    
    # req_waits = np.arange(0.001,0.051,0.001).tolist()
    # req_waits = req_waits[::-1]
    clientNums = [1,2,4,8,16][::-1]
    req_sizes = [1,2,4,8,16,32] #KB
    
    clientNums = [64]
    req_waits = [0.02] #组合起来每秒发50个请求
    req_sizes = [8]  

    # req_waits = [1]   # 用于测服务率

    count = 0
    for i in range(5):    #同样配置循环五次，查看是否差距过大
        for wait in req_waits:
            for samp in sampleNums:
                for client in clientNums:
                    for size in req_sizes:
                        REQ_WIAT = wait
                        sampleNum = samp
                        clientNum = client
                        REQ_SIZE = size*1024
                        bgwrite = 0
                        bucket_name = 'readtest' + str(REQ_SIZE)
                        objlist = []
                        readTest( count=count)
                        count = count +1
                        time.sleep(1)
        
 
if __name__ == '__main__':
    main()


