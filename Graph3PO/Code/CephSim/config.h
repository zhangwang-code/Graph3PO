#ifndef CEPHSIM_CONFIG_H
#define CEPHSIM_CONFIG_H
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    帮助配置集群，生成集群结构，理解上来说就是用于NS3的精简版 Crush_map   包括
    1 用于辅助拓扑结构构造的 配置结构体 P2PConfig NodeInfo等
    2 辅助构建device
    3 辅助构建ipv4
    4 辅助生成Node节点信息
    5 辅助配置可视化节点信息
    6 其他辅助函数
    
 */

#include "cephSimDefine.h" //通用的头文件 namespace define数据等

//存储P2P信道配置
struct P2PConfig
{
  /* data */
  //startNodeID(int),endNodeID(int),channel_rate(Mbps),channel_delay(ms),disk_read_rate(MB/S),disk_write_rate(MB/S),disk_read_latency(ms),disk_write_latency(ms)
  int startNodeID, endNodeID;
  string DataRate, Delay, QueueSize;
  string disk_read_rate, disk_write_rate, disk_read_latency, disk_write_latency;
  string disk_capacity; //容量 crush根据容量来按比例进行数据放置 所以需要写入容量信息

  void
  show ()
  {
    cout << startNodeID << " " << endNodeID << " " << DataRate << " " << Delay << " " << QueueSize;
    cout << disk_read_rate << " " << disk_write_rate << " " << disk_read_latency << " "
         << disk_write_latency << " " << disk_capacity << endl;
  }
};
static vector<P2PConfig> P2PCONFIGS; //存所有配置

//存NS3的Node的信息，主要是存Node的ip地址
struct NodeInfo
{
  int nodeID; //节点编号
  string
      type; //节点类型 root为client 叶子节点为server 中间的为tranfer 分别对应ceph中的client osd bucket
  Ptr<Node> p_node; //node指针
  Address address; //ip
  vector<int> items; //连接的后置node
  NetDeviceContainer device_con; //节点上的网络设备容器
  Ipv4InterfaceContainer ip4_Interface_Con; //device的ip
  string disk_read_rate, disk_write_rate, disk_read_latency, disk_write_latency,
      disk_capacity; //osd节点的读写速率与延迟 只有当type为 server 的时候才有效

  NodeInfo ()
  {
    items.push_back (0);
    items.clear ();
    disk_read_rate = "NULL";
    disk_write_rate = "NULL";
    disk_read_latency = "NULL";
    disk_write_latency = "NULL";
    disk_capacity = "NULL";
  }
  void
  show ()
  {
    cout << "Node ID:" << nodeID << " type:" << type << " address:" << address;
    if (type != "server") //非叶子结点才有后续节点
      {
        cout << " next node id:";
        for (auto item : items)
          {
            cout << item << " ";
          }
      }
    cout << "device's address: ";
    for (uint32_t i = 0; i < device_con.GetN (); ++i)
      {
        cout << device_con.Get (i)->GetAddress () << " ";
      }
    cout << "ipv4 address: ";
    for (uint32_t i = 0; i < ip4_Interface_Con.GetN (); ++i)
      {
        cout << ip4_Interface_Con.GetAddress (i) << " ";
      }
    if (type == "server") //osd节点的性质
      {
        printf ("server performance: read rate %sMS/s read latency %sms write rate %sMB/s write "
                "latency %sms capacity %sGB",
                disk_read_rate.c_str (), disk_write_rate.c_str (), disk_read_latency.c_str (),
                disk_write_latency.c_str (), disk_capacity.c_str ());
        // cout<<disk_read_rate<<" "<<disk_write_rate<<" "<<disk_read_latency<<" "<<disk_write_latency;
      }
    cout << endl;
  }
  //检查node节点device ip
  void
  checkIp (void)
  {
    cout << "check node device ip: ";
    for (uint32_t i = 0; i < p_node->GetNDevices (); ++i)
      {
        cout << p_node->GetDevice (i)->GetAddress () << " ";
      }
    cout << endl;
  }
};
static vector<NodeInfo> NODEINFOS; //全局变量 存所有节点信息

// split功能实现
std::vector<std::string>
stringSplit (const std::string &str, const char delim)
{
  std::stringstream ss (str);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline (ss, item, delim))
    {
      if (!item.empty ())
        {
          elems.push_back (item);
        }
    }
  return elems;
}

//读取拓扑结构 结果存在P2PCONFIGS
void
ReadCluTopology (string filename)
{
  //读取配置

  ifstream in;
  in.open (filename);
  string line;
  // string currentPath(get_current_dir_name());
  // cout<<"work dir: "<<currentPath<<endl;
  if (in.is_open ()) // 有该文件 读取配置信息 写入P2PCONFIGS
    {
      int count = 0; //行号
      while (getline (in, line)) // line中不包括每行的换行符
        {
          ++count;
          if (count == 1)
            continue; //跳过首行 表头

#ifdef TOPOLOGY_CREATE_LOG
          cout << count << " |" << line << endl;
#endif

          vector<string> line_split = stringSplit (line, ',');
          P2PConfig p2pconfig;
          p2pconfig.startNodeID = atoi (line_split[0].c_str ());
          p2pconfig.endNodeID = atoi (line_split[1].c_str ());
          p2pconfig.DataRate = line_split[2] + "Mbps";
          p2pconfig.Delay = line_split[3] + "ms";
          p2pconfig.QueueSize = "10000p"; //先保证不会队列超出
          p2pconfig.disk_read_rate = line_split[4];
          p2pconfig.disk_write_rate = line_split[5];
          p2pconfig.disk_read_latency = line_split[6];
          p2pconfig.disk_write_latency = line_split[7];
          p2pconfig.disk_capacity = line_split[8];
          P2PCONFIGS.push_back (p2pconfig);
        }
      ALLNODENUM = P2PCONFIGS.size () + 1;
      vector<int> node_next_num (ALLNODENUM, 0);
      for (auto p2p : P2PCONFIGS)
        {
          node_next_num[p2p.startNodeID]++; //后继节点数量加一
        }
      for (auto i : node_next_num)
        {
          if (i == 0)
            {
              SERVERNUM++; //服务节点数量 + 1
            }
        }
      TRANSFERNUM = ALLNODENUM - SERVERNUM - 1;
      printf ("Node num:%d Server num:%d Transfer num:%d\n", ALLNODENUM, SERVERNUM, TRANSFERNUM);
    }
  else // 没有该文件
    {
      cout << "Read cluster topology error:no such file " << filename << endl;
    }
  in.close ();
}

//建立网络链路
void
EstablishNetworkLink (NodeContainer &allNodes, vector<PointToPointHelper *> &p2phelper_p_vec,
                      vector<Ipv4AddressHelper *> &ip4_helper_p_vec)
{
  for (uint32_t i = 0; i < P2PCONFIGS.size (); ++i) //循环遍历 创建链路 分配ip等
    {
#ifdef TOPOLOGY_CREATE_LOG
      cout << "链路创建：" << i + 1 << '/' << P2PCONFIGS.size () << endl;
#endif
      PointToPointHelper *p2phelper_p = p2phelper_p_vec[i];
      P2PConfig p2pconfig = P2PCONFIGS[i];
      Ipv4AddressHelper *ip4helper_p = ip4_helper_p_vec[i];
      string
          network; //创建对应网络 格式为  10.startNodeID.endNodeID.0 子网掩码：255.255.255.0 （如果节点数量大于255，则需要重新设计格式）
      network =
          "10." + to_string (p2pconfig.startNodeID) + "." + to_string (p2pconfig.endNodeID) + ".0";

// p2pconfig.show();
#ifdef TOPOLOGY_CREATE_LOG
      cout << "check network:" << network << endl;
#endif

      //设置p2p
      p2phelper_p->SetDeviceAttribute ("DataRate", StringValue (p2pconfig.DataRate));
      p2phelper_p->SetChannelAttribute ("Delay", StringValue (p2pconfig.Delay));
      p2phelper_p->SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (p2pconfig.QueueSize));

      NetDeviceContainer temp_device_con; //临时存放 PointToPointHelper 创建出来的device
      NodeContainer temp_node_con; //临时存放 PointToPointHelper 使用的参数 节点
      Ipv4InterfaceContainer temp_ip_con; //临时存放 ip

      temp_node_con.Add (allNodes.Get (p2pconfig.startNodeID));
      temp_node_con.Add (allNodes.Get (p2pconfig.endNodeID));

      temp_device_con = p2phelper_p->Install (temp_node_con); //生成设备

      //设置网络
      ip4helper_p->SetBase (network.c_str (), "255.255.255.0");
      temp_ip_con = ip4helper_p->Assign (temp_device_con); //分配ip

      //内容写入NODEINFOS
      NODEINFOS[p2pconfig.startNodeID].items.push_back (p2pconfig.endNodeID); //后继节点
      NODEINFOS[p2pconfig.startNodeID].device_con.Add (
          temp_device_con.Get (0)); //存 链路前一个节点的netdevice
      NODEINFOS[p2pconfig.endNodeID].device_con.Add (
          temp_device_con.Get (1)); //存 链路 后一个节点的netdevice

      NODEINFOS[p2pconfig.startNodeID].ip4_Interface_Con.Add (
          temp_ip_con.Get (0)); //存 链路前一个节点的ip
      NODEINFOS[p2pconfig.endNodeID].ip4_Interface_Con.Add (
          temp_ip_con.Get (1)); //存 链路后一个节点的ip

      NODEINFOS[p2pconfig.endNodeID].disk_read_rate = p2pconfig.disk_read_rate;
      NODEINFOS[p2pconfig.endNodeID].disk_write_rate = p2pconfig.disk_write_rate;
      NODEINFOS[p2pconfig.endNodeID].disk_read_latency = p2pconfig.disk_read_latency;
      NODEINFOS[p2pconfig.endNodeID].disk_write_latency = p2pconfig.disk_write_latency;
      NODEINFOS[p2pconfig.endNodeID].disk_capacity = p2pconfig.disk_capacity;
    }
  for (uint32_t i = 0; i < NODEINFOS.size (); ++i) //设置节点类型
    {
      //判断类型
      if (i == 0) //root
        {
          NODEINFOS[i].type = "client";
        }
      else if (NODEINFOS[i].items.size () < 1) //叶子节点 服务节点
        {
          NODEINFOS[i].type = "server";
        }
      else
        {
          NODEINFOS[i].type = "transfer";
        }

      NODEINFOS[i].address = NODEINFOS[i].ip4_Interface_Con.GetAddress (0); //默认选用第一个设备地址
#ifdef TOPOLOGY_CREATE_LOG
      NODEINFOS[i].show ();
#endif
      // NODEINFOS[i].checkNode();
    }
}

//获取服务节点的address 方法：遍历判断是否叶子节点
vector<Address>
GetServerAddress (void)
{
  vector<Address> temp_add_vec;
  for (auto nodeinfo_it = NODEINFOS.begin (); nodeinfo_it != NODEINFOS.end (); ++nodeinfo_it)
    {
      if (nodeinfo_it->items.size () < 1) //叶子节点 服务节点
        {
          temp_add_vec.push_back (nodeinfo_it->address);
        }
    }
  return temp_add_vec;
}

//添加可视化描述 自动生成节点位置 目前只支持三层结构
void
SetNodeVisualization (NodeContainer &allnode, AnimationInterface &anim, MobilityHelper &mobility)
{

  //设置节点位置
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // root (node0)
  // mobility 使用 Install( ( nodes) ) 函数在 nodes包含的节点上安装
  mobility.Install (allnode);

  // for ( int i=0;i<int(allnode.GetN());++i )
  // {
  //     mobility.Install(allnode.Get(i));
  // }

  //存  Ptr<ConstantPositionMobilityModel>
  vector<Ptr<ConstantPositionMobilityModel>> p_CPMM;
  for (int i = 0; i < int (allnode.GetN ()); ++i)
    {
      Ptr<ConstantPositionMobilityModel> temp =
          allnode.Get (i)->GetObject<ConstantPositionMobilityModel> ();
      p_CPMM.push_back (temp);
    }

  /*
    位置 规则：
        摆放方式仿照拓扑图.svg
        第一列 横坐标 起点1 最后一列横坐标 98 纵轴 上下同样为 1和98
        纵坐标按照当列数量均匀排列

    */

  const double x_len = (98.0 - 1.0) / (LAYERNUM - 1); // 每一列的距离

  //root
  p_CPMM[0]->SetPosition (Vector (1.0, 49.0, 0));

  //host
  double y_len = (98.0 - 1.0) / (HOSTNUM + 1);
  for (int i = 0; i < HOSTNUM; ++i)
    {
      p_CPMM[i + 1]->SetPosition (Vector (1.0 + x_len, (i + 1) * y_len, 0));
    }

  //osd
  y_len = (98.0 - 1.0) / (OSDNUM + 1);
  for (int i = 0; i < OSDNUM; ++i)
    {
      p_CPMM[i + 1 + HOSTNUM]->SetPosition (Vector (1.0 + x_len * 2, (i + 1) * y_len, 0));
    }

  //给可视化节点添加描述
  anim.UpdateNodeDescription (0, "Root");
  anim.UpdateNodeDescription (1, "Host1");
  anim.UpdateNodeDescription (2, "Host2");
  anim.UpdateNodeDescription (3, "Host3");
  anim.UpdateNodeDescription (4, "Host4");

  anim.UpdateNodeDescription (5, "OSD0");
  anim.UpdateNodeDescription (6, "OSD1");
  anim.UpdateNodeDescription (7, "OSD2");

  anim.UpdateNodeDescription (8, "OSD3");
  anim.UpdateNodeDescription (9, "OSD4");

  anim.UpdateNodeDescription (10, "OSD5");
  anim.UpdateNodeDescription (11, "OSD6");

  anim.UpdateNodeDescription (12, "OSD7");
  anim.UpdateNodeDescription (13, "OSD8");
}

#endif