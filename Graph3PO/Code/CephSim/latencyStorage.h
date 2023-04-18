/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    均为static  实现延迟数据的全局存储 
 */

#ifndef LATENCYSTORAGE_H
#define LATENCYSTORAGE_H

#include "cephSimDefine.h" //通用的头文件 namespace define数据等
#include "config.h"
#include <unordered_map>

struct PacketTrace
{
  /* data */
  uint64_t clientSendUid, serverSendUid; //packet在client发送的id，server返回的uid，server的id
  int serverID;
  double clientSendTime; //客户端发送时间
  double serverRevTime; //服务端接收时间
  double serverSendTime; //服务端返回消息的时间
  double clientRevTime; //客户端接收返回消息的时间
  uint64_t queueLen; //处理该packet的时候 该osd的队长
  double slo;
};

struct GCNData
{
  int bucket;
  vector<double> latency;
  vector<double> start;
  vector<int> queueL;
};

static string preview_latency = "Arrival_rate,Ave,P50,P90,P99,P999,P9999,sla_default_rate(%),sla_"
                                "default_percentage(%),latencyFile,clusterMapFile\n"; //表头
class latencyStorage
{
private:
  /* data */
  //  unordered_map 内容为： id ：time(Seconds)
  static int m_serverNum; //存server数量 用于vectort初始化
  static unordered_map<uint64_t, double> m_clientSendTime; //存client 发送数据包的时间
  static unordered_map<uint64_t, double> m_clientRevTime; //存client 收到数据包的时间
  static unordered_map<uint64_t, uint64_t>
      m_packet2serverid; //存server收到一个packet 其对应的返回的packet的id
  static unordered_map<uint64_t, double> m_serverSendTimes; //存server 发送数据包的时间  多个server
  static unordered_map<uint64_t, double> m_serverRevTimes; //存server 接收数据包的时间
  static unordered_map<uint64_t, uint64_t>
      m_rev2send; //记录server接收到一个packet 后 发送返回的packet 的id对照
  static unordered_map<uint64_t, uint64_t> m_uidQueueLen; //存处理该packet的时候 该osd目前的队长
  static unordered_map<uint64_t, double> m_uidSLA; //存sla 单位 秒

public:
  latencyStorage (/* args */);
  ~latencyStorage ();

  // 记录数据
  static void AddClientSendTime (unordered_map<uint64_t, double> m);
  static void AddClientRevTime (unordered_map<uint64_t, double> m);
  static void AddServerSendTimes (unordered_map<uint64_t, double> m);
  static void AddServerRevTimes (unordered_map<uint64_t, double> m);
  static void AddRev2Send (uint64_t revid, uint64_t sendid);
  static void AddPacket2ServerID (unordered_map<uint64_t, uint64_t>);
  static void AddUidQueueLen (unordered_map<uint64_t, uint64_t> m);
  static void AddSLA (uint64_t uid, double slo);

  // 获取数据
  static unordered_map<uint64_t, double> GetClientSendTime (void);
  static unordered_map<uint64_t, double> GetClientRevTime (void);
  static unordered_map<uint64_t, double> GetServerSendTimes (void);
  static unordered_map<uint64_t, double> GetServerRevTimes (void);
  static double GetSLA (uint64_t uid); //找到对应uid的SLA 单位ms，没找到返回-1

  //初始化函数
  static unordered_map<uint64_t, double> CreateMapU64D (void);
  static vector<unordered_map<uint64_t, double>> CreateVec (void);
  static unordered_map<uint64_t, uint64_t> CreateMapU64U64 (void);

  //显示结果 用于测试
  static void TestData (void); //用于运行前的数据检查
  static void Show (void); //用于运行后的数据显示

  //计算延迟
  static void WriteTraceToFile (string latencyFilePath, string packetTracePath,
                                string clusterMapFile); //写trace文件
  static void WritePreViewToFile (string path);

  //清除数据
  static void CleanData (void);
};

//latencyStorage 静态成员 初始化
int latencyStorage::m_serverNum = SERVERNUM;
unordered_map<uint64_t, double> latencyStorage::m_clientSendTime = latencyStorage::CreateMapU64D ();
unordered_map<uint64_t, uint64_t> latencyStorage::m_rev2send = latencyStorage::CreateMapU64U64 ();
unordered_map<uint64_t, double> latencyStorage::m_clientRevTime = latencyStorage::CreateMapU64D ();
unordered_map<uint64_t, double> latencyStorage::m_serverSendTimes =
    latencyStorage::CreateMapU64D ();
unordered_map<uint64_t, double> latencyStorage::m_serverRevTimes = latencyStorage::CreateMapU64D ();
unordered_map<uint64_t, uint64_t> latencyStorage::m_packet2serverid =
    latencyStorage::CreateMapU64U64 ();
unordered_map<uint64_t, uint64_t> latencyStorage::m_uidQueueLen =
    latencyStorage::CreateMapU64U64 ();
unordered_map<uint64_t, double> latencyStorage::m_uidSLA = latencyStorage::CreateMapU64D ();

latencyStorage::latencyStorage (/* args */)
{
}

latencyStorage::~latencyStorage ()
{
  m_serverRevTimes.clear ();
  m_serverSendTimes.clear ();
  m_clientRevTime.clear ();
  m_clientSendTime.clear ();
  m_rev2send.clear ();
  m_packet2serverid.clear ();
  m_uidQueueLen.clear ();
  m_uidSLA.clear ();
}

void
latencyStorage::AddClientRevTime (unordered_map<uint64_t, double> m)
{
  // m_clientRevTime.insert(m.begin(),m.end());
  for (auto it : m)
    {
      m_clientRevTime.insert (unordered_map<uint64_t, double>::value_type (it.first, it.second));
    }
}

void
latencyStorage::AddClientSendTime (unordered_map<uint64_t, double> m)
{
  // m_clientSendTime.insert(m.begin(),m.end());
  for (auto it : m)
    {
      m_clientSendTime.insert (unordered_map<uint64_t, double>::value_type (it.first, it.second));
    }
}

void
latencyStorage::AddServerRevTimes (unordered_map<uint64_t, double> m)
{
  // m_serverRevTimes.insert(m.begin(),m.end());
  for (auto it : m)
    {
      m_serverRevTimes.insert (unordered_map<uint64_t, double>::value_type (it.first, it.second));
    }
}

void
latencyStorage::AddServerSendTimes (unordered_map<uint64_t, double> m)
{
  // m_serverSendTimes.insert(m.begin(),m.end());
  for (auto it : m)
    {
      m_serverSendTimes.insert (unordered_map<uint64_t, double>::value_type (it.first, it.second));
    }
}

void
latencyStorage::AddPacket2ServerID (unordered_map<uint64_t, uint64_t> m)
{
  // m_packet2serverid.insert(m.begin(),m.end());
  for (auto it : m)
    {
      m_packet2serverid.insert (
          unordered_map<uint64_t, uint64_t>::value_type (it.first, it.second));
    }
}

void
latencyStorage::AddRev2Send (uint64_t revid, uint64_t sendid)
{
  m_rev2send.insert (unordered_map<uint64_t, uint64_t>::value_type (revid, sendid));
}

void
latencyStorage::AddUidQueueLen (unordered_map<uint64_t, uint64_t> m)
{
  for (auto it : m)
    {
      m_uidQueueLen.insert (unordered_map<uint64_t, double>::value_type (it.first, it.second));
    }
}

void
latencyStorage::AddSLA (uint64_t uid, double slo)
{
  m_uidSLA.insert (unordered_map<uint64_t, double>::value_type (uid, slo));
}

unordered_map<uint64_t, double>
latencyStorage::CreateMapU64D (void)
{
  unordered_map<uint64_t, double> tempMap;
  tempMap.insert (unordered_map<uint64_t, double>::value_type (uint64_t (0), double (0)));
  tempMap.erase (tempMap.begin (), tempMap.end ());
  return tempMap;
}

unordered_map<uint64_t, uint64_t>
latencyStorage::CreateMapU64U64 (void)
{
  unordered_map<uint64_t, uint64_t> tempMap;
  tempMap.insert (unordered_map<uint64_t, uint64_t>::value_type (uint64_t (0), uint64_t (0)));
  tempMap.erase (tempMap.begin (), tempMap.end ());
  return tempMap;
}

vector<unordered_map<uint64_t, double>>
latencyStorage::CreateVec (void)
{
  unordered_map<uint64_t, double> tempMap = latencyStorage::CreateMapU64D ();
  vector<unordered_map<uint64_t, double>> tempVec (m_serverNum, tempMap);
  return tempVec;
}

unordered_map<uint64_t, double>
latencyStorage::GetClientSendTime (void)
{
  return m_clientSendTime;
}

unordered_map<uint64_t, double>
latencyStorage::GetClientRevTime (void)
{
  return m_clientRevTime;
}

unordered_map<uint64_t, double>
latencyStorage::GetServerSendTimes (void)
{
  return m_serverSendTimes;
}

unordered_map<uint64_t, double>
latencyStorage::GetServerRevTimes (void)
{
  return m_serverRevTimes;
}

//找到对应uid的SLA,单位ms,没找到返回-1
double
latencyStorage::GetSLA (uint64_t uid)
{
  if (m_uidSLA.find (uid) != m_uidSLA.end ())
    {
      return m_uidSLA.find (uid)->second;
    }
  return -1; //没找到返回-1
}

void
latencyStorage::TestData (void)
{
  //Test latencyStorage
  cout << "Test latencyStorage::m_clientSendTime" << endl;
  for (const auto &[key, value] : m_clientSendTime)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Test latencyStorage::m_serverRevTimes" << endl;
  for (const auto &[key, value] : m_serverRevTimes)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Test latencyStorage::m_rev2send" << endl;
  for (const auto &[key, value] : m_rev2send)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Test latencyStorage::m_serverSendTimes" << endl;
  for (const auto &[key, value] : m_serverSendTimes)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Test latencyStorage::m_clientRevTime" << endl;
  for (const auto &[key, value] : m_clientRevTime)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
}

void
latencyStorage::Show (void)
{
  //Show latencyStorage
  cout << "Show latencyStorage::m_clientSendTime" << endl;
  for (const auto &[key, value] : m_clientSendTime)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Show latencyStorage::m_serverRevTimes" << endl;
  for (const auto &[key, value] : m_serverRevTimes)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Show latencyStorage::m_rev2send" << endl;
  for (const auto &[key, value] : m_rev2send)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Show latencyStorage::m_serverSendTimes" << endl;
  for (const auto &[key, value] : m_serverSendTimes)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
  cout << "Show latencyStorage::m_clientRevTime" << endl;
  for (const auto &[key, value] : m_clientRevTime)
    {
      //key
      cout << key << " " << value << endl;
      //value
    }
}

void
latencyStorage::WriteTraceToFile (
    string latencyFilePath = "cephSimResult/latency.csv",
    string packetTracePath = "cephSimResult/packettrace.csv",
    string clusterMapFile = "cephSimResult/cluster_map_copy/000_map.csv")
{

  vector<PacketTrace> pakTrace_vec;
  for (auto it = m_clientSendTime.begin (); it != m_clientSendTime.end ();
       ++it) //循环遍历，写入一个packet的trace信息
    {
      PacketTrace pakTrace;
      pakTrace.clientSendUid = it->first;
      pakTrace.clientSendTime = it->second;

      if (m_uidQueueLen.find (pakTrace.clientSendUid) != m_uidQueueLen.end ()) //寻找队长信息
        {
          pakTrace.queueLen = m_uidQueueLen.find (pakTrace.clientSendUid)->second;
        }
      else
        {
          cout << "packet server queue lenth not found, client send Uid:" << pakTrace.clientSendUid
               << endl;
        }

      if (m_uidSLA.find (pakTrace.clientSendUid) != m_uidSLA.end ())
        {
          pakTrace.slo = m_uidSLA.find (pakTrace.clientSendUid)->second;
        }
      else
        {
          cout << "packet SLO not found, client send Uid:" << pakTrace.clientSendUid << endl;
        }

      if (m_serverRevTimes.find (pakTrace.clientSendUid) != m_serverRevTimes.end ()) //寻找接收时间
        {
          pakTrace.serverRevTime = m_serverRevTimes.find (pakTrace.clientSendUid)->second;
        }
      else
        {
          cout << "packet server rev time not found, client send Uid:" << pakTrace.clientSendUid
               << endl;
          continue;
        }

      if (m_packet2serverid.find (pakTrace.clientSendUid) !=
          m_packet2serverid.end ()) //寻找请求被哪个server处理了
        {
          pakTrace.serverID = m_packet2serverid.find (pakTrace.clientSendUid)->second;
        }
      else
        {
          cout << "packet server id not found, client send Uid:" << pakTrace.clientSendUid << endl;
          continue;
        }

      if (m_rev2send.find (pakTrace.clientSendUid) != m_rev2send.end ()) //寻找返回的UID
        {
          pakTrace.serverSendUid = m_rev2send.find (pakTrace.clientSendUid)->second;
        }
      else
        {
          cout << "server return packet's Uid not fount,  client send Uid:"
               << pakTrace.clientSendUid << endl;
          continue;
        }

      if (m_serverSendTimes.find (pakTrace.serverSendUid) !=
          m_serverSendTimes.end ()) //寻找返回时间
        {
          pakTrace.serverSendTime = m_serverSendTimes.find (pakTrace.serverSendUid)->second;
        }
      else
        {
          cout << "server return packet's time not fount, server send Uid:"
               << pakTrace.serverSendUid << endl;
          continue;
        }

      if (m_clientRevTime.find (pakTrace.serverSendUid) !=
          m_clientRevTime.end ()) //寻找client 接收时间
        {
          pakTrace.clientRevTime = m_clientRevTime.find (pakTrace.serverSendUid)->second;
        }
      else
        {
          cout << "client rev  time not fount, server send Uid:" << pakTrace.serverSendUid << endl;
          continue;
        }

      pakTrace_vec.push_back (pakTrace);
    }

  //计算尾延迟指标 sla违约率 sla违约百分比
  double sumlatency = 0; //总延迟 毫秒
  double max_sendtime = 0; //用于计算到达率
  double min_sendtime = 9999;
  uint64_t sla_false_count = 0; //sla违约个数
  double sla_rate; // sla违约率
  vector<double> all_latency;
  double sum_sla_false_rate = 0; //总违约百分比
  double ave_sla_false_rate; //平均违约百分比
  //计算 sla违约率 sla违约百分比
  for (auto paktrace : pakTrace_vec)
    {
      sumlatency += (paktrace.clientRevTime - paktrace.clientSendTime) * 1000.0;
      all_latency.push_back ((paktrace.clientRevTime - paktrace.clientSendTime) * 1000.0);
      if (paktrace.clientSendTime > max_sendtime)
        {
          max_sendtime = paktrace.clientSendTime;
        }
      if (paktrace.clientSendTime < min_sendtime)
        {
          min_sendtime = paktrace.clientSendTime;
        }
      if ((paktrace.clientRevTime - paktrace.clientSendTime) > paktrace.slo)
        {
          sla_false_count++;
          sum_sla_false_rate +=
              (((paktrace.clientRevTime - paktrace.clientSendTime) / paktrace.slo - 1.0) *
               100); //总违约百分比的和
        }
    }
  sla_rate = double (sla_false_count) / pakTrace_vec.size () * 100;
  ave_sla_false_rate = sla_false_count == 0 ? 0 : (sum_sla_false_rate / sla_false_count);
  double arrival_rate = pakTrace_vec.size () / (max_sendtime - min_sendtime); //平均到达率
  double avelatency = sumlatency / pakTrace_vec.size ();
  sort (all_latency.begin (), all_latency.end ());
  //计算尾延迟指标
  double p50, p90, p99, p999, p9999;
  p50 = all_latency[int (all_latency.size () * 0.5)];
  p90 = all_latency[int (all_latency.size () * 0.9)];
  p99 = all_latency[int (all_latency.size () * 0.99)];
  p999 = all_latency[int (all_latency.size () * 0.999)];
  p9999 = all_latency[int (all_latency.size () * 0.9999)];
  char *buf = new char[150];
  sprintf (buf, " Ave:%7.4f P50:%7.4f P90:%7.4f P99:%7.4f P999:%7.4f P9999:%7.4f", avelatency, p50,
           p90, p99, p999, p9999);
  char *latency_buf = new char[200];
  sprintf (latency_buf, "%7.4f,%7.4f,%7.4f,%7.4f,%7.4f,%7.4f,%7.4f,%7.4f,%7.4f,%s,%s\n",
           arrival_rate, avelatency, p50, p90, p99, p999, p9999, sla_rate, ave_sla_false_rate,
           latencyFilePath.c_str (), clusterMapFile.c_str ());
  preview_latency += string (latency_buf);

  ofstream tracefile;
  tracefile.open (packetTracePath);

  if (!tracefile.is_open ()) //打开失败
    {
      cout << packetTracePath << "文件打开失败" << endl;
    }
  tracefile << "packet_id,server_id,client_send_time(s),server_Rev_Time(s),server_Send_Time(s),"
               "client_rev_time(s),latency(ms),queue_len"
            << endl;
  for (auto paktrace : pakTrace_vec)
    {
      tracefile << paktrace.clientSendUid << "," << paktrace.serverID << ","
                << paktrace.clientSendTime << "," << paktrace.serverRevTime << ","
                << paktrace.serverSendTime << "," << paktrace.clientRevTime << ","
                << (paktrace.clientRevTime - paktrace.clientSendTime) * 1000.0 << ","
                << paktrace.queueLen << endl;
    }
  cout << packetTracePath << "结果写入成功" << endl;
  tracefile.close ();

  ofstream latencyfile;
  latencyfile.open (latencyFilePath);
  if (!latencyfile.is_open ()) //打开失败
    {
      cout << latencyFilePath << "文件打开失败" << endl;
    }
  latencyfile << "Sim latency at ReqAveWait:" << REWAIT << " ReqSize:" << RESIZE
              << "B SampleNum:" << RENUM << buf << endl;
  latencyfile << "startTime,latency" << endl;
  for (auto paktrace : pakTrace_vec)
    {
      latencyfile << paktrace.clientSendTime << ","
                  << (paktrace.clientRevTime - paktrace.clientSendTime) * 1000.0 << endl;
    }
  cout << latencyFilePath << "结果写入成功" << endl;
  latencyfile.close ();

  //写用于GCN的数据
  //adj 邻接矩阵
  ofstream gcnadjfile;
  string gcnadjpath = "cephSimResult/adj.csv";
  gcnadjfile.open (gcnadjpath);
  if (!gcnadjfile.is_open ()) //打开失败
    {
      cout << gcnadjpath << "文件打开失败" << endl;
    }
  vector<vector<double>> adj (ALLNODENUM, vector<double> (ALLNODENUM, 0));
  for (auto node : NODEINFOS)
    {
      if (node.nodeID == 0) // client - host
        {
          adj[0][0] = 0;
          for (auto next : node.items)
            {
              adj[0][next] = 0.5;
            }
        }
      else if (node.nodeID < ALLNODENUM - OSDNUM) //host - osd
        {
          adj[node.nodeID][node.nodeID] = 0;
          for (auto next : node.items)
            {
              adj[node.nodeID][next] = 1;
            }
        }
      else // osd
        {
          adj[node.nodeID][node.nodeID] = 1;
        }
    }
  //显示 adj
  // for(auto x:adj)
  // {
  //     for(uint32_t i = 0;i<x.size();i++)
  //     {
  //         auto y = x[i];
  //         cout<<y<<' ';
  //         gcnadjfile<<y;
  //         if(i!=x.size()-1)
  //         {
  //             gcnadjfile<<',';
  //         }
  //     }
  //     cout<<endl;
  //     gcnadjfile<<endl;
  // }
  gcnadjfile.close ();
  cout << gcnadjpath << "结果写入成功" << endl;

  //特征矩阵
  ofstream gcnfile;
  string gcnpath = "cephSimResult/waitTime.csv";
  gcnfile.open (gcnpath);
  if (!gcnfile.is_open ()) //打开失败
    {
      cout << gcnpath << "文件打开失败" << endl;
    }
  for (int i = 0; i < ALLNODENUM; i++)
    {
      gcnfile << i;
      if (i != ALLNODENUM - 1)
        {
          gcnfile << ",";
        }
      else
        {
          gcnfile << endl;
        }
    }

  vector<GCNData> gcnData_v;
  for (int i = 0; i < ALLNODENUM; i++)
    {
      GCNData temp;
      temp.bucket = i;
      gcnData_v.push_back (temp);
    }
  double maxTime = 0;
  for (auto paktrace : pakTrace_vec)
    {
      int bukID = paktrace.serverID + (ALLNODENUM - OSDNUM);
      gcnData_v[bukID].latency.push_back ((paktrace.clientRevTime - paktrace.clientSendTime) *
                                          1000.0);
      gcnData_v[bukID].queueL.push_back (paktrace.queueLen);
      gcnData_v[bukID].start.push_back (paktrace.serverRevTime);
      if (maxTime < paktrace.serverRevTime)
        {
          maxTime = paktrace.serverRevTime;
        }
    }

  double startWindow = 1.0; //第一个窗口从1开始
  double windowLen = 1.0;

  while (startWindow < maxTime - 1)
    {
      string temp = "";
      for (int bukID = 0; bukID < ALLNODENUM; bukID++)
        {
          if (bukID < ALLNODENUM - OSDNUM)
            {
              temp += (to_string (0) + ",");
            }
          else
            {
              double sumlatency = 0;
              int sunQueueLen = 0;
              int sumReq = 0;

              for (uint64_t i = 0; i < gcnData_v[bukID].start.size (); i++)
                {
                  if (gcnData_v[bukID].start[i] >= startWindow &&
                      gcnData_v[bukID].start[i] < (startWindow + windowLen))
                    {
                      sumReq++;
                      sunQueueLen += gcnData_v[bukID].queueL[i];
                      sumlatency += gcnData_v[bukID].latency[i];
                    }
                  if (gcnData_v[bukID].start[i] > (startWindow + windowLen))
                    {
                      break;
                    }
                }
              if (sumReq)
                {
                  temp += (to_string ((sumlatency / sumReq) * (sunQueueLen / sumReq)));
                }
              else
                {
                  temp += to_string (0);
                }
              if (bukID != ALLNODENUM - 1)
                {
                  temp += ",";
                }
              else
                {
                  temp += "\n";
                }
            }
        }
      gcnfile << temp;
      startWindow += windowLen;
    }
  cout << gcnpath << "结果写入成功" << endl;
  gcnfile.close ();

  delete[] buf;
  delete[] latency_buf;
}

void
latencyStorage::WritePreViewToFile (string path = "cephSimResult/01_latencyPreview.csv")
{
  ofstream file;
  file.open (path);
  if (!file.is_open ())
    {
      cout << path << "文件打开失败" << endl;
    }
  file << preview_latency;
  cout << path << "结果写入成功" << endl;
  file.close ();
}

void
latencyStorage::CleanData (void)
{
#ifdef LATENCY_SHOW
  cout << "start latencyStorage::CleanData:" << m_clientRevTime.size () << " "
       << m_clientSendTime.size () << " " << m_rev2send.size () << " " << m_serverRevTimes.size ()
       << " " << m_serverSendTimes.size () << " " << m_packet2serverid.size () << endl;
#endif

  m_clientRevTime.clear ();
  m_clientSendTime.clear ();
  m_rev2send.clear ();
  m_serverRevTimes.clear ();
  // m_serverRevTimes = latencyStorage::CreateMapU64D();
  m_serverSendTimes.clear ();
  // m_serverSendTimes = latencyStorage::CreateMapU64D();
  m_packet2serverid.clear ();

#ifdef LATENCY_SHOW
  cout << "end latencyStorage::CleanData:" << m_clientRevTime.size () << " "
       << m_clientSendTime.size () << " " << m_rev2send.size () << " " << m_serverRevTimes.size ()
       << " " << m_serverSendTimes.size () << " " << m_packet2serverid.size () << endl;
#endif
}

#endif