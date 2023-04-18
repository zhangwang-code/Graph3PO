/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    实现 server app 对应s3中的 osd 概念
 */

#ifndef SERVERAPP_H
#define SERVERAPP_H

#include "cephSimDefine.h" //通用的头文件 namespace define数据等
#include "latencyStorage.h"
#include <numeric> //线性拟合的时候用
#include <unordered_map>
#include <atomic>

//————————————————————————————————【MyServerApp   Start】————————————————————————————————
/*
    安装在OSD节点上的APP 
    功能：
    1 使用rev socket 接受数据包 做出行动 
    2 使用sender socket 给上一层节点反馈 
 */

/// @brief
class ServerApp : public Application
{
public:
  ServerApp (); // 构造函数
  virtual ~ServerApp (); // 析构函数
  static TypeId GetTypeId (void); // 设置ServerApp 的属性系统
  //Setup 函数通过参数值给ServerApp 的私有成员变量赋值
  void Setup (int index, int serverid);

private:
  virtual void StartApplication (void); // 启动ServerApp
  virtual void StopApplication (void); // 停止ServerApp

  void ReceivePacket (Ptr<Socket> socket); //收到数据包

  void ReadProcess (uint32_t tagValue, uint64_t origUID); //读操作
  void WriteProcess (uint32_t tagValue, uint64_t origUID); //写操作

  Time CaculateReadTime (uint32_t size); //计算读操作的时间 输入为处理大小 单位 B
  Time CaculateWriteTime (uint32_t size); //计算写操作的时间 输入为处理大小 单位 B

  void SendPacket (Ptr<Packet> packet); // 返回数据
  void ProcessPacket (Ptr<Packet> packet); //接收包后的处理

  void Queue (void); //用于处理排队

  /// @brief 最小二乘法做线性拟合
  /// @param Y y参数列表
  /// @return 计算违例风险 返回值等于 0-预测点的剩余SLO百分比，即结果小于0则无风险，大于0则有风险，值为风险大小
  double CacuRiskUseLeastSquares (std::vector<double> &Y);

  int m_serverID; //id
  int node_index; //安装所在node的在NODEINFOS的index（下标）

  bool m_running; // 运行状态
  std::atomic<bool> m_disk_is_available; //记录当前osd的磁盘是否被使用
  std::atomic<bool>
      m_dispatch_lock; //记录调度 即同一时间只能进行一次调度  //TODO 可能用不上，最后上传代码需要检查

  Ptr<Socket> m_UdpSocket; //rev socket 用于接受信息 发送信息

  Ptr<Node> m_pnode; //安装app 的节点 用来创建socket

  Address m_rootPeer; // 远端地址 根节点

  unordered_map<uint64_t, double> m_sendTimeMap; //存每个packet发出的时间 id ：time(Seconds)
  unordered_map<uint64_t, double> m_revTimeMap; //存每个packet收到的时间 id : time(Seconds)
  unordered_map<uint64_t, double> m_processTimeMap; //存每个packet被处理的时间 id : time(Seconds)
  unordered_map<uint64_t, uint64_t> m_uidQueueLen; //存UID请求处理的时候 请求队列的长度

  queue<Ptr<Packet>> m_queue_packet; //存待处理的packet
  queue<Ptr<Packet>> m_priority_queue; //存优先处理的延迟敏感数据

  float m_disk_read_rate, m_disk_write_rate, m_disk_read_latency,
      m_disk_write_latency; //存储设备性能数据 单位MB/s ms

  FILE *serverTrace; //用来记录调度的trace

  /// @brief  在调度过程中用来存储有违例风险的packt以及在队列原来的位置，用来计算cost
  struct RiskPacket
  {
    /* data */

    Ptr<Packet> packet_p;
    int pos; //位置
    double cost; //调度对其他请求的影响
    double leftSLA; //剩余的sla
    double
        lambda; //重要度权重，表示目前队列中越靠前的请求越重要，调度对其影响越大，所以pos越小，lambda越大
    double risk; //风险程度

    RiskPacket ()
    {
    }
    RiskPacket (Ptr<Packet> pkt, int p, double r)
    {
      packet_p = pkt;
      pos = p;
    }
    bool
    operator<(const RiskPacket &t) const
    {
      return risk > t.risk;
    }
  };

  /// @brief 剩余百分比的列表 用来做线性拟合,key是packet UID
  unordered_map<uint64_t, vector<double>> m_leftSLAPercentage;

}; //serverapp声明结束

ServerApp::ServerApp ()
{
}

ServerApp::~ServerApp ()
{
}

void //setup 函数，根据参数设置私有成员属性的值
ServerApp::Setup (int index, int serverid)
{
#ifdef APP_RUN_LOG
  cout << "ServerAPP " << serverid << " Setup" << endl;
#endif
  //赋值语句
  node_index = index;
  m_rootPeer = NODEINFOS[0].address; //默认第一个为client
  m_serverID = serverid;
  m_disk_read_rate = std::stof (NODEINFOS[node_index].disk_read_rate);
  m_disk_write_rate = std::stof (NODEINFOS[node_index].disk_write_rate);
  m_disk_read_latency = std::stof (NODEINFOS[node_index].disk_read_latency);
  m_disk_write_latency = std::stof (NODEINFOS[node_index].disk_write_latency);
}

/* static */
TypeId
ServerApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ServerApp")
                          .SetParent<Application> ()
                          .SetGroupName ("CephSim")
                          .AddConstructor<ServerApp> ();
  return tid;
}

void
ServerApp::StartApplication (void) // 启动应用
{
#ifdef APP_RUN_LOG
  cout << Simulator::Now ().GetSeconds () << "s ServerApp " << m_serverID << " Start" << endl;
  printf ("Check server %d performance: read rate %fMS/s read latency %fms write rate %fMB/s write "
          "latency %fms\n",
          m_serverID, m_disk_read_rate, m_disk_read_latency, m_disk_write_rate,
          m_disk_write_latency);
// cout<<"Check server "<<m_serverID<<" address:"<<m_pnode->GetDevice(0)->GetAddress()<<endl;
#endif

  m_pnode = GetNode ();
  m_disk_is_available = true;
  m_dispatch_lock = true;

  //Receiver socket on server node
  m_UdpSocket = Socket::CreateSocket (m_pnode, UdpSocketFactory::GetTypeId ());

  InetSocketAddress local = InetSocketAddress (InetSocketAddress (
      Ipv4Address::ConvertFrom (NODEINFOS[node_index].address), RECEIVEPORT)); //作为接收端的地址

  //连接远端地址 绑定本地地址
  if (Ipv4Address::IsMatchingType (m_rootPeer) == true)
    {
      if (m_UdpSocket->Bind (local) == -1)
        {
          cout << m_serverID << " server send UdpSocket bing fail!" << endl;
        }
      if (m_UdpSocket->Connect (
              InetSocketAddress (Ipv4Address::ConvertFrom (m_rootPeer), RECEIVEPORT)) == -1)
        {
          cout << m_serverID << " server send UdpSocket connect fail!" << endl;
        }
    }
  else if (InetSocketAddress::IsMatchingType (m_rootPeer) == true)
    {
      if (m_UdpSocket->Bind (local) == -1)
        {
          cout << m_serverID << " server send UdpSocket bing fail!" << endl;
        }
      if (m_UdpSocket->Connect (m_rootPeer) == -1)
        {
          cout << m_serverID << " server send UdpSocket connect fail!" << endl;
        }
    }
  // cout<<m_serverID<<" serverapp socket bound node:"<<m_UdpSocket->GetNode()->GetId()<<endl;

  // m_UdpSocket
  m_UdpSocket->SetRecvCallback (
      MakeCallback (&ServerApp::ReceivePacket, this)); //收到数据包的回调函数

  char *temp = new char[50];
  sprintf (temp, "cephSimResult/serverTrace/server%3d", m_serverID);
  string filepath (temp);
  delete[] temp;
  serverTrace = fopen (filepath.c_str (), "w");
  if (serverTrace == nullptr)
    {
      cout << filepath << "打开失败！" << endl;
    }
}

void
ServerApp::StopApplication (void) // 停止应用
{

  m_running = false; // 运行状态设为false
  if (m_UdpSocket != 0)
    {
      m_UdpSocket->Close ();
      m_UdpSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }

//延迟数据
#ifdef APP_RUN_LOG
  cout << Simulator::Now ().GetSeconds () << "s serverApp " << m_serverID << "延迟数据写入" << endl;
#endif
  latencyStorage::AddServerSendTimes (m_sendTimeMap);
  latencyStorage::AddServerRevTimes (m_revTimeMap);
  unordered_map<uint64_t, uint64_t> uid2serverid;
  for (auto it : m_revTimeMap)
    {
      uid2serverid.insert (unordered_map<uint64_t, uint64_t>::value_type (it.first, m_serverID));
    }
  latencyStorage::AddPacket2ServerID (uid2serverid);
  latencyStorage::AddUidQueueLen (m_uidQueueLen);
  // cout<<"check server rev time"<<endl;
  // for(auto it :m_revTimeMap )
  // {
  //   cout<<it.first<<" "<<it.second<<endl;
  // }
  // cout<<"check server process time"<<endl;
  // for(auto it :m_processTimeMap )
  // {
  //   cout<<it.first<<" "<<it.second<<endl;
  // }
  fclose (serverTrace);
#ifdef APP_RUN_LOG
  cout << Simulator::Now ().GetSeconds () << "s ServerAPP " << m_serverID << " END" << endl;
#endif
}

double
ServerApp::CacuRiskUseLeastSquares (std::vector<double> &Y)
{
  if (Y.size () < 2)
    return -1;
  int len = Y.size ();
  // cout<<len<<endl;
  std::vector<double> X (len);
  for (int i = 0; i < len; i++)
    {
      X[i] = i;
      // cout<<Y[i]<<' ';
    }
  // cout<<endl;

  double sum1 = 0, sum2 = 0;
  double x_avg = std::accumulate (X.begin (), X.end (), 0.0) / len;
  double y_avg = std::accumulate (Y.begin (), Y.end (), 0.0) / len;

  for (int i = 0; i < len; ++i)
    {
      sum1 += (X.at (i) * Y.at (i) - x_avg * y_avg);
      sum2 += (X.at (i) * X.at (i) - x_avg * x_avg);
    }

  double k = sum1 / sum2; //斜率
  double b = y_avg - k * x_avg; //截距

  double p_x =
      X[X.size () - 1] +
      1; //默认计算下一个位置的违例风险 //TODO 计算下一个位置还是计算当pos为0时候的 预测剩余时间 都测一下看看效果

  // cout<<"predict:x = "<<p_x<<" y="<<k*p_x+b<<endl;
  // std::cout << "y=" << k << "x + " << b << std::endl;

  return -(k * p_x + b);
}

void
ServerApp::SendPacket (Ptr<Packet> packet) // 发送数据包
{
  m_sendTimeMap.insert (unordered_map<uint64_t, double>::value_type (
      packet->GetUid (), Simulator::Now ().GetSeconds ())); //存放send时间
  Address tempAdd;

#ifndef DEBUG_LOG_PACKET //不开启LOG
  m_UdpSocket->Send (packet);
#endif

#ifdef DEBUG_LOG_PACKET
  int sendres = m_UdpSocket->Send (packet);
  if (m_UdpSocket->GetPeerName (tempAdd) == 0)
    {
      cout << "Server send packet UID: " << packet->GetUid () << " address:  " << tempAdd
           << " res: " << sendres << endl;
    }
#endif
}

void
ServerApp::ProcessPacket (Ptr<Packet> packet)
{

  //packet 处理过程
  // read the tag from the packet copy
  MyTag tagCopy;
  packet->PeekPacketTag (tagCopy);
  uint32_t tagValue = tagCopy.GetSimpleValue ();

#ifdef DEBUG_LOG_PACKET
  cout << Simulator::Now ().GetSeconds ()
       << "s Server Precess one packet! UID: " << packet->GetUid () << " tag: " << tagValue << endl;
#endif

  m_processTimeMap.insert (unordered_map<uint64_t, double>::value_type (
      packet->GetUid (), Simulator::Now ().GetSeconds ()));
  // packet->PrintPacketTags (cout);  //输出所有tag
  // cout << endl;
  int packeSize = int (packet->GetSize ());
  //通过tag 判断操作类型 tag值为1代表写 包大小即为packet大小 tag值大于1代表读 值即为读的packet大小。

  if (tagValue != 1) //读
    {
      //模拟读操作
      Time opTime = CaculateReadTime (tagValue);
#ifdef DEBUG_LOG_PACKET
      cout << "Read Process size " << tagValue << "B time " << opTime.GetSeconds () << "s" << endl;
#endif
      Simulator::Schedule (opTime, &ServerApp::ReadProcess, this, tagValue, packet->GetUid ());
    }
  else //写
    {
      //模拟写操作
      Time opTime = CaculateWriteTime (packeSize);

#ifdef DEBUG_LOG_PACKET
      cout << "Write Process size " << packeSize << "B time " << opTime.GetSeconds () << "s"
           << endl;
#endif

      Simulator::Schedule (opTime, &ServerApp::WriteProcess, this, tagValue, packet->GetUid ());
    }
}

//用做接收数据包的回调函数
void
ServerApp::ReceivePacket (Ptr<Socket> socket) //收到数据包
{
  Address from;
  Ptr<Packet> packet;
  // Ptr<Packet> packet = socket->Recv ();
  packet = socket->RecvFrom (from);
// cout<<Simulator::Now().GetSeconds()<<"s serverapp receive a packet UID: "<<packet->GetUid() <<" from address:" <<from<<endl;
#ifdef DEBUG_LOG_PACKET
  cout << Simulator::Now ().GetSeconds ()
       << "s serverapp receive a packet UID: " << packet->GetUid () << endl;
#endif
  m_revTimeMap.insert (unordered_map<uint64_t, double>::value_type (
      packet->GetUid (), Simulator::Now ().GetSeconds ())); //存放rev时间

  m_queue_packet.push (packet);
  m_uidQueueLen.insert (unordered_map<uint64_t, uint64_t>::value_type (
      packet->GetUid (), m_queue_packet.size ())); //存储请求入队时候当前队长
  m_leftSLAPercentage[packet->GetUid ()].emplace_back (1.0); //入队时候剩余SLO百分比为100%

  Queue ();
  // ProcessPacket(packet);  //将packet传给serverapp进行处理
}

/// @brief 计算读操作所需要的时间
/// @param size 请求读对象的大小
/// @return 操作所需时间
Time
ServerApp::CaculateReadTime (uint32_t size)
{

  float latency = m_disk_read_latency;
  // 刻画百分之一事件
  int r100 = rand () % 100;
  if (r100 == 0)
    {
      latency += m_disk_read_latency; //延迟翻倍
    }

  // 刻画千分之一事件
  int r1000 = rand () % 1000;
  if (r1000 == 0)
    {
      latency += m_disk_read_latency * 3; //延迟翻倍
    }

  // 刻画万分之一事件
  int r10000 = rand () % 10000;
  if (r10000 == 0)
    {
      latency += m_disk_read_latency * 8; //延迟翻倍
    }

  //输入单位 B 带宽单位 MB/S 返回单位 S
  return Seconds ((size / m_disk_read_rate / 1024 / 1024) + latency / 1000.0);
}

Time
ServerApp::CaculateWriteTime (uint32_t size)
{
  //输入单位 B 带宽单位 MB/S 返回单位 S
  return Seconds ((size / m_disk_write_rate / 1024 / 1024) + m_disk_write_latency / 1000.0);
}

void
ServerApp::ReadProcess (uint32_t tagValue, uint64_t origUID) //读操作
{
  // cout<<"ReadProcess "<<endl;
  Ptr<Packet> returnPacket = Create<Packet> (tagValue); // 返回的包
  returnPacket->RemoveAllByteTags ();
  returnPacket->RemoveAllPacketTags (); //移除原有tag
  MyTag tag;
  tag.SetSimpleValue (tagValue);
  returnPacket->AddPacketTag (tag);

#ifdef DEBUG_LOG_PACKET
  cout << Simulator::Now ().GetSeconds ()
       << "s Server return one packet! UID: " << returnPacket->GetUid () << " tag: " << tagValue
       << endl;
#endif

  latencyStorage::AddRev2Send (origUID, returnPacket->GetUid ()); //存储 接收和发送的id映射关系
  SendPacket (returnPacket);

  // cout<<Simulator::Now().GetSeconds()<<"s UID:"<<origUID<<" end process "<<this<<endl;
  m_leftSLAPercentage.erase (origUID);
  m_disk_is_available = true;
  Queue (); //触发下一个packet处理
}

void
ServerApp::WriteProcess (uint32_t tagValue, uint64_t origUID) //写操作
{
  // cout<<"WriteProcess "<<endl;
  Ptr<Packet> returnPacket = Create<Packet> (1); // 返回的包
  returnPacket->RemoveAllByteTags ();
  returnPacket->RemoveAllPacketTags (); //移除原有tag
  MyTag tag;
  tag.SetSimpleValue (tagValue);
  returnPacket->AddPacketTag (tag);

#ifdef DEBUG_LOG_PACKET
  cout << Simulator::Now ().GetSeconds ()
       << "s Server return one packet! UID: " << returnPacket->GetUid () << " tag: " << tagValue
       << endl;
#endif

  latencyStorage::AddRev2Send (origUID, returnPacket->GetUid ()); //存储 接收和发送的id映射关系
  SendPacket (returnPacket);

  // cout<<Simulator::Now().GetSeconds()<<"s UID:"<<origUID<<" end process "<<this<<endl;
  m_disk_is_available = true;
  Queue (); //触发下一个packet处理
}

void
ServerApp::Queue (void)
{

// 通过调度来选择是否存在SLA即将违例的请求，以及是否调度到队头
#ifdef ServiceCurveDispatch
  if (m_dispatch_lock && m_queue_packet.size () > 0) //不止一个请求在队列中
    {
      // cout<<"UID:"<<m_queue_packet.front()->GetUid()<<endl;
      m_dispatch_lock = false;
      float startTime = clock ();
      vector<RiskPacket> packet_vec; //暂存请求队列中的请求，下标小的是排在前面的 用于计算指标
      uint64_t position = 0; //遍历队列，下标标记下表
      double nowTime = Simulator::Now ().GetSeconds ();
      uint64_t queueSize = m_queue_packet.size ();
      while (position < queueSize) //计数，保证循环遍历一遍队列中的请求
        {
          RiskPacket temp;
          temp.packet_p = m_queue_packet.front ();
          m_queue_packet.pop ();
          temp.pos = position;
          temp.lambda = 1.0 / (position + 1.0); //重要程度等比递减
          bool is_packet_in_priority_queue =
              false; //记录当前packet是否加入到优先队列 如果没有则需要放回原队列
          //计算 risk
          double duration, slo;
          if (m_revTimeMap.find (temp.packet_p->GetUid ()) !=
              m_revTimeMap.end ()) //找到packet接收时间
            {
              duration =
                  (nowTime - m_revTimeMap.find (temp.packet_p->GetUid ())->second); //已经过去的时间
              if (latencyStorage::GetSLA (temp.packet_p->GetUid ()) > 0) //找到sla信息
                {
                  slo = latencyStorage::GetSLA (temp.packet_p->GetUid ());
                  temp.leftSLA = slo - duration;
                  double risk = duration / slo;
                  // printf("调度过程中查看risk值:pos:%3d,duration:%5.3fms,leftSLA:%5.3fms,risk:%5.3f\n",temp.pos,duration*1000,temp.leftSLA*1000,temp.risk);
                  fprintf (serverTrace,
                           "根据risk判断是否需要调入优先队列:pos:%3d,duration:%5.3fms,SLO:%5.3fms,"
                           "leftSlO:%5.3fms,risk:%5.3f,uid:%ld\n",
                           temp.pos, duration * 1000, slo * 1000, temp.leftSLA * 1000, risk,
                           temp.packet_p->GetUid ());
                  fprintf (serverTrace, "优先队列的队长:%3ld\n", m_priority_queue.size ());
                  if (risk >
                      SLAthreshold) // 超过sla阈值 的加入 set<RiskPacket> sla_risk_packet_set 中存放
                    {

                      //计算cost
                      double costMax = COSTMAX; //cost 阈值
                      double N, alpha, total;
                      N = temp.pos;
                      alpha = 0.5;
                      total = packet_vec.size ();
                      double Sj_sum = 0; // 计算公式右边的分母
                      for (int i = 0; i < N; i++)
                        {
                          Sj_sum += (packet_vec[i].leftSLA);
                        }
                      // 计算公式右边的分子
                      double molecule = 0;
                      for (int i = 0; i < N; i++)
                        {
                          molecule += (packet_vec[i].lambda * (Sj_sum - packet_vec[i].leftSLA));
                        }
                      double cost = alpha * N / total + (1 - alpha) * molecule / Sj_sum; //cost 值
                      if (temp.pos == 0) //排在第一个 则cost为0
                        {
                          cost = 0;
                        }
                      temp.cost = cost;

                      // printf("调度过程中查看cost值:pos:%3d,cost:%5.3f\n",riskPacket.pos,riskPacket.cost);
                      fprintf (serverTrace, "调度过程中查看cost值:pos:%3d,cost:%5.3f\n", temp.pos,
                               temp.cost);

                      //判断cost 是否超过阈值
                      if (cost < costMax) //符合 阈值 则选出来，放到优先队列
                        {
                          m_priority_queue.push (temp.packet_p);
                          is_packet_in_priority_queue = true;
                        }
                    }
                }
              else
                {
                  cout << "SLO not found UID:" << temp.packet_p->GetUid () << endl;
                  temp.leftSLA = -1;
                }
            }
          else
            {
              cout << "packet rev time not fount,UID:" << temp.packet_p->GetUid () << endl;
              temp.leftSLA = -1;
            }

          if (!is_packet_in_priority_queue) //请求没有加入优先队列，则放回原队列
            {
              m_queue_packet.push (temp.packet_p);
            }
          packet_vec.push_back (temp);
          position++;
        }

      fprintf (serverTrace, "The time of dispatch is:%5.3fms\n\n",
               (clock () - startTime) / CLOCKS_PER_SEC * 1000.0);
      m_dispatch_lock = true;
    } // endif  不止一个请求在队列中

  //选择请求运行，优先选择优先队列中的
  if (!m_priority_queue.empty ()) //优先队列内非空 优先运行
    {
      if (m_disk_is_available == true)
        {
          m_disk_is_available = false; //开始处理 独占disk

          Ptr<Packet> packet = m_priority_queue.front ();
          m_priority_queue.pop ();

          // cout<<Simulator::Now().GetSeconds()<<"s UID:"<<packet->GetUid()<<" start process "<<this<<endl;
          ProcessPacket (packet);
          // cout<<Simulator::Now().GetSeconds()<<"s UID:"<<packet->GetUid()<<" end process "<<this<<endl;
          // m_disk_is_available = true;   //不能在此处设置 无效
          // 结束之后取出下一个需要处理的
          Queue ();
        }
    }
  else if (!m_queue_packet.empty ()) //运行非优先队列
    {
      if (m_disk_is_available == true)
        {
          m_disk_is_available = false; //开始处理 独占disk

          Ptr<Packet> packet = m_queue_packet.front ();
          m_queue_packet.pop ();

          // cout<<Simulator::Now().GetSeconds()<<"s UID:"<<packet->GetUid()<<" start process "<<this<<endl;
          ProcessPacket (packet);
          // cout<<Simulator::Now().GetSeconds()<<"s UID:"<<packet->GetUid()<<" end process "<<this<<endl;
          // m_disk_is_available = true;   //不能在此处设置 无效
          // 结束之后取出下一个需要处理的
          Queue ();
        }
    }
  else //当前无请求
    {
      return;
    }
#endif

#ifdef OurWorkDispatch
  if (m_disk_is_available && m_queue_packet.size () > 0) //有不止一个请求在队列中
    {

      m_disk_is_available = false; //开始处理 独占disk

      // cout<<"UID:"<<m_queue_packet.front()->GetUid()<<endl;
      float startTime = clock ();
      vector<RiskPacket> packet_vec; //暂存请求队列中的请求，下标小的是排在前面的
      set<RiskPacket>
          sla_risk_packet_set; //存在风险的packet ，RiskPacket重载了 < 运算符，但实现的按Risk从大到小排序
      uint64_t position = 0; //遍历队列，下标标记下表
      double nowTime = Simulator::Now ().GetSeconds ();
      while (!m_queue_packet.empty ()) //queue不方便操作，先取出来用vector操作再放回
        {
          RiskPacket temp;
          temp.packet_p = m_queue_packet.front ();
          temp.pos = position;
          temp.lambda = 1.0 / (position + 1.0); //重要程度等比递减
          //计算 risk
          double duration, slo;
          if (m_revTimeMap.find (temp.packet_p->GetUid ()) !=
              m_revTimeMap.end ()) //找到packet接收时间
            {
              duration =
                  (nowTime - m_revTimeMap.find (temp.packet_p->GetUid ())->second); //已经过去的时间
              if (latencyStorage::GetSLA (temp.packet_p->GetUid ()) > 0) //找到sla信息
                {
                  slo = latencyStorage::GetSLA (temp.packet_p->GetUid ());
                  temp.leftSLA = slo - duration;
                  m_leftSLAPercentage[temp.packet_p->GetUid ()].emplace_back (
                      (temp.leftSLA / slo)); //添加剩余slo比例
                  double risk =
                      CacuRiskUseLeastSquares (m_leftSLAPercentage[temp.packet_p->GetUid ()]);
                  fprintf (serverTrace, "risk:%5.3f,uid:%ld leftSLAPercentage: ", risk,
                           temp.packet_p->GetUid ());
                  for (auto t : m_leftSLAPercentage[temp.packet_p->GetUid ()])
                    {
                      fprintf (serverTrace, "%5.3f ", t);
                    }
                  fprintf (serverTrace, "\n");
                  temp.risk = risk;
                  if (risk > 0)
                    {
                      sla_risk_packet_set.insert (temp);
                    }
                }
              else
                {
                  cout << "SLO not found UID:" << temp.packet_p->GetUid () << endl;
                  temp.leftSLA = -1;
                  temp.risk = -1;
                }
            }
          else
            {
              cout << "packet rev time not fount,UID:" << temp.packet_p->GetUid () << endl;
              temp.leftSLA = -1;
              temp.risk = -1;
            }

          packet_vec.push_back (temp);
          // printf("调度过程中查看risk值:pos:%3d,duration:%5.3fms,leftSLA:%5.3fms,risk:%5.3f\n",temp.pos,duration*1000,temp.leftSLA*1000,temp.risk);
          fprintf (serverTrace,
                   "根据risk判断是否需要调度:pos:%3d,duration:%5.3fms,SLO:%5.3fms,leftSlO:%5.3fms,"
                   "risk:%5.3f,uid:%ld\n",
                   temp.pos, duration * 1000, slo * 1000, temp.leftSLA * 1000, temp.risk,
                   temp.packet_p->GetUid ());
          m_queue_packet.pop ();
          position++;
        }

      double costMax = COSTMAX; //cost 阈值
      int64_t choose = -1; //选中的调度请求的下标 没有候选则为-1
      for (auto it = sla_risk_packet_set.begin (); it != sla_risk_packet_set.end ();
           it++) //从最大risk的开始遍历
        {
          RiskPacket riskPacket = *(it);
          if (riskPacket.pos == 0) //最紧急的现在就在队头，则不需要调度
            {
              break;
            }
          // 计算cost 以下参数定义见 cost计算公式
          double N, alpha, total;
          N = riskPacket.pos;
          alpha = 0.5;
          total = packet_vec.size ();
          double Sj_sum = 0; // 计算公式右边的分母
          for (int i = 0; i < N; i++)
            {
              Sj_sum += (packet_vec[i].leftSLA);
            }
          // 计算公式右边的分子
          double molecule = 0;
          for (int i = 0; i < N; i++)
            {
              molecule += (packet_vec[i].lambda * (Sj_sum - packet_vec[i].leftSLA));
            }
          double cost = alpha * N / total + (1 - alpha) * molecule / Sj_sum; //cost 值
          riskPacket.cost = cost;

          // printf("调度过程中查看cost值:pos:%3d,cost:%5.3f\n",riskPacket.pos,riskPacket.cost);
          fprintf (serverTrace, "调度过程中查看cost值:pos:%3d,cost:%5.3f\n", riskPacket.pos,
                   riskPacket.cost);
          //判断cost 是否超过阈值
          if (cost < costMax) //符合 阈值 则选出来，放到调度队头
            {
              m_queue_packet.push (riskPacket.packet_p);
              choose = riskPacket.pos;
              break;
            }
        }

      //将剩余请求重新放回去
      for (RiskPacket r_pkt : packet_vec)
        {
          if (r_pkt.pos != choose) //剩余的按照原有顺序放回
            {
              m_queue_packet.push (r_pkt.packet_p);
            }
        }
      // cout<<"The time of dispatch is:"<<(clock()-startTime)/ CLOCKS_PER_SEC * 1000.0 <<"ms"<<endl;

      fprintf (serverTrace, "The time of dispatch is:%5.3fms\n\n",
               (clock () - startTime) / CLOCKS_PER_SEC * 1000.0);

      //处理
      ProcessPacket (m_queue_packet.front ());
      m_queue_packet.pop ();
      Queue ();
    }
  else if (m_disk_is_available && m_queue_packet.size () == 1)
    {
      m_disk_is_available = false;
      ProcessPacket (m_queue_packet.front ());
      m_queue_packet.pop ();
      Queue ();
    }
#endif

//没有任何调度策略的时候 默认选队头请求
#ifndef OurWorkDispatch
#ifndef ServiceCurveDispatch

  if (m_disk_is_available && m_queue_packet.size () != 0)
    {
      m_disk_is_available = false;
      ProcessPacket (m_queue_packet.front ());
      m_queue_packet.pop ();
      Queue ();
    }

#endif
#endif
}

//————————————————————————————————【MyServerApp   END】————————————————————————————————

#endif