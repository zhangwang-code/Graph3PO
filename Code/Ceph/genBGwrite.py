# 生成写负载

# coding: utf-8

# 实验环境初始化

from operator import imod
import os
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from boto3.session import Session
import botocore
from tqdm import tqdm
import throttle
import random
import shutil


# 发起请求和计算系统停留时间
def request_timing(s3res, i): # 使用独立 session.resource 以保证线程安全

    obj_name = "testObj%08d"%(i,) # 所建对象名
    # print(obj_name)
    # temp_file = '.tempfile'
    service_time = 0 # 系统滞留时间

    start = time.time()
    # print( 'filesize:',getFileSize(local_file) )
    s3res.Object(bucket_name, obj_name).upload_file(local_file) # 将本地文件上传为对象
    # 或
    # bucket.put_object(Key=obj_name, Body=open(local_file, 'rb'))
    # 下载obj
    # s3res.Object(bucket_name, obj_name).download_file(temp_file)
    end = time.time()
    system_time = end - start
    sleeptime = 1 - system_time
    # print('latency:%7.5f sleeptime:%7.5f'%(system_time,sleeptime))
    if sleeptime > 0:
        time.sleep(sleeptime)  #剩余时间休眠，保证一秒发一个 1MB的数据

    return system_time * 1000 # 换算为毫秒
    # return system_time  # 秒


# 按照请求到达率限制来执行和跟踪请求
def arrival_rate_max(s3res, i): # 不进行限速
    return request_timing(s3res, i)



random.seed(time.time())   #随机数种子

sampleNum = 100 #样例数量
clientNum = 2  #并发线程数量
requestSize = 2 * 1024 * 1024   #请求大小 byte

# throughput = 1   # MB/S  目的吞吐量
#参数计算需要满足的是 每秒运行的线程数 * 并发数 * 请求大小 = 吞吐量


# 准备密钥
aws_access_key_id = 'C3K4C32HFH39Z02CIXKM'
aws_secret_access_key = 'vC7GUlDiBuNd5gEXQUjmCvGNE0ra9KRubWODUUFx'

# 本地S3服务地址
# local_s3 = 'http://192.168.3.57:8000' #compute2
local_s3 = 'http://192.168.3.157:8000'  #compute4
local_s3 = 'http://192.168.3.29:8000'  #storage1

try:
    # 建立会话
    session = Session(aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key)

    # 连接到服务
    s3 = session.resource('s3', endpoint_url=local_s3)

    # 新建一个实验用 bucket (注意："bucket name" 中不能有下划线)

    bucket_name = 'genbgwrite' 

    if s3.Bucket(bucket_name) not in s3.buckets.all():
        s3.create_bucket(Bucket=bucket_name)
    
    bucket = s3.Bucket(bucket_name) 

except Exception:
    print('实验环境创建失败')
else:
    print('实验环境创建成功')

    # 清理实验环境
try:
    # 删除bucket下所有object
    bucket.objects.filter().delete()

    # 删除bucket下某个object
    # bucket.objects.filter(Prefix=obj_name).delete()

    # bucket.delete()\
except botocore.exceptions.ClientError as e:
    print('error in bucket removal') 
else:
    print('bucket delete')       


time.sleep(10)    

# 初始化本地数据文件
local_file = "_test_file.bin"
test_bytes = [0xFF for i in range(int(requestSize))] # 填充至所需大小

with open(local_file, "wb") as lf:
    lf.write(bytearray(test_bytes))


# 按照预设IAT发起请求


latency = []
failed_requests = []

writetime = time.time() 
count = 250 #循环次数
for i in range(count):
    with tqdm(desc="Accessing S3", total=sampleNum) as pbar:      # 进度条设置，合计执行 sampleNum 项上传任务 (见 submit 部分)，进度也设置为 sampleNum 步
        with ThreadPoolExecutor(max_workers=clientNum) as executor: # 通过 max_workers 设置并发线程数
            futures = [
                executor.submit(
                    arrival_rate_max,
                    session.resource('s3', endpoint_url=local_s3), i) for i in range(sampleNum) # 为保证线程安全，应给每个任务申请一个新 resource
                ]
            for future in as_completed(futures):
                if future.exception():
                    failed_requests.append(futures)
                else:
                    latency.append(future.result()) # 正确完成的请求，采集延迟
                pbar.update(1)

stopwirtetime = time.time()


# 删除本地测试文件


os.remove(local_file)

# 记录延迟到CSV文件
latencyfile = 'genBGlatency.csv'    
try:
    
    with open(latencyfile, "w+") as tracefile:
        tracefile.write("Latency at SampleNum:%d ClientNum:%d Max:%5.3f Ave:%5.3f\n"%(sampleNum*count,clientNum,max(latency),sum(latency)/len(latency)))
        tracefile.writelines([str(l) + '\n' for l in latency])
except IOError:
    print('latency文件写入失败')
else:
    print('latency文件写入成功')     

#计算实际吞吐率 MB/S
thp = (sampleNum*count * requestSize)/1024/1024 /(stopwirtetime - writetime) 
print ('实际吞吐率：%7.5fMB/S  持续时间：%5.3fs '%(thp,stopwirtetime - writetime))

# 多线程异常信息
print('异常多线程数量：',len(failed_requests))
# for f in failed_requests:
#     for ff in f:
#         print(f)



