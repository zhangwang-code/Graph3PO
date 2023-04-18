#ifndef CLIENTAPP_H
#define CLIENTAPP_H
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    实现 client app
    
 */

#include "cephSimDefine.h" //通用的头文件 namespace define数据等
#include "latencyStorage.h"
#include "config.h"

//————————————————————————————————【MyClientApp Start】————————————————————————————————
/*
    安装在ROOT节点上的APP 
    功能：
    1 使用sender socket 给下一层节点发包 
    2 使用rev socket 接受反馈 计算延迟
 */

class ClientApp : public Application
{
public:
  ClientApp (); // 构造函数
  virtual ~ClientApp (); // 析构函数
  static TypeId GetTypeId (void); // 设置ClientApp 的属性系统
  //Setup 函数通过参数值给ClientApp 的私有成员变量赋值
  void Setup (int index, int clientid);

private:
  virtual void StartApplication (void); // 启动ClientApp
  virtual void StopApplication (void); // 停止ClientApp

  int node_index; //安装所在node的在NODEINFOS的index（下标）
  int client_id; //该client app 的编号

  void ReceivePacket (Ptr<Socket> socket); //收到数据包
  void ScheduleTx (void); // 生成下一个包的到达事件
  void SendPacket (void); // 发送数据包
  void ProcessPacket (Ptr<Packet> packet); //接收包后的处理
  void ReadWorkLoadFile (string path);
  void ReadOSDWaitTimePreFile (string path);

  vector<Ptr<Socket>> m_sendSockets; // Socket 指针数组，负责建立socket 连接
  Ptr<Socket> m_revsocket; //rev socket 用于接受信息
  vector<Address> m_nextNodePeers; // 远端地址数组

  Ptr<Node> m_pnode; //安装的节点的指针

  EventId m_sendEvent; // 发送事件
  bool m_running; // 运行状态
  uint32_t m_nPackets; //需要发送到数据包
  uint32_t m_packetsSent; // 已发送数据包
  uint32_t m_readNum; //read 数量
  uint32_t m_writeNum; //write 数量
  float m_readProportion; //read 所占百分比 值在0-100之间

  // 指数随机变量pArrival 产生随机数作为包的发送间隔
  Ptr<ExponentialRandomVariable> pArrival;
  //产生随机数作为数据包大小
  Ptr<ExponentialRandomVariable> pSize;

  vector<int> requestSum; //记录每个nextNode上接受的请求数量
  int nextNodeNum; //后继节点的数量

  unordered_map<uint64_t, double> sendTimeMap; //存每个packet发出的时间 id ：time(Seconds)
  unordered_map<uint64_t, double> revTimeMap; //存每个packet收到的时间 id : time(Seconds)

  int m_disk_capacity_sum; //记录容量总量
  vector<float> m_disk_capacity;
  vector<int>
      m_serverChoice; //用于按照osd容量进行server的抽签 原算法为：比如osd.0有300GB，则插入300个0，osd.1有500GB，则插入500个1，最后在所有下标中随机抽取，抽到的下标对应的内容就是所选的server

  struct Request
  {
    /* data */
    string type;
    vector<uint64_t> osd_list;
  };

  vector<ClientApp::Request> m_requets_list;

  vector<vector<double>> m_osd_wait_time_predict; //依据GCN方法预测的osd等待时间
};

ClientApp::ClientApp () : m_sendEvent (), m_running (false), m_nPackets (0), m_packetsSent (0)
{
}

ClientApp::~ClientApp ()
{
  // StopApplication();
  // m_sendSockets.clear();
  // m_nextNodePeers.clear();
}

//setup 函数，根据参数设置私有成员属性的值
void
ClientApp::Setup (int index, int clientid)
{
//以下语句均为赋值
#ifdef APP_RUN_LOG
  cout << "ClientAPP" << clientid << " Setup" << endl;
#endif
  m_nextNodePeers = GetServerAddress (); //获取服务端address
  node_index = index; //节点号
  client_id = clientid; //app id
  m_nPackets = RENUM; //发送的包数
  nextNodeNum = m_nextNodePeers.size (); //发送目标的数量 用来随机
  m_pnode = GetNode (); //获取安装的node的指针
  m_readProportion = READProportion;
  // cout<<"nextnodeNum:"<<nextNodeNum<<endl;
}

void
ClientApp::ReadWorkLoadFile (string path = "scratch/CephSim2022-12-8/workload.csv")
{
  std::ifstream in (path);
  string line;
  if (in) // 有该文件 读取配置信息 写入P2PCONFIGS
    {
      int count = 0; //行号
      while (getline (in, line)) // line中不包括每行的换行符
        {
          ++count;
          if (count == 1)
            continue; //跳过首行 表头
          vector<string> line_split = stringSplit (line, ',');
          Request req;
          req.type = line_split[1];
          for (uint64_t i = 2; i < line_split.size (); i++)
            {
              req.osd_list.push_back (atoi (line_split[i].c_str ()));
            }
          m_requets_list.push_back (req);
        }
    }
  else // 没有该文件
    {
      cout << "Read cluster topology error:no such file " << path << endl;
    }
  in.close ();

  //check
  // for(auto req : m_requets_list)
  // {
  //   for(auto osd : req.osd_list)
  //   {
  //     cout<<osd<<' ';
  //   }
  //   cout<<endl;
  // }
  // sleep(10);
}

void
ClientApp::ReadOSDWaitTimePreFile (string path = "scratch/CephSim2022-12-8/osdWaitTimePredict.csv")
{
  std::ifstream in (path);
  string line;

  if (in) // 有该文件 读取配置信息 写入P2PCONFIGS
    {
      int count = 0; //行号
      while (getline (in, line)) // line中不包括每行的换行符
        {
          ++count;
          // if(count == 1)
          //     continue;   //跳过首行 表头
          vector<string> line_split = stringSplit (line, ',');
          vector<double> temp;
          for (uint64_t i = 0; i < line_split.size (); i++)
            {
              temp.push_back (atof (line_split[i].c_str ()));
            }
          m_osd_wait_time_predict.push_back (temp);
        }
    }
  else // 没有该文件
    {
      cout << "Read cluster topology error:no such file " << path << endl;
    }
  in.close ();
  //check
  // for(auto x:m_osd_wait_time_predict)
  // {
  //   for(auto y : x)
  //   {
  //     cout<<y<<' ';
  //   }
  //   cout<<endl<<endl;
  // }
  // sleep(100);
}

/* static */
TypeId
ClientApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ClientApp")
                          .SetParent<Application> ()
                          .SetGroupName ("CephSim")
                          .AddConstructor<ClientApp> ();
  return tid;
}

void
ClientApp::StartApplication (void) // 启动应用
{
#ifdef APP_RUN_LOG
  cout << Simulator::Now ().GetSeconds () << "s ClientAPP Start" << endl;
#endif
  // cout<<"Check Server Address"<<endl;
  // for( auto add : m_nextNodePeers  )
  // {
  //   cout<<add<<endl;
  // }

  // 指数随机变量PArrival
  pArrival = CreateObject<ExponentialRandomVariable> ();
  pArrival->SetAttribute ("Mean", DoubleValue (REWAIT)); // 平均值
  pArrival->SetAttribute ("Bound", DoubleValue (0)); // 下界
  // 指数随机变量Psize
  pSize = CreateObject<ExponentialRandomVariable> ();
  pSize->SetAttribute ("Mean", DoubleValue (RESIZE)); // 平均值，包大小
  pSize->SetAttribute ("Bound", DoubleValue (0)); // 下界， 包大小

  m_running = true; // 将m_running 设为true ，表明程序为运行状态
  m_packetsSent = 0; // 已发包的数目m_packetsSent =0
  m_readNum = 0;
  m_writeNum = 0;

  //Socket options for IPv4, currently TOS, TTL, RECVTOS, and RECVTT
  // bool ipRecvTos = true;
  // bool ipRecvTtl = true;

  //Receiver socket on server node
  m_revsocket = Socket::CreateSocket (m_pnode, UdpSocketFactory::GetTypeId ());
  // m_revsocket->SetIpRecvTos (ipRecvTos);
  // m_revsocket->SetIpRecvTtl (ipRecvTtl);

  InetSocketAddress local = InetSocketAddress (InetSocketAddress (
      Ipv4Address::ConvertFrom (NODEINFOS[node_index].address), RECEIVEPORT)); //作为接收端的地址
  if (m_revsocket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("client revsocket Failed to bind");
    }
  m_revsocket->SetRecvCallback (MakeCallback (&ClientApp::ReceivePacket, this));
  // m_revsocket->SetAllowBroadcast (true);

  // uint32_t ipTos = 5;
  // uint32_t ipTtl = 5;

  //创建send socket
  for (int i = 0; i < nextNodeNum; ++i)
    {
      Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (m_pnode, UdpSocketFactory::GetTypeId ());
      Address m_peer = m_nextNodePeers[i];
      // cout<<"node "<<i<< " address:"<<m_peer<<endl;
      //连接远端地址 绑定本地地址
      if (Ipv4Address::IsMatchingType (m_peer) == true)
        {
          if (ns3UdpSocket->Bind (InetSocketAddress (
                  Ipv4Address::ConvertFrom (NODEINFOS[node_index].address), SENDPORT + i)) == -1)
            {
              cout << i << " client send UdpSocket bing fail!" << endl;
            }
          if (ns3UdpSocket->Connect (
                  InetSocketAddress (Ipv4Address::ConvertFrom (m_peer), RECEIVEPORT)) == -1)
            {
              cout << i << " client send UdpSocket connect fail!" << endl;
            }
        }
      else if (InetSocketAddress::IsMatchingType (m_peer) == true)
        {
          if (ns3UdpSocket->Bind (InetSocketAddress (
                  Ipv4Address::ConvertFrom (NODEINFOS[node_index].address), SENDPORT + i)) == -1)
            {
              cout << i << " client send UdpSocket bing fail!" << endl;
            }
          if (ns3UdpSocket->Connect (m_peer) == -1)
            {
              cout << i << " client send UdpSocket connect fail!" << endl;
            }
        }

      ns3UdpSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
      // ns3UdpSocket->SetAllowBroadcast (true);
      m_sendSockets.push_back (ns3UdpSocket); //存入socket 资源 vector
      requestSum.push_back (0);
    }
  // cout<<"client send socket check:"<<endl;  //看是否和服务节点的ip一致
  // for( auto socket : m_sendSockets )
  // {
  //   Address add ;
  //   socket->GetPeerName(add);
  //   cout<<add<<endl;
  // }

  //准备server抽签用的数组
  for (auto node : NODEINFOS)
    {
      if (node.type == "server")
        {
          m_disk_capacity.push_back (atof (node.disk_capacity.c_str ())); //存入每个server的容量
        }
    }

  for (uint32_t j = 0; j < m_disk_capacity.size (); ++j)
    {
      for (int i = 0; i < int (m_disk_capacity[j]); ++i)
        {
          m_serverChoice.push_back (j);
        }
    }
  m_disk_capacity_sum = m_serverChoice.size ();

  //读取负载信息
  cout << "Read WorkLoad" << endl;
  ReadWorkLoadFile ("scratch/CephSim2022-12-8/workload.csv");

#ifdef IS_USE_OSD_WAIT_TIME
  //读取osd等待时间的预测信息
  cout << "Read osdWaitTimePredict.csv" << endl;
  ReadOSDWaitTimePreFile ("scratch/CephSim2022-12-8/osdWaitTimePredict.csv");
#endif

  //开始发送包
  if (m_nPackets > 0)
    {
      SendPacket ();
    }
  else
    {
      cout << "数据包数量有误" << endl;
      StopApplication ();
    }
}

void
ClientApp::StopApplication (void) // 停止应用
{
  m_running = false; // 运行状态设为false
  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent); // 取消发送事件
    }

  for (int i = 0; i < nextNodeNum; ++i)
    {
      Ptr<Socket> m_socket = m_sendSockets[i];
      if (m_socket)
        {
          m_socket->Close (); // 关闭socket 连接
        }
    }

  cout << "Percentage of capacity of each server and actual percentage of requests:(Should be "
          "similar)"
       << endl;
  for (auto cap : m_disk_capacity)
    {
      printf ("%5.3f ", cap / m_disk_capacity_sum);
    }
  cout << endl;
  for (auto it : requestSum)
    {
      printf ("%5.3f ", (it + 0.00001) / m_nPackets);
    }
  cout << endl;

  cout << "request num in servers:" << endl;
  for (auto it : requestSum)
    {
      cout << it << " ";
    }
  cout << endl;
  cout << "workload:read num " << m_readNum << " write num " << m_writeNum << endl;

#ifdef APP_RUN_LOG
  cout << Simulator::Now ().GetSeconds () << "s Client延迟数据写入" << endl;
#endif
  latencyStorage::AddClientSendTime (sendTimeMap);

  latencyStorage::AddClientRevTime (revTimeMap);
#ifdef APP_RUN_LOG
  cout << Simulator::Now ().GetSeconds () << "s ClientAPP END" << endl;
#endif
}

void
ClientApp::SendPacket (void) // 发送数据包
{
  int pvalue = int (ceil (pSize->GetValue ())); // 指数分布随机变量pSize 获得一个随机做为包大小
  while (pvalue <= 1 || pvalue > RESIZEMAX) // 小于1或大于最大值则重新生成
    {
      pvalue = int (ceil (pSize->GetValue ())); //保证大于1
    }
  Ptr<Packet> packet; //发送的包
  MyTag tag;
  //读写 还没写负载 先用随机代替
  int flag = rand () % 100 + 1; //判断读写
  if (flag <= m_readProportion) //读  发一个小packet tag值是读的大小
    {
      packet = Create<Packet> ();
      packet->RemoveAllByteTags ();
      packet->RemoveAllPacketTags (); //移除原有tag
      tag.SetSimpleValue (uint32_t (pvalue));
      // store the tag in a packet.
      packet->AddPacketTag (tag);
      m_readNum++;
    }
  else //写 发送一个 pvalue大小的包 tag值为 1
    {
      packet = Create<Packet> (pvalue); // 根据pvalue 值创建一个相应大小的数据包
      packet->RemoveAllByteTags ();
      packet->RemoveAllPacketTags (); //移除原有tag
      tag.SetSimpleValue (uint32_t (1));
      // store the tag in a packet.
      packet->AddPacketTag (tag);
      m_writeNum++;
    }

  // int r = rand();
  // // r = 0 ;  //固定发往一个osd，测试排队代码是否可用
  // int nextNodeid = m_serverChoice[r%m_disk_capacity_sum] ; // 按照server容量 为概率抽签 获得一个随机做为nextNode标号

  int nextNodeid = m_requets_list[m_packetsSent].osd_list[0]; //主OSD

#ifdef IS_USE_OSD_WAIT_TIME
  // 使用预测结果进行优化请求发送
  if (rand () % 100 < 50) //降低调度影响，一半情况还是用主OSD，一半情况判断osd状态
    {
      int time = Simulator::Now ().GetSeconds () - 1; //已经过去了多久
      vector<double> osdWait = m_osd_wait_time_predict[time % m_osd_wait_time_predict.size ()];
      map<double, int> wait_osdid;
      for (auto id : m_requets_list[m_packetsSent].osd_list)
        {
          wait_osdid[osdWait[id + (ALLNODENUM - SERVERNUM)]] = id;
        }
      nextNodeid = wait_osdid.begin ()->second; //等待时间最小的osd
    }
#endif

  requestSum[nextNodeid]++;

  Ptr<Socket> m_socket = m_sendSockets[nextNodeid]; //获取对应socket

  sendTimeMap.insert (unordered_map<uint64_t, double>::value_type (
      packet->GetUid (), Simulator::Now ().GetSeconds ())); //存放send时间

#ifdef DEBUG_LOG_PACKET
  int send_res;
  send_res = m_socket->Send (packet); // 发送当前包 结果存在send_res
  // Address sendAddress;
  // m_socket->GetPeerName(sendAddress);
  // cout<<Simulator::Now().GetSeconds()<<"s client send to Node:"<<nextNodeid+5<<" UID: "<<packet->GetUid()<<" tag: "<<uint32_t(tag.GetSimpleValue())<<" res: "<<send_res<<" address:"<<sendAddress<<endl;
  cout << Simulator::Now ().GetSeconds () << "s client send to Node:" << nextNodeid + 5
       << " UID: " << packet->GetUid () << " tag: " << uint32_t (tag.GetSimpleValue ())
       << " res: " << send_res << endl;
#endif

  //给不同的请求设置不同的sla
  double sloChoose = (rand () % 1000000) / 10000.0; //归一化为百分比
  double slo; //单位 ms
  if (sloChoose < 50.0) //百分之50的请求
    {
      slo = 100;
    }
  else if (sloChoose < 60.0)
    {
      slo = 100;
    }
  else if (sloChoose < 70.0)
    {
      slo = 100;
    }
  else if (sloChoose < 80.0)
    {
      slo = 100;
    }
  else if (sloChoose < 90.0)
    {
      slo = 100;
    }
  else if (sloChoose < 99.0)
    {
      slo = 200.0;
    }
  else if (sloChoose < 99.9)
    {
      slo = 250.0;
    }
  else if (sloChoose < 99.99)
    {
      slo = 280.0;
    }
  else
    {
      slo = 282.114;
    }

  latencyStorage::AddSLA (packet->GetUid (), slo / 1000.0); //加入sla 单位（s）
  m_socket->Send (packet);

  // 如果已发送包的数目小于总的包的数目，调用ScheduleTx 函数并使m_packetsSent+1
  if (++m_packetsSent < m_nPackets)
    {
      if (m_packetsSent % (m_nPackets / 10) == 0 ||
          m_packetsSent == m_nPackets) //每十分之一进度显示一次
        {
          cout << "already send:" << m_packetsSent << "/" << m_nPackets << endl;
        }
      ScheduleTx (); // 设置下一个包的调度事件
    }
}

void
ClientApp::ScheduleTx (void) // 生成下一个包的到达事件
{
  if (m_running) // 启动应用后m_running 设置为true, 应用停止后设置为false
    {
      double value = pArrival->GetValue (); // 使用指数随机变量生成发送下一个包的时间间隔
      Time tNext (Seconds (value)); // 将value 转换成时间变量tNext
      // tNext=  (Seconds (0)); // 连续发送 测试排队
      m_sendEvent = Simulator::Schedule (tNext, &ClientApp::SendPacket, this); //发送事件
      // 经过tNext 时间后，调用SendPacket 函数发送下一个包
    }
}

void
ClientApp::ReceivePacket (Ptr<Socket> socket) //收到数据包
{
  Ptr<Packet> packet = socket->Recv ();
  revTimeMap.insert (unordered_map<uint64_t, double>::value_type (
      packet->GetUid (), Simulator::Now ().GetSeconds ())); //存放rev时间
  ProcessPacket (packet); //将packet传给ProcessPacket进行处理
}

void
ClientApp::ProcessPacket (Ptr<Packet> packet)
{
  //packet 处理过程
  // read the tag from the packet copy
  MyTag tagCopy;
  packet->PeekPacketTag (tagCopy);
  uint32_t tagValue = uint32_t (tagCopy.GetSimpleValue ());
#ifdef DEBUG_LOG_PACKET
  cout << Simulator::Now ().GetSeconds ()
       << "s Client Received one packet! UID: " << packet->GetUid () << " tag: " << tagValue
       << endl;
#endif

  // packet->PrintPacketTags (cout);  //输出所有tag
  // cout << endl;
  // int packeSize = int(packet->GetSize()) ;
  //通过tag 判断操作类型 tag值为1代表写 写大小即为packet大小 tag值大于1代表读 值即为读的packet大小。
  if (tagValue != 1) //读
    {
      //Sim中只管收到返回结果的时间，已经由LatencyStorage类记录。
    }
  else //写
    {
    }
  //延迟结果写入
}
//————————————————————————————————【MyClientApp   END】————————————————————————————————
#endif