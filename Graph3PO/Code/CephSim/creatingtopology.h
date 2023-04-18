/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    CreatingTopology（）功能的是原来的创建拓扑的主函数部分，因为需要循环多次，把这段代码封装

    
 */
#ifndef CREATINGTOPOLOGY_H
#define CREATINGTOPOLOGY_H

#include "cephSimDefine.h" //通用的头文件 namespace define数据等
#include "clientApp.h"
#include "serverApp.h"
// #include "transferApp.h"

//浮点数转string
string
double2string (double d)
{
  char *buf = new char[100];
  sprintf (buf, "%5.2f", d);
  string re (buf);
  //取出头尾空格
  re.erase (0, re.find_first_not_of (" "));
  re.erase (re.find_last_not_of (" ") + 1);
  delete[] buf;
  return re;
}

int
CopyFile (string SourceFile, string NewFile)
{
  ifstream in (SourceFile);
  ofstream out;
  string line;
  out.open (NewFile);

  if (in) // 有该文件
    {
      if (!out.is_open ())
        {
          cout << "out 文件打开失败" << NewFile << endl;
          return 0;
        }
      while (getline (in, line)) // line中不包括每行的换行符
        {
          if (line.size () > 2) //非空行
            {
              out << line << endl;
            }
        }
    }
  else
    {
      cout << "读取文件失败 " << SourceFile << endl;
      return 0;
    }
  in.close ();
  out.close ();
  // in.clear();
  // out.clear();
  return 1;
}

//建立NS3仿真器拓扑结构
void
CreatingTopology (string filename)
{
  //读取拓扑结构 结果存在P2PCONFIGS
  cout << "Read Topology Info " << filename << endl;
  ReadCluTopology (filename);

  //————————————————【创建节点 初始化拓扑结构】——————————————
  NodeContainer allNodes;
  allNodes.Create (ALLNODENUM); //所有节点 其中 0 是root 1-4 是host1-4 其他是osd

  //初始化 NODEINFOS  静态链表 存储cluster_map
  cout << "Init cluster map" << endl;
  for (uint32_t i = 0; i < allNodes.GetN (); ++i)
    {
      NodeInfo node;
      node.nodeID = i;
      node.p_node = allNodes.Get (i);
      NODEINFOS.push_back (node);
    }

  // for ( auto config_it = P2PCONFIGS.begin();config_it!=P2PCONFIGS.end();++config_it ) //循环遍历
  // {
  //     config_it->show();
  // }

  // ----------定义 PointToPointHelper 对象 pointToPoint ，负责网络设备的配置------
  vector<PointToPointHelper *> p2phelper_p_vec; //PointToPointHelper 指针数组
  vector<Ipv4AddressHelper *> ip4_helper_p_vec; //Ipv4AddressHelper  指针数组
  for (int i = 0; i < ALLNODENUM - 1; ++i)
    {
      PointToPointHelper *p2p_temp = new PointToPointHelper;
      p2phelper_p_vec.push_back (p2p_temp);
      Ipv4AddressHelper *ip4_temp = new Ipv4AddressHelper;
      ip4_helper_p_vec.push_back (ip4_temp);
    }

  //————————————————【给每个网络设备安装Internet协议栈】——————————————
  // Internet栈
  cout << "Install Internet Stack" << endl;
  InternetStackHelper stack;
  stack.Install (allNodes);

  //搭建NS3 网络结构，同时填入集群拓扑信息 具体实现封装在函数中。
  EstablishNetworkLink (allNodes, p2phelper_p_vec, ip4_helper_p_vec);
  // cout<<"Check Cluster Map Info"<<endl;

  //————————————————【Install APP】——————————————
  int client_count = 0;
  int transfer_count = 0;
  int server_count = 0;
  ApplicationContainer serverApps;
  for (uint32_t i = 0; i < NODEINFOS.size (); ++i)
    {
      auto node = NODEINFOS[i];
      if (node.type == "client") //安装client app
        {
          Ptr<ClientApp> clientapp =
              CreateObject<ClientApp> (); // 创建一个客户端 对象，使 clientapp 指向它

          //将app 安装在节点0上
          node.p_node->AddApplication (clientapp);
          //其他设置操作在app函数内进行 因为所有信息都可以在cluster map NODEINFOS中获取
          clientapp->Setup (i, client_count);
          // 设置启动时间
          clientapp->SetStartTime (Seconds (1.0));
          // 设置停止时间
          clientapp->SetStopTime (Seconds (CLIENTENDTIME));
          client_count++;
        }
      else if (node.type == "server") //安装server app
        {
          Ptr<ServerApp> serverapp =
              CreateObject<ServerApp> (); // 创建一个服务端对象，使 serverapp 指向它
          // 将app 安装在节点1上
          node.p_node->AddApplication (serverapp);

          //setup 将节点指针传入 其他设置操作在app函数内进行
          serverapp->Setup (i, server_count);

          // 设置启动时间
          serverapp->SetStartTime (Seconds (0));
          // 设置停止时间
          serverapp->SetStopTime (Seconds (SERVERENDTIME));
          server_count++;
        }
      else //transfer app 暂时先不写，使用默认
        {
          //————————————————【MyTransferApp 中间节点】——————————————
          // int transfer_count = 0;  //节点数量计数 用于ID编号
          // for( int i=1;i<TRANSFERNUM+1;i++  )   //遍历所有transfer node
          // {
          //   Ptr<TransferApp> transferapp = CreateObject<TransferApp> () ;
          //   vector<Address> serverAddress; // 获取连接的server node的ip
          //   for( int it : NODEINFOS[i].items  )   //遍历 node i 的 后继节点
          //   {
          //     serverAddress.push_back( NODEINFOS[it].address );
          //   }
          //   //setup 将节点指针传入 其他设置操作在app函数内进行
          //   transferapp->Setup(allNodes.Get(i),serverAddress,transfer_count,true);
          //   transfer_count++;
          //   serverAddress.clear();
          //   // 安装在node1
          //   allNodes.Get(i)->AddApplication ( transferapp );
          //   // 设置启动时间
          //   transferapp->SetStartTime(Seconds(0));
          //   // 设置停止时间
          //   transferapp->SetStopTime(Seconds(1000));
          // }
          //————————————————【MyTransferApp END】——————————————
          transfer_count++;
        }
    }
  serverApps.Start (Seconds (1.0));

  // 全局路由
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //   // trace
  //   AsciiTraceHelper ascii;
  //   p2phelper_p_vec[0]->EnableAsciiAll ( ascii.CreateFileStream("cephSimResult/asciiTrace.tr"));

  // 可视化
  // AnimationInterface anim("cephSimResult/anim.xml");
  // MobilityHelper mobility;
  // //配置可视化
  // SetNodeVisualization(allNodes,anim,mobility);

  for (auto it : p2phelper_p_vec)
    {
      delete it;
    }
  for (auto it : ip4_helper_p_vec)
    {
      delete it;
    }
}

//删除运行一次所产生的数据
void
DeleteContextInformation (void)
{
  //清理类中静态成员
  latencyStorage::CleanData ();
  //清理普通 static 变量
  P2PCONFIGS.clear ();
  NODEINFOS.clear ();
  ALLNODENUM = 0;
  SERVERNUM = 0;
  TRANSFERNUM = 0;
}

// 计算在剩余leftN个OSD分配给hostN个主机时候的分配可能性
void
Gethost_vec (vector<vector<int>> &result, vector<int> temp, int leftN, int hostN)
{
  if (leftN <= 0 || hostN <= 0)
    {
      return;
    }
  if (hostN == 1) //最后一个主机全部选剩下的osd
    {
      temp.push_back (leftN);
      result.push_back (temp);
      return;
    }
  for (int i = 1; i <= (leftN - hostN + 1); i++) //当前主机选择几个 必须给后面主机至少每个主机留一个
    {
      temp.push_back (i); //当前选择
      Gethost_vec (result, temp, leftN - i, hostN - 1);
      temp.pop_back ();
    }
}

//批量修改cluster_map信息
void
AutoCreatCluster_map (void)
{

  // 生成不同连接方式，主机数量 1-8，OSD数量 16
  vector<vector<int>> hostN_vec_v; // 第二维的意义是 主机连接的OSD数量
  int osdleft = 16; //剩余需要分配的OSD数量
  for (int hostN = 1; hostN <= 8; hostN++)
    {
      // hostN 为主机数量
      vector<int> host_vec;
      Gethost_vec (hostN_vec_v, host_vec, osdleft, hostN);
    }
  // 用来查看生成连接状态的结果是否正确。
  ofstream file;
  string filename1 ("cephSimResult/host-osd.csv");
  file.open (filename1);
  if (!file.is_open ())
    {
      cout << filename1 << "文件打开失败" << endl;
    }

  for (auto vec : hostN_vec_v)
    {
      int s = 0;
      for (auto i : vec)
        {
          file << i << ",";
          s += i;
        }
      file << vec.size () << endl;
    }
  file.close ();
  cout << "Gethost_vec end" << endl;
  sleep (1);

  vector<P2PConfig> config_Template;

  //读取配置模板
  string filename = "scratch/CephSim2022-12-8/14158_cluster_map.csv";
  std::ifstream in (filename);
  string line;
  // string currentPath(get_current_dir_name());
  // cout<<"work dir: "<<currentPath<<endl;

  if (in) // 有该文件 读取配置信息 写入P2PCONFIGS
    {
      int count = 0; //行号
      while (getline (in, line)) // line中不包括每行的换行符
        {
          ++count;
          if (count == 1)
            continue; //跳过首行 表头
          // cout <<count <<' '<< line << endl;
          vector<string> line_split = stringSplit (line, ',');
          P2PConfig p2pconfig;
          p2pconfig.startNodeID = atoi (line_split[0].c_str ());
          p2pconfig.endNodeID = atoi (line_split[1].c_str ());
          p2pconfig.DataRate = line_split[2];
          p2pconfig.Delay = line_split[3];
          p2pconfig.disk_read_rate = line_split[4];
          p2pconfig.disk_write_rate = line_split[5];
          p2pconfig.disk_read_latency = line_split[6];
          p2pconfig.disk_write_latency = line_split[7];
          p2pconfig.disk_capacity = line_split[8];

          config_Template.push_back (p2pconfig);
        }
    }
  else // 没有该文件
    {
      cout << "Read cluster topology error:no such file " << filename << endl;
    }
  in.close ();

  // 不同连接方式
  for (uint32_t vecI = 0; vecI < hostN_vec_v.size (); vecI++)
    {
      auto vec = hostN_vec_v[vecI];
      int hostNum = vec.size ();
      vector<P2PConfig> temp_conf; //一组配置
      int p2pPoint = 0; //记录下标  走完下面这个for循环 内容会是第一个连接OSD节点链路的配置下标
      for (auto conf : config_Template)
        {
          // 主机连接客户端的设置
          if (conf.startNodeID == 0)
            {
              if (conf.endNodeID <= hostNum)
                {
                  temp_conf.push_back (conf);
                }
              p2pPoint++;
            }
        }

      // 主机连接OSD的设置
      int startHostID = 1;
      int osd_nodeid = hostNum + 1; //第一个osd节点是第几个node
      for (int osdN : vec)
        {
          for (int i = 1; i <= osdN; i++)
            {
              auto conf = config_Template[p2pPoint];
              p2pPoint++;
              conf.startNodeID = startHostID;
              conf.endNodeID = osd_nodeid;
              temp_conf.push_back (conf);
              osd_nodeid++;
            }
          startHostID++;
        }

      // temp_conf 结果写入文件
      char *filename_buf = new char[100];
      sprintf (filename_buf, "scratch/CephSim2022-12-8/cluster_map/%05d_cluster_map.csv", vecI);
      string filename (filename_buf);
      delete[] filename_buf; //指针用完就删

      ofstream file;
      file.open (filename);
      if (!file.is_open ())
        {
          cout << filename << "文件打开失败" << endl;
        }
      file << "startNodeID(int),endNodeID(int),channel_rate(Mbps),channel_delay(ms),disk_read_rate("
              "MB/S),disk_write_rate(MB/"
              "S),disk_read_latency(ms),disk_wirte_latency(ms),diks_capacity(GB)"
           << endl;
      for (auto conf : temp_conf)
        {
          char *write_buf = new char[100];
          sprintf (write_buf, "%d,%d,%s,%s,%s,%s,%s,%s,%s", conf.startNodeID, conf.endNodeID,
                   conf.DataRate.c_str (), conf.Delay.c_str (), conf.disk_read_rate.c_str (),
                   conf.disk_write_rate.c_str (), conf.disk_read_latency.c_str (),
                   conf.disk_write_latency.c_str (), conf.disk_capacity.c_str ());
          file << write_buf << endl;
          delete[] write_buf;
        }
      cout << filename << "文件写入成功" << endl;

      file.close ();
      sleep (0.005);
    }
  cout << "批量创建配置完成" << endl;
}

void
CreateWorkLoad (string path = "scratch/CephSim2022-12-8/workload.csv")
{
  // vector<double>weight{0.053,0.053,0.053,0.053,0.053,0.053,0.053,0.053,0.053,0.032,0.079,0.032,0.032,0.160,0.032,0.160};
  vector<double> weight{372.6, 372.6, 372.6, 372.6, 372.6, 372.6,  372.6, 372.6,
                        372.6, 223.1, 557.9, 223,   223,   1126.4, 222.6, 1126.4};
  vector<int> serverChoice;
  for (uint32_t j = 0; j < weight.size (); ++j)
    {
      for (int i = 0; i < int (weight[j]); ++i)
        {
          serverChoice.push_back (j);
        }
    }
  int capacity_sum = 0;
  capacity_sum = serverChoice.size ();
  int requestN = 1000000;
  vector<vector<int>> req_osd_list;
  size_t replicatedN = 3;
  vector<int> count (int (weight.size ()), 0);
  for (int req = 0; req < requestN; req++)
    {
      vector<int> osd_list;
      set<int> osd_set;
      cout << '[' << req << ']' << endl;
      while (osd_set.size () < replicatedN)
        {
          int chose = serverChoice[rand () % capacity_sum];
          if (osd_set.find (chose) == osd_set.end ())
            {
              osd_list.push_back (chose);
              osd_set.insert (chose);
              count[chose]++;
              cout << chose << ' ';
            }
        }
      req_osd_list.push_back (osd_list);
      cout << endl;
      osd_list.clear ();
      osd_set.clear ();
    }

  // cout<<"count:"<<endl;
  // for(uint64_t i =0;i<weight.size();i++)
  // {
  //     // cout<<i<<endl;
  //     cout<<double(count[i])/requestN/replicatedN<<' ';
  // }

  ofstream file;
  file.open (path);
  if (!file.is_open ())
    {
      cout << path << "文件打开失败" << endl;
    }
  file << "requestID,type";
  for (size_t i = 0; i < replicatedN; i++)
    {
      file << ",osd" << i;
    }
  file << endl;
  for (int i = 0; i < requestN; i++)
    {
      file << i << ",read";
      for (uint64_t j = 0; j < req_osd_list[i].size (); j++)
        {
          file << "," << req_osd_list[i][j];
        }
      file << endl;
    }
}

#endif