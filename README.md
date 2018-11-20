# casim
Computer Architecture Simulation Infrastructure for CSCE 614 Computer Architecture



### 1. Environemnt setup

Everytime you want to build or run zsim, you need to setup the environment variables first.

```
$ source setup_env
```



#### 2. Download benchmarks

```
$ cd casim
$ wget http://students.cse.tamu.edu/jyhuang/csce614/casim_bench.tgz
$ tar xzvf casim_bench.tgz
```



#### 3. Setup zsim

##### 3.1 Download config files and runscript

```
$ cd zsim
$ wget http://students.cse.tamu.edu/jyhuang/csce614/configs.tgz
$ wget http://students.cse.tamu.edu/jyhuang/csce614/hw2runscript
$ wget http://students.cse.tamu.edu/jyhuang/csce614/hw4runscript
$ tar xzvf configs.tgz
$ chmod +x hw2runscript hw4runscript
```

##### 3.2 Compile zsim

```
$ scons -j4
```

##### 3.3 Launch a test to run

```
./build/opt/zsim tests/simple.cfg
```



###### For more information, check `zsim/README.md`
