
# Table of contents



- <a href=#setup>Set up</a>

  - <a href=#job_scripts>Job control scripts</a>

  - <a href=#wrapper_scripts>Wrapper scripts</a>
      - <a href=#mps_wrapper>mps-wrapper.sh</a>
      - <a href=#map_gpus>map-gpus.sh</a>
      


- <a href=#run_womps>Run Gamess without MPS</a>

  - <a href=#submit_job>Submit job</a>

  - <a href=#wait_job>Wait for the jobs to start</a>

  - <a href=#nohang>Check that Gamess does not hang</a>
     - <a href=#short_job>JS=23947569</a>
     - <a href=#long_job>JL=23941996</a> 




- <a href=#run_mps>Run Gamess with MPS</a>

  - <a href=#submit_job2>Submit job</a>

  - <a href=#wait_job2>Wait for the jobs to start</a>

  - <a href=#hangs>Check that Gamess hangs</a>
     - <a href=#short_job2>JS=23947619</a>
     - <a href=#long_job2>JL=23942197</a>



<p>
<br>
<p>



<a name=setup></a>
## Set up



<a name=job_scripts></a>
### Job control scripts


Job control scripts without (subgms_batch_profile_ppn8x2y_pros.sh) and with (subgms_batch_profile_mps_ppn8x2y_pros.sh) MPS


```
  perlmutter $ cd /global/cfs/cdirs/m419/GAMESS/gamess-tsatta-offload_nvhpc_sort2_loadbl_gpu
  
  perlmutter $ ls -l \
      05_Defi_problem/scripts/subgms_batch_profile_ppn8x2y_pros.sh \
      05_Defi_problem/scripts/subgms_batch_profile_mps_ppn8x2y_pros.sh
  -rw-rw---- 1 gmateesc m419 5376 Aug  5  2023 05_Defi_problem/scripts/subgms_batch_profile_ppn8x2y_pros.sh      
  -rw-rw---- 1 gmateesc m419 5388 Aug  5  2023 05_Defi_problem/scripts/subgms_batch_profile_mps_ppn8x2y_pros.sh

```

The only difference is the block of code that executes srun

```
  perlmutter $ diff \
      05_Defi_problem/scripts/subgms_batch_profile_ppn8x2y_pros.sh \
      05_Defi_problem/scripts/subgms_batch_profile_mps_ppn8x2y_pros.sh

  6,7c6,7
  < #   sbatch -A m4265_g  -N 1 -n 128 --gpus-per-node=4  -t 6:00:00 subgms_batch_profile_ppn8x2y_pros.sh
  < #   sbatch -A m4265_g  -N 2 -n 256 --gpus-per-node=4  -t 3:00:00 subgms_batch_profile_ppn8x2y_pros.sh
  ---
  > #   sbatch -A m4265_g  -N 1 -n 128 --gpus-per-node=4  -t 6:00:00 subgms_batch_profile_mps_ppn8x2y_pros.sh
  > #   sbatch -A m4265_g  -N 2 -n 256 --gpus-per-node=4  -t 3:00:00 subgms_batch_profile_mps_ppn8x2y_pros.sh

  13c13
  < #   ./subgms_batch_profile_ppn8x2y_pros.sh
  ---
  > #   ./subgms_batch_profile_mps_ppn8x2y_pros.sh


  195c195
  < #without_mps () {   ===> block for without MPS
  ---
  > without_mps () {

  209c209
  < #}
  ---
  > }


  216c216
  < with_mps () {    ===> block for with MPS
  ---
  > #with_mps () {


  233c233
  < }
  ---
  > #}

```




<a name=wrapper_scripts></a>
### Wrapper scripts


<a name=mps_wrapper></a>
#### mps-wrapper.sh


```
  perlmutter $ cd /global/cfs/cdirs/m419/GAMESS/gamess-tsatta-offload_nvhpc_sort2_loadbl_gpu

  perlmutter $ cat  01_Gpus_for_gamess/mps-wrapper.sh | egrep -v "^ *#|^$"
  export CUDA_MPS_PIPE_DIRECTORY=/tmp/nvidia-mps
  export CUDA_MPS_LOG_DIRECTORY=/tmp/nvidia-log
  if [ $SLURM_LOCALID -eq 0 ]; then
    CUDA_VISIBLE_DEVICES=$SLURM_JOB_GPUS nvidia-cuda-mps-control -d
  fi

  sleep 5
  "$@"

  if [ $SLURM_LOCALID -eq 0 ]; then
    echo quit | nvidia-cuda-mps-control
  fi

```




<a name=map_gpus></a>
#### map-gpus.sh


```
  perlmutter $ cd /global/cfs/cdirs/m419/GAMESS/gamess-tsatta-offload_nvhpc_sort2_loadbl_gpu
  
  perlmutter $ cat  01_Gpus_for_gamess/map-gpus.sh | egrep -v "^ *#|^$"
  export CUDA_VISIBLE_DEVICES=$(( SLURM_LOCALID % 4 ))
  exec $*

```





<a name=run_womps></a>
## Run Gamess without MPS



<a name=submit_job></a>
### Submit job



I create two jobs: the long one will not exceed the time limit, but has to wait longer
to be satrted, the short one starts sooner but it will exceed the time limit:

```
  perlmutter $ cd /global/cfs/cdirs/m419/GAMESS/gamess-tsatta-offload_nvhpc_sort2_loadbl_gpu

  perlmutter $ sbatch  -A m4265_g -N 1 -n 128 \
                       --gpus-per-node=4 \
                       -t 07:55:00 \
		       05_Defi_problem/scripts/subgms_batch_profile_ppn8x2y_pros.sh
  Submitted batch job 23941996



  perlmutter $ sbatch  -A m4265_g -N 1 -n 128 \
                       --gpus-per-node=4 \
                       -t 02:55:00 \
		       05_Defi_problem/scripts/subgms_batch_profile_ppn8x2y_pros.sh
  Submitted batch job 23947569


```



<a name=wait_job></a>
### Wait for the jobs to start


Show the status of the two jobs

```
  perlmutter $ export J=23941996,23947569

  perlmutter $ fmt="--format=JobID,Jobname,Partition,AllocCPUS,state,time,start,elapsed,nnodes,ncpus,nodelist"


  perlmutter $ sacct -j $J $fmt
  JobID           JobName  Partition  AllocCPUS      State  Timelimit               Start    Elapsed   NNodes      NCPUS        NodeList 
  ------------ ---------- ---------- ---------- ---------- ---------- ------------------- ---------- -------- ---------- --------------- 
  23941996     subgms_ba+   gpu_ss11          0    PENDING   07:55:00             Unknown   00:00:00        1          0   None assigned 

  23947569     subgms_ba+   gpu_ss11        128    RUNNING   02:55:00 2024-04-04T06:47:28   00:14:57        1        128       nid001248 
  23947569.ba+      batch                   128    RUNNING            2024-04-04T06:47:28   00:14:57        1        128       nid001248 
  23947569.ex+     extern                   128    RUNNING            2024-04-04T06:47:28   00:14:57        1        128       nid001248 
  23947569.0   gamess.00+                   128    RUNNING            2024-04-04T06:47:41   00:14:44        1        128       nid001248 



  perlmutter $ sacct -j $J $fmt
  JobID           JobName  Partition  AllocCPUS      State  Timelimit               Start    Elapsed   NNodes      NCPUS        NodeList 
  ------------ ---------- ---------- ---------- ---------- ---------- ------------------- ---------- -------- ---------- --------------- 
  23941996     subgms_ba+   gpu_ss11        128    RUNNING   07:55:00 2024-04-04T16:29:40   00:56:41        1        128       nid008264 
  23941996.ba+      batch                   128    RUNNING            2024-04-04T16:29:40   00:56:41        1        128       nid008264 
  23941996.ex+     extern                   128    RUNNING            2024-04-04T16:29:40   00:56:41        1        128       nid008264 
  23941996.0   gamess.00+                   128    RUNNING            2024-04-04T16:29:49   00:56:32        1        128       nid008264 

  23947569     subgms_ba+   gpu_ss11        128    TIMEOUT   02:55:00 2024-04-04T06:47:28   02:55:22        1        128       nid001248 
  23947569.ba+      batch                   128  CANCELLED            2024-04-04T06:47:28   02:55:29        1        128       nid001248 
  23947569.ex+     extern                   128  COMPLETED            2024-04-04T06:47:28   02:55:27        1        128       nid001248 
  23947569.0   gamess.00+                   128  CANCELLED            2024-04-04T06:47:41   02:55:20        1        128       nid001248 

```




<a name=nohang></a>
### Check that Gamess does not hang


<a name=short_job></a>
#### JS=23947569


```
  perlmutter $ export JS=23947569
  
  perlmutter $ alias peek="ls -lh $PSCRATCH/$JS/output/JOB.*/*.log"

  perlmutter $ peek
  -rw-rw---- 1 gmateesc gmateesc 9.6M Apr  4 07:04 /pscratch/sd/g/gmateesc/23947569/output/JOB.23947569/gpu_usage_23947569_1.log
  -rw-r----- 1 gmateesc gmateesc 2.2M Apr  4 07:03 /pscratch/sd/g/gmateesc/23947569/output/JOB.23947569/JOB.23947569.log



  perlmutter $ alias peekf="tail -8 $PSCRATCH/$JS/output/JOB.*/JOB*.log"

  perlmutter $ peekf
  gpu time tdhf_apb (s)=           2.12
  gpu time tdhf_amb (s)=           1.73
  gpu time tdhf_apb (s)=           2.05
  gpu time tdhf_amb (s)=           1.69
  gpu time tdhf_apb (s)=           2.02
  gpu time tdhf_amb (s)=           1.69
  gpu time tdhf_apb (s)=           1.98
  gpu time tdhf_amb (s)=           1.64

```



The job needs more than 2h 55 min to complete, so it will be cancelled by SLURM when it reaches the maximum time

```
  perlmutter $ sacct -j $J $fmt
  JobID           JobName  Partition  AllocCPUS      State  Timelimit               Start    Elapsed   NNodes      NCPUS        NodeList 
  ------------ ---------- ---------- ---------- ---------- ---------- ------------------- ---------- -------- ---------- --------------- 
  23941996     subgms_ba+   gpu_ss11          0    PENDING   07:55:00             Unknown   00:00:00        1          0   None assigned 

  23947569     subgms_ba+   gpu_ss11        128    TIMEOUT   02:55:00 2024-04-04T06:47:28   02:55:22        1        128       nid001248 
  23947569.ba+      batch                   128  CANCELLED            2024-04-04T06:47:28   02:55:29        1        128       nid001248 
  23947569.ex+     extern                   128  COMPLETED            2024-04-04T06:47:28   02:55:27        1        128       nid001248 
  23947569.0   gamess.00+                   128  CANCELLED            2024-04-04T06:47:41   02:55:20        1        128       nid001248 
```


But the log file is telling us what happened
```
  perlmutter $ peekf
  CPU     0: STEP CPU TIME=    53.98 TOTAL CPU TIME=      10482.7 (    174.7 MIN)
  TOTAL WALL CLOCK TIME=      10482.7 SECONDS, CPU UTILIZATION IS   100.00%

          ---------------------
          ELECTROSTATIC MOMENTS
          ---------------------

  slurmstepd: error: *** STEP 23947569.0 ON nid001248 CANCELLED AT 2024-04-04T16:42:51 DUE TO TIME LIMIT ***
```



<a name=long_job></a>
#### JL=23941996


```
  perlmutter $ export JL=23941996


  perlmutter $ alias peek="ls -lh $PSCRATCH/$JL/output/JOB.*/*.log"

  perlmutter $ peek
  -rw-rw---- 1 gmateesc gmateesc  33M Apr  4 17:27 /pscratch/sd/g/gmateesc/23941996/output/JOB.23941996/gpu_usage_23941996_1.log
  -rw-r----- 1 gmateesc gmateesc 2.4M Apr  4 17:27 /pscratch/sd/g/gmateesc/23941996/output/JOB.23941996/JOB.23941996.log



  perlmutter $ alias peekf="tail -8 $PSCRATCH/$JL/output/JOB.*/JOB*.log"
  perlmutter $ peekf
  gpu time tdhf_apb (s)=           1.02
  gpu time tdhf_amb (s)=           0.85
  gpu time tdhf_apb (s)=           1.00
  gpu time tdhf_amb (s)=           0.85
  gpu time tdhf_apb (s)=           0.98
  gpu time tdhf_amb (s)=           0.83
  gpu time tdhf_apb (s)=           0.97
  slurmstepd: error: *** STEP 23941996.0 ON nid008264 CANCELLED AT 2024-04-05T00:27:02 ***


```




<a name=run_mps></a>
## Run Gamess with MPS



<a name=submit_job2></a>
### Submit job


I create two jobs: the long one will not exceed the time limit, but has to wait longer
to be satrted, the short one starts sooner but it will exceed the time limit:



```

  perlmutter $ cd /global/cfs/cdirs/m419/GAMESS/gamess-tsatta-offload_nvhpc_sort2_loadbl_gpu

  perlmutter $ sbatch  -A m4265_g -N 1 -n 128 \
                       --gpus-per-node=4 \
                       -t 07:55:00 \
                       05_Defi_problem/scripts/subgms_batch_profile_mps_ppn8x2y_pros.sh

  Submitted batch job 23942197




  perlmutter $ sbatch -A m4265_g -N 1 -n 128 \
                      --gpus-per-node=4 \
		      -t 02:55:00 \
		      05_Defi_problem/scripts/subgms_batch_profile_mps_ppn8x2y_pros.sh

  Submitted batch job 23947619


```



<a name=wait_job2></a>
### Wait for the jobs to start


Show the status of the two jobs

```
  perlmutter $ export J=23942197,23947619

  perlmutter $ fmt="--format=JobID,Jobname,Partition,AllocCPUS,state,time,start,elapsed,nnodes,ncpus,nodelist"

  perlmutter $ sacct -j $J $fmt
  JobID           JobName  Partition  AllocCPUS      State  Timelimit               Start    Elapsed   NNodes      NCPUS        NodeList 
  ------------ ---------- ---------- ---------- ---------- ---------- ------------------- ---------- -------- ---------- --------------- 
  23942197     subgms_ba+   gpu_ss11          0    PENDING   07:55:00             Unknown   00:00:00        1          0   None assigned 
  23947619     subgms_ba+   gpu_ss11          0    PENDING   02:55:00             Unknown   00:00:00        1          0   None assigned 



```




<a name=hangs></a>
### Check that Gamess hangs


<a name=short_job2></a>
#### JS=23947619


```

  perlmutter $ export JS=23947619  

  perlmutter $ alias peek="ls -lh $PSCRATCH/$JS/output/JOB.*/*.log"

  perlmutter $ alias peekf="tail -8 $PSCRATCH/$JS/output/JOB.*/JOB*.log"

```



<a name=long_job2></a>
#### JL=23942197


```
  perlmutter $ export JL=23942197

  perlmutter $ alias peek="ls -lh $PSCRATCH/$JL/output/JOB.*/*.log"

  perlmutter $ alias peekf="tail -8 $PSCRATCH/$JL/output/JOB.*/JOB*.log"

```
