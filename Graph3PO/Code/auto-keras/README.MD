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
python 01_autokeras_find_model.py
```
## Key configurations
```shell
# If you aim to find a better and more fitting model, you should consider increasing the values of the following two parameters. 
# These values were used in our experiment, you can replace them in the source code to explore other options.
train_trials = 200
train_epoch = 100
```

___
## Official website introduction
https://autokeras.com/tutorial/structured_data_regression/