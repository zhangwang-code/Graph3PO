#ifndef CEPHSIMDEFINE_H
#define CEPHSIMDEFINE_H
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    记录项目中的所有
    1 define 
    2 头文件
    3 namespace
    4 一些debug信息显示的宏定义，用来控制条件编译，所以其他文件需要包含这个头文件
    
 */
//————————————————————————————————【头文件】————————————————————————————————
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/vector.h"
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <algorithm>

#include "myTag.h"

//————————————————————————————————【namespace】————————————————————————————————
using namespace ns3;

using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::map;
using std::multimap;
using std::ofstream;
using std::queue;
using std::rand;
using std::set;
using std::sort;
using std::sprintf;
using std::srand;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

//————————————————————————————————【自定义数据】————————————————————————————————
//通用
#define RECEIVEPORT 9 //接收端socket端口号
#define SENDPORT 507 //请求发送端socket的端口号

#define LAYERNUM 3 //拓扑层数
#define HOSTNUM TRANSFERNUM //方便在不同语义下使用宏定义
#define OSDNUM SERVERNUM //方便在不同语义下使用宏定义

static int ALLNODENUM = 0; //所有节点数量
static int TRANSFERNUM = 0; //传输节点数量
static int SERVERNUM = 0; //osd数量

//config.h
#define CLUSTERMAPPATH \
  "scratch/CephSim2022-12-8/cluster_map.csv" // cluster_map.txt 文件路径 10-9版本开始使用抽取的配置信息。

#define RESPATH "cephSimResult"

//client.h
#define RENUM 1600000 //请求数量
#define RESIZE 1024 * 8 //请求平均大小 B
#define RESIZEMAX 65000 //最大大小 B
#define REWAIT 1.0 / 200.0 //请求平均等待时间

#define CLIENTENDTIME \
  RENUM *(REWAIT + 0.1) * 5 + 1 //clientapp停止时间，这样为了保证所有请求结束之前app没有关闭
#define READProportion 100 //读所占的百分比
#define IS_USE_OSD_WAIT_TIME //是否使用osd状态预测来优化请求发送的OSD

//server.h
#define SERVERENDTIME CLIENTENDTIME + 5 //server app 停止时间，比clientapp停止时间慢5s
#define SLAthreshold 0.5 //sla 阈值 即 risk 阈值
// #define ServiceCurveDispatch  // 是否调度进行基于服务曲线的调度 如果不需要调度则注释此行
// #define ControlTheoryDispatch    // 是否调度进行基于控制理论的调度 如果不需要调度则注释此行
#define OurWorkDispatch // 是否调度进行基于我们的工作的调度 如果不需要调度则注释此行

#define COSTMAX 0.5 //设置的cost值

//CephSim.cc
#define RUN_NUM 3 //循环次数
// #define IS_CREATE_CLUSTER_MAP   //是否重新生成拓扑结构文件
// #define IS_CREATE_WORKLOAD  //是否重新生成负载文件

// DEBUG   如果需要显示对应的信息则把注释去掉

// #define DEBUG_LOG_PACKET   //如果显示packet收发中间过程则打开
// #define LATENCY_SHOW   //显示每次运行完的延迟数据
// #define APP_RUN_LOG  //显示app的setup 开始，结束等信息
// #define TOPOLOGY_CREATE_LOG //显示创建拓扑的过程

#endif
