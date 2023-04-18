# Fast Start
## OS
```
Ubuntu：20.04 Server
(Linux version 5.4.0-146-generic (buildd@lcy02-amd64-026) (gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1)))
```
## Python Version
```
3.8.10
```
## Create a virtual environment
```
python3 -m venv venv
```
## Activate venv
```
source venv/bin/activate
 ```
## Installation requirements
```
pip install -r requirements.txt
```
## Run
```
python 01_AutoReg_NS3.py
```
## Note
It should be noted that repeatedly running the code requires deleting the previous tmp file ```(auto_sklearn_tmp/)``` first
## Key configurations
```shell
# If you aim to find a better and more fitting model, you should consider increasing the values of the following two parameters. 
# These values were used in our experiment, you can replace them (line 41, 42) in the source code to explore other options.
time_left_for_this_task = 60 * 60 * 2 
per_run_time_limit = 60 
```
## Result Example
```
          rank  ensemble_weight               type      cost   duration
model_id                                                               
135          1              0.2      random_forest  0.034550  15.880286
196          2              0.4  gradient_boosting  0.035076   2.682198
156          3              0.2      random_forest  0.036138  11.326378
187          4              0.2        extra_trees  0.036545  11.163536
......
auto-sklearn results:
  Dataset name: ns3-sim
  Metric: r2
  Best validation score: 0.965450
  Number of target algorithm runs: 197
  Number of successful target algorithm runs: 152
  Number of crashed target algorithm runs: 2
  Number of target algorithms that exceeded the time limit: 30
  Number of target algorithms that exceeded the memory limit: 13

Train R2 score: 0.9729272250129015
Test R2 score: 0.9644907806953806
```

___
## Official website introduction
https://github.com/automl/auto-sklearn