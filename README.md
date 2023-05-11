# Odd Even Sort  
這是一個basic的odd even sort實作紀錄，使用mpi.h以及在twcc的台灣杉三號上操作

## 一、Information
hackmd: [link](https://hackmd.io/wCMZWHslRuOkZQ6WzBgygA?view)

## 二、Iplementation
#### 編譯前置作業
先進行module load，要注意編譯使用的MPI要和執行使用的是同一個
* module purge
* module load compiler/intel/2022
* module load IntelMPI/2021.6
---
#### odd even sort 程式實作
1. 讀入command line arguement，進行`comm_size`和`comm_rank`的Initialize
2. 用動態配置空間建立count和display陣列，count存取每個processor存放的data數，display存取每個processor prefix累積的data數
3. 每個process使用`mpi_open`和`mpi_read_at`讀取data，並在每個processor中各自qsort各自的data
4. 不同的process根據奇偶階段和自己rank的奇偶找出自己的partner，並用`mpi_send`和`mpi_recv`開始進行processor間的溝通
5. 每個process將自己接收到的data和自己的data進行merge sort，根據和自己partner的相對順序決定data要merge up或merge down
6. 最後在rank0的processor呼叫`mpi_gatherv`將data收集到rank0中，再用`mpi_write_at`寫進輸出txt檔
7. free掉所有allocate的空間，呼叫`mpi_finalize`結束程式

---

#### 想法補充
* `mpi_read_at`可以節省使用`mpi_scatterv`需要的通訊時間
* merge sort只做自己需要的部份，可以節省一半的sorting時間
* 要注意`mpi_send`和`mpi_recv`的順序，因為不同process完成通訊的時間並不一樣
## 三、Experiment & Analysis
### 基本測資
測資測試結果：所有testcases中只有36和26過不了，後來檢查測資發現裡面的數據只有0和-0

而在更先前原本有幾筆測資也過不了，但是這些過不了的測資也沒有龐大的數據量，幸好後來TA開放TA時間到實驗室幫助我們解決問題，發現是因為台灣衫三號的OpenMPI/4.0.5有狀況，最後使用Intel的MPI測資就全過了，只是會有一個奇怪的error message出現：
> MPI startup(): PMI server not found. Please set I_MPI_PMI_LIBRARY variable if it is not a singleton case.

後來發現這個錯誤會導致只有用到一個processor再進行運算，最後只好用mpirun來執行程式，但是因為mpirun的結構和srun不一樣，所以還要將mpirun結合sbatch才能完成正常的程式運行

###### sbatch
```BAT=
#!/bin/bash

#SBATCH -J oddeven.log         # Job Name
#SBATCH -A ACD110018           # Account
#SBATCH -p ctest               # Partition
#SBATCH -o oddeven_out.log     # Redirect `stdout` to File
#SBATCH -e oddeven_err.log     # Redirect `stderr` to File

#SBATCH -n 12
#SBATCH -N 3

module purge
module load compiler/intel/2022
module load IntelMPI/2021.6

export UCX_NET_DEVICES=all

time mpirun ./odd_even_sort 536869888 ./testcases/40.in out.txt
```
`export UCXNETDEVICES=all`是為了修正台灣衫三號的node溝通問題

而最後在多個processor的運算後，剩下除了36以及26的奇怪測資，其他都全部過了

另外，在Makefile中我額外增加了一條`-std=c99`的參數，因為我發現我的for迴圈中不能進行宣告，後來發現是因為C的版本問題
```MAKEFILE=
CXX = mpicc
exe = odd_even_sort
obj = odd_even_sort.c

$(exe): $(obj)
        $(CXX) -std=c99 -o $(exe) $(obj)


.PHONY: clean
clean:
        rm $(exe)
```
---


### 額外測試
以第40筆測資作為測試資料，因為data基數最多
![](https://i.imgur.com/wHfW9bf.png)
> processor對應runtime圖表，包含3、2、1個nodes的資料

由上圖可以輕易觀察出，越多processor的運算越快，但是node數對runtime則沒有巨大的影響
100個processor沒有一個node的資料是因為台灣衫三號中一個node最多只能跑56個processor

![](https://i.imgur.com/OUh6Er6.png)
> 1200個processors的測試，包含50、60個nodes的資料

可以發現雖然使用了1200個processors，儘管資料量仍然遠大於這個數字，可是運算速度卻比使用100個processors慢，甚至跟使用了12、24個processors差不多，可以得出processors的數量並不是越多越好的結論

