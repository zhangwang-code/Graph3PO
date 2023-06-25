
# coding: utf-8
# 后台是安装在 conpute2上的RGW 集群由compute2 compute4 storage2 storage3组成
# 此代码为读测试准备用于读取的文件
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


# 生成指定平均值的服从泊松分布的大小  read test bucket 
def readBucketInit(requestSize  ):
    print( 'readBucketInit size: ',requestSize )
    random.seed(time.time())   #随机数种子

    sampleNum = 100000 #样例数量

    # 准备密钥
    aws_access_key_id = 'C3K4C32HFH39Z02CIXKM'
    aws_secret_access_key = 'vC7GUlDiBuNd5gEXQUjmCvGNE0ra9KRubWODUUFx'

    # 本地S3服务地址
    local_s3 = 'http://192.168.3.57:8000'

    bucket_name = 'readtest' + str(requestSize)
    print('bucket name: ',bucket_name)

    try:
        # 建立会话
        session = Session(aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key)

        # 连接到服务
        s3 = session.resource('s3', endpoint_url=local_s3)

        # 新建一个实验用 bucket (注意："bucket name" 中不能有下划线 )

        if s3.Bucket(bucket_name) not in s3.buckets.all():
            s3.create_bucket(Bucket=bucket_name)
            
    except Exception:
        print('readBucketInit环境创建失败')
    else:
        print('readBucketInit环境创建成功')

    bucket = s3.Bucket(bucket_name)
    
    # 不删除 直接加入 扩充数据集
    # try:
    #     # 删除bucket下所有object
    #     bucket.objects.filter().delete()
    #     print( 'read bucket delete  ' )

    # except botocore.exceptions.ClientError as e:
    #     print('error in bucket removal')    



    #获取用来上传的文件名 如果存在则返回路径 不存在则创建
    def getfile(filesize):
        #命名规则：_test_file + filesize .bin 
        filename = '_write_buffer' + str(filesize) +'.bin'
        path = '_write_buffer/' + filename
        # print('文件路径：',path)
        if os.path.isfile(path):
            return path
        else:
            test_bytes = [0xFF for i in range(filesize)] # 填充至所需大小
            with open(path, "wb") as lf:  
                lf.write(bytearray(test_bytes))
            return path

    failed_requests = []
    readfilename = []

    for i in range(sampleNum):
        try:
            obj_name = "testObj%08d"%(i,) # 所建对象名
            # 准备文件
            size = int(random.expovariate(1.0 / requestSize)) +1 #确保不为0
            # print('filesize:%d'%size)
            try:
                local_file = getfile(size)
            except Exception as e:
                print('获取上传文件失败')
                print(e)    
            # else:
                # print('获取上传文件成功')  
                  
            #文件准备完成  开始上传至 ceph
            s3.Object(bucket_name, obj_name).upload_file(local_file) # 将本地文件上传为对象
        except Exception as e:
            failed_requests.append(e)
        else:
            readfilename.append( obj_name+' '+local_file )

        if i%100 == 0:
            print(i)

    # # 记录文件名到CSV文件
    # readfilenamefile = 'readfilename.csv'    
    # try:
        
    #     with open(readfilenamefile, "w+") as tracefile:
    #         tracefile.writelines([str(r) + '\n' for r in readfilename])
    # except IOError:
    #     print('readfilename文件写入失败')
    # else:
    #     print('readfilename文件写入成功')    

    print('异常数量：',len(failed_requests))       
    for e in failed_requests:
        print(e)      


def main():
    reSize = 1024
    # 循环往 1-32KB的用于读取测试的bucket添加数据
    for i in range( 6 ):
        readBucketInit(requestSize=reSize)
        reSize = reSize * 2
 
if __name__ == '__main__':
    main()    