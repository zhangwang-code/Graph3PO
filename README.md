# Graph3PO

This is the **official implementation** of *Graph3PO*. With *Graph3PO*, developers can generate workloads by adjusting workload parameters. Depending on the specific characteristics of the workload, *Graph3PO* can accurately calculate the execution time for these tasks based on predefined real machine clusters of different scales. This feature provides valuable guidance to developers, helping them determine the optimal cluster size to deploy for various task scenarios. As a result, developers no longer need to overly design complex scheduling policies for different applications in order to ensure tasks are executed in a timely manner. The adaptive task scheduling capabilities of *Graph3PO* eliminate the need for such intricacies.

## Workflow
- Install the Ceph cluster and NS-3 simulator.
- Obtain performance data on Ceph with different workload features.
- Generate 100,000 emulator parameter configurations, and obtain different percentile latencies under default request parameters by randomly running 3,000 groups of configurations.
- Use the *Auto-Sklearn* and *AutoKeras* to match a best simulator parameter configuration.
- Run the simulator and obtain the final experimental results.

## Environment
- CPU: Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz.
- OS: Ubuntu：20.04 Server.
- Linux version: 5.4.0-146-generic (buildd@lcy02-amd64-026).
- gcc version: 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
- NS-3 version: 3.35.
- Workload generator: please refer to the *clientApp.h* in the folder *CephSim*.
- Python libraries: please refer to the *requirements.txt*. We recommend installing different *requirements.txt* files into different python virtual environments.

## Installation

#### Ceph Installation
- Ceph is installed by Cephadm with docker 20.10.7.
- The installation method is available at <https://docs.ceph.com/en/latest/cephadm/>.

#### NS-3 Installation
> Method 1 (version: 3.35)
```
$ git clone https://gitlab.com/nsnam/ns-3-dev.git               
$ cd ns-3-dev                                                   
$ git checkout -b ns-3.35-branch ns-3.35                        
$ bake.py configure -e ns-3.35                                  
$ /waf configure --enable-examples --enable-tests               
$ ./waf build                                                   
$ ./test.py
```

> Method 2
```
$ git clone https://gitlab.com/nsnam/bake                       
$ export BAKE_HOME=`pwd`/bake                                     
$ export PATH=$PATH:$BAKE_HOME                                  
$ export PYTHONPATH=$PYTHONPATH:$BAKE_HOME                      
$ bake.py check                                                 
$ bake.py configure -e ns-3.35                                  
$ bake.py show                                                  
$ bake.py deploy                                                
$ cd source/ns-3.35                                             
$ ./test.py
```

> Method 3 (version: 3.38) 
```
$ git clone https://gitlab.com/nsnam/ns-3-dev.git
$ cd ns-3-dev
$ git checkout -b ns-3.38-release ns-3.38
$ ./ns3 configure --enable-examples --enable-tests
$ ./ns3 build
$ ./test.py
```

## Simulator Running
> 
```
$ mkdir cephSimResult                                          
$ cd cephSimResult                                             
$ mkdir latency                                                
$ mkdir packetTrace                                            
$ mkdir serverTrace                                            
$ cd ..                                                        
$ git clone https://github.com/zhangwang-code/Graph3PO.git     
$ mv Graph3PO/CephSim scratch                                  
$ ./waf --run scratch/CephSim/CephSim
```