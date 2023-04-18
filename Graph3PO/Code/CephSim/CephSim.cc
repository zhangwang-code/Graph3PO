/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    CephSim main函数
    拓扑图 见 仿真器拓扑图.svg
 */

#include "cephSimDefine.h" //通用的头文件 namespace define数据等
#include "latencyStorage.h"
#include "config.h"
#include "creatingtopology.h"
#include <ctime>

NS_LOG_COMPONENT_DEFINE ("CephSim");

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv); 

  //设置日志
  Time::SetResolution (Time::NS);
  LogComponentEnable ("CephSim", LOG_LEVEL_INFO); //启用Log打印提示信息
  
  srand((unsigned)time(NULL)); //设置随机种子
  
  #ifdef IS_CREATE_CLUSTER_MAP
  // // 按照设置的参数上下界进行配置的批量生成 
  AutoCreatCluster_map();
  sleep(100);
  #endif

  #ifdef IS_CREATE_WORKLOAD
  CreateWorkLoad("scratch/CephSim2022-12-8/workload.csv");
  sleep(100);
  #endif
  
  //循环运行RUN_NUM次实验
  for(int i = 0;i<RUN_NUM;++i)
  {
      cout<<"-----------------【Run round "<<i+1<<"/"<<RUN_NUM<<"】-----------------"<<endl;
      //随机抽取拓扑
      float startTime = clock();

      // int rad = rand()%16384;
      // if( rand()%2 == 0)
      // {
      //   rad = rad /10;  //让前面的概率大些
      // }
      // char *clusterfilename_buf = new char[100];
      // char *filename_copy = new char[100];
      // sprintf(clusterfilename_buf,"scratch/CephSim2022-12-8/cluster_map/%05d_cluster_map.csv",rad);
      // sprintf(filename_copy,"cephSimResult/cluster_map_copy/%05d_cluster_map.csv",rad);

      // string clusterfile(clusterfilename_buf),cluster_copy(filename_copy);

      // //指针内容用完就删
      // delete []clusterfilename_buf;
      // delete []filename_copy;

      // CopyFile(clusterfile,cluster_copy);
      // // string clusterfile("scratch/CephSim2022-12-8/60068_cluster_map.csv");

      string clusterfile("scratch/CephSim2022-12-8/cluster_map_mod.csv");
      NS_LOG_INFO("Creating Topology");
      CreatingTopology(clusterfile); //创建链路，安装app
      
      #ifdef LATENCY_SHOW 
      latencyStorage::TestData(); //运行前检查是否数据为空
      #endif

      cout<<"Simulator Start at "<<Simulator::Now().GetSeconds()<<endl;
      Simulator::Run ();
      Simulator::Stop();
      cout<<"Simulator Stop at "<<Simulator::Now().GetSeconds()<<endl;
      Simulator::Destroy ();  //会清除网络设备

      #ifdef LATENCY_SHOW 
      latencyStorage::Show(); 
      #endif

      char *latency_file_path = new char[100];
      char *trace_file_path = new char[100];
      sprintf(latency_file_path,"cephSimResult/latency/latency%03d.csv",i);
      sprintf(trace_file_path,"cephSimResult/packetTrace/packettrace%03d.csv",i);
      latencyStorage::WriteTraceToFile(latency_file_path,trace_file_path,clusterfile);
      delete []latency_file_path;
      delete []trace_file_path;

      if ( i != RUN_NUM -1 )
      {
        cout<<"Clean Environment wait 1 seconds"<<endl;
        sleep(0.5);
        DeleteContextInformation();
        sleep(0.5);
      }
      cout<<"The time of one round is:"<<(clock()-startTime)/ CLOCKS_PER_SEC <<"s"<<endl;
  }
  
  latencyStorage::WritePreViewToFile();  

  return 0;
}
