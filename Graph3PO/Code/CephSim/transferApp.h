/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    实现 transfer app 对应s3中的 bucket 概念
 */

#ifndef TRANSFERAPP_H
#define TRANSFERAPP_H

#include "cephSimDefine.h" //通用的头文件 namespace define数据等

//————————————————————————————————【TransferApp   Start】————————————————————————————————
/*
    安装在中间节点上的APP 
    功能：
    1 使用rev socket 接收数据包  
    2 使用sender socket 将数据包向下传递
    3 使用ceph的osd选择算法
 */

class TransferApp : public Application
{
public:
  TransferApp (); // 构造函数
  virtual ~TransferApp (); // 析构函数
  static TypeId GetTypeId (void); // 设置TransferApp 的属性系统
  //Setup 函数通过参数值给TransferApp 的私有成员变量赋值
  void Setup (Ptr<Node> install_node, vector<Address> addresses, int id,
              bool is_lastTansApp); //第三个参数表示是否下一层是OSD

private:
  virtual void StartApplication (void); // 启动TransferApp
  virtual void StopApplication (void); // 停止TransferApp

  void ReceivePacket (Ptr<Socket> socket); //收到数据包
  void SendPacket (Ptr<Packet> packet, int nextID); // 返回数据
  void ProcessPacket (Ptr<Packet> packet); //接收包后的处理

  int m_transferID; //app's id
  int nextNodeNum; //后继节点的数量

  bool m_running; // 运行状态
  bool is_lastTansApp; //记录是否是最后一层传输节点 即下一层是否是OSD true 表示 是

  Ptr<Socket> m_revsocket; //rev socket 用于接受信息
  vector<Ptr<Socket>> m_sendSockets; // Socket 指针数组，负责建立socket 连接
  Ptr<Node> m_pnode; //安装app 的节点 用来创建socket

  vector<Address> m_nextNodePeers; // 远端地址数组

  EventId m_sendEvent; // 发送事件

}; //TransferApp声明结束

TransferApp::TransferApp ()
{
}

TransferApp::~TransferApp ()
{
}

//setup 函数，根据参数设置私有成员属性的值
void
TransferApp::Setup (Ptr<Node> install_node, vector<Address> addresses, int id,
                    bool is_last_TransApp)
{
  //赋值
  m_pnode = install_node;
  m_nextNodePeers = addresses;
  m_transferID = id;
  is_lastTansApp = is_last_TransApp;
  nextNodeNum = addresses.size ();
  cout << "TransferAPP " << id << " Setup" << endl;
}

/* static */
TypeId
TransferApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("TransferApp")
                          .SetParent<Application> ()
                          .SetGroupName ("CephSim")
                          .AddConstructor<TransferApp> ();
  return tid;
}

void
TransferApp::StartApplication (void)
{
  cout << Simulator::Now ().GetSeconds () << "s TransferApp " << m_transferID << " Start" << endl;
  //Socket options for IPv4, currently TOS, TTL, RECVTOS, and RECVTT
  bool ipRecvTos = true;
  bool ipRecvTtl = true;

  //Receiver socket on server node
  m_revsocket = Socket::CreateSocket (m_pnode, UdpSocketFactory::GetTypeId ());
  m_revsocket->SetIpRecvTos (ipRecvTos);
  m_revsocket->SetIpRecvTtl (ipRecvTtl);
  m_revsocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), RECEIVEPORT)); //作为接收端的地址
  m_revsocket->SetRecvCallback (MakeCallback (&TransferApp::ReceivePacket, this));

  m_running = true; // 将m_running 设为true ，表明程序为运行状态

  //创建send socket
  for (int i = 0; i < nextNodeNum; ++i)
    {
      Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (m_pnode, UdpSocketFactory::GetTypeId ());
      //Set socket options, it is also possible to set the options after the socket has been created/connected.

      ns3UdpSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), SENDPORT)); //绑定本地
      Address m_peer = m_nextNodePeers[i];
      ns3UdpSocket->Connect (m_peer); //连接发送端
      m_sendSockets.push_back (ns3UdpSocket); //存入socket 资源 vector
    }
}

void
TransferApp::StopApplication (void)
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

  // 暂时不实现transfer层的时间记录
  // cout<<"延迟数据写入 AddClientSendTime"<<endl;
  // latencyStorage::AddClientSendTime(sendTimeMap);
  // latencyStorage::AddClientRevTime(revTimeMap);

  cout << Simulator::Now ().GetSeconds () << "s TransferAPP " << m_transferID << " END" << endl;
}

void
TransferApp::SendPacket (Ptr<Packet> packet, int nextID)
{
  Ptr<Socket> socket = m_sendSockets[nextID];
  socket->Send (packet);
}

//用做接收数据包的回调函数
void
TransferApp::ReceivePacket (Ptr<Socket> socket) //收到数据包
{
  Ptr<Packet> packet = socket->Recv ();
  //   revTimeMap.insert( map<uint64_t,double>::value_type(packet->GetUid(),Simulator::Now().GetSeconds()) ); //存放rev时间
  cout << Simulator::Now ().GetSeconds ()
       << "s transferapp receive a packet UID: " << packet->GetUid () << endl;
  ProcessPacket (packet); //将packet传给ProcessPacket 进行处理
}

//packet 处理过程
void
TransferApp::ProcessPacket (Ptr<Packet> packet)
{
  //寻找 此请求的OBJ 对应的 OSD 获取OSD的编号 目前为了跑通先随机

  int r = rand ();
  int nextNodeid = r % nextNodeNum; // 获得一个随机做为nextNode标号

  //获取到转发的节点号后 进行转发
  SendPacket (packet, nextNodeid);
}

//————————————————————————————————【TransferApp   END】————————————————————————————————
#endif