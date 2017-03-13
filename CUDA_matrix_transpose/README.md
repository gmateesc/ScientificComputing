
# Efficient matrix transpose with CUDA


```
1. Matrix transpose code

   1.1 Layout

   1.2 Code skeleton

   1.3 Tile dimension


2. Build the transpose code

   2.1 CUDA versions

   2.2 Make files
       2.2.1 Makefile for CUDA-5.5
       2.2.2 Makefile for CUDA-6.0
              2.2.2.1 With compute capability 1.0
              2.2.2.2 With compute capability 2.0


3. Run code with srun

   3.1 Get device info

   3.2 The run scripts
         3.2.1 Script for CUDA-5.5
         3.2.2 Script for CUDA-6.0

   3.3 Run the scripts with srun
       3.3.1 Run CUDA-5.5 code
       3.3.2 Run CUDA-6.0 code
              3.3.2.1 CUDA-6.0 Compute capability 1.0
              3.3.2.2 CUDA-6.0 Compute capability 2.0
```




## Matrix transpose code


### Layout

```shell

  bash $ tree src
  src
  ├── matrix_1024_1024
  │   ├── Makefile
  │   ├── findcudalib.mk
  │   └── transpose.cu
  └── matrix_512_512
      ├── Makefile
      ├── findcudalib.mk
      └── transpose.cu

```


### Code skeleton

```C
 bash $ more transpose.cu
 // ...

__global__ void transpose(float *odata, const float *idata)
{
  __shared__ float tile[TILE_DIM][TILE_DIM+1];
    
  int x = blockIdx.x * TILE_DIM + threadIdx.x;
  int y = blockIdx.y * TILE_DIM + threadIdx.y;
  int width = gridDim.x * TILE_DIM;

  for (int j = 0; j < TILE_DIM; j += BLOCK_ROWS)
     tile[threadIdx.y+j][threadIdx.x] = idata[(y+j)*width + x];

  __syncthreads();

  x = blockIdx.y * TILE_DIM + threadIdx.x;  // transpose block offset
  y = blockIdx.x * TILE_DIM + threadIdx.y;

  for (int j = 0; j < TILE_DIM; j += BLOCK_ROWS)
     odata[(y+j)*width + x] = tile[threadIdx.x][threadIdx.y + j];
}
```



### Tile dimension

  bash $ egrep "TILE_DIM =" src/matrix_*_*/*cu
  src/matrix_1024_1024/transpose.cu:const int TILE_DIM = 32;
  src/matrix_512_512/transpose.cu:const int TILE_DIM = 32;







## Build the transpose code



### CUDA versions


The code was tested with CUDA-5.5 and CUDA-6.0

```shell
  bash $ locate nvcc | grep bin/nvcc
  /opt/apps/cuda/5.0/bin/nvcc
  /opt/apps/cuda/5.0/bin/nvcc.profile

  /opt/apps/cuda/5.5/bin/nvcc
  /opt/apps/cuda/5.5/bin/nvcc.profile

  /opt/apps/cuda/6.0/bin/nvcc
  /opt/apps/cuda/6.0/bin/nvcc.profile
```





### Make files


#### Makefile for CUDA-5.5


The Makfile is a slightly modifuied version of the Makefile example provided by NVIDIA.

```Makefile

  bash $ more src/matrix_512_512/Makefile 
  # ...

  include ./findcudalib.mk

  # Location of the CUDA Toolkit
  CUDA_PATH ?= "/usr/local/cuda-5.5"

  # internal flags
  NVCCFLAGS   += -m${OS_SIZE}
  CCFLAGS     :=
  NVCCLDFLAGS :=
  LDFLAGS     :=

  # Extra user flags
  EXTRA_NVCCFLAGS   ?=
  EXTRA_NVCCLDFLAGS ?=
  EXTRA_LDFLAGS     ?=
  EXTRA_CCFLAGS     ?=

  # OS-specific build flags
  ifneq ($(DARWIN),) 
    LDFLAGS += -rpath $(CUDA_PATH)/lib
    CCFLAGS += -arch $(OS_ARCH) $(STDLIB)  
  else
    ifeq ($(OS_ARCH),armv7l)
      ifeq ($(abi),gnueabi)
        CCFLAGS += -mfloat-abi=softfp
      else
        # default to gnueabihf
        override abi := gnueabihf
        LDFLAGS += --dynamic-linker=/lib/ld-linux-armhf.so.3
        CCFLAGS += -mfloat-abi=hard
      endif
    endif
  endif

  ifeq ($(ARMv7),1)
  NVCCFLAGS += -target-cpu-arch ARM
  ifneq ($(TARGET_FS),) 
  CCFLAGS += --sysroot=$(TARGET_FS)
  LDFLAGS += --sysroot=$(TARGET_FS)
  LDFLAGS += -rpath-link=$(TARGET_FS)/lib
  LDFLAGS += -rpath-link=$(TARGET_FS)/usr/lib
  LDFLAGS += -rpath-link=$(TARGET_FS)/usr/lib/arm-linux-$(abi)
  endif
  endif

  # Debug build flags
  ifeq ($(dbg),1)
      NVCCFLAGS += -g -G
      TARGET := debug
  else
      TARGET := release
  endif

  ALL_CCFLAGS :=
  ALL_CCFLAGS += $(NVCCFLAGS)
  ALL_CCFLAGS += $(addprefix -Xcompiler ,$(CCFLAGS))
  ALL_CCFLAGS += $(EXTRA_NVCCFLAGS)
  ALL_CCFLAGS += $(addprefix -Xcompiler ,$(EXTRA_CCFLAGS))

  ALL_LDFLAGS :=
  ALL_LDFLAGS += $(ALL_CCFLAGS)
  ALL_LDFLAGS += $(NVCCLDFLAGS)
  ALL_LDFLAGS += $(addprefix -Xlinker ,$(LDFLAGS))
  ALL_LDFLAGS += $(EXTRA_NVCCLDFLAGS)
  ALL_LDFLAGS += $(addprefix -Xlinker ,$(EXTRA_LDFLAGS))

  # Common includes and paths for CUDA
  INCLUDES  := -I$(CUDA_PATH)/samples/common/inc
  LIBRARIES :=

  LIBRARIES += -lcublas

  PROGRAM := transpose

  # Target rules
  all: build

  build: $(PROGRAM)

  $(PROGRAM).o: $(PROGRAM).cu
	$(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -o $@ -c $<

  $(PROGRAM): $(PROGRAM).o
	$(NVCC) $(ALL_LDFLAGS) -o $@ $+ $(LIBRARIES)
	mkdir -p $(HOME)/bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))
	cp $@ $(HOME)/bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))

  run: build
	./$(PROGRAM)

  clean:
	rm -f $(PROGRAM).o $(PROGRAM)
	rm -rf $(HOME)/bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))/$(PROGRAM)

  clobber: clean
```



Build the code:
```
  bash $ make
  "/opt/apps/cuda/5.5"/bin/nvcc -ccbin g++ -I"/opt/apps/cuda/5.5"/samples/common/inc  \
       -m64     -o transpose.o -c transpose.cu
  "/opt/apps/cuda/5.5"/bin/nvcc -ccbin g++   -m64        -o transpose transpose.o  -lcublas

  mkdir -p /home1/02990/gmateesc/bin/x86_64/linux/release

  cp transpose /home1/02990/gmateesc/bin/x86_64/linux/release
```



#### Makefile for CUDA-6.0

See

  http://www.pgroup.com/userforum/viewtopic.php?t=4383&sid=90a5dc1510a2c6b520c630b9606969ba


##### With compute capability 1.0

Change the Makefile


  bash $ diff Makefile-CUDA-6.0  Makefile
  40c40
  < CUDA_PATH ?= "/opt/apps/cuda/6.0"
  ---
  > CUDA_PATH ?= "/opt/apps/cuda/5.5"
  ---


Make

```
  bash $ make clean
  rm -f transpose.o transpose
  rm -rf /home1/02990/gmateesc/bin/x86_64/linux/release/transpose
 

  bash $ make -f Makefile-CUDA-6.0
  "/opt/apps/cuda/6.0"/bin/nvcc -ccbin g++ -I"/opt/apps/cuda/6.0"/samples/common/inc  -m64 \
        -o transpose.o -c transpose.cu
  nvcc warning : The 'compute_10' and 'sm_10' architectures are deprecated, \
  and may be removed in a future release.

  "/opt/apps/cuda/6.0"/bin/nvcc -ccbin g++   -m64    \
             -o transpose transpose.o  -lcublas
  nvcc warning : The 'compute_10' and 'sm_10' architectures are deprecated, \
  and may be removed in a future release.

  mkdir -p /home1/02990/gmateesc/bin/x86_64/linux/release
  cp transpose /home1/02990/gmateesc/bin/x86_64/linux/release
```

NOTE:  

  The above nvcc command is equivalent to specifying compute capability 1.0
```
  $ /opt/apps/cuda/6.0/bin/nvcc -arch compute_10 -ccbin g++ \
           -I/opt/apps/cuda/6.0/samples/common/inc  -o transpose.o -c transpose.cu
```



##### With compute capability 2.0


Change makefile

```
  bash $ diff Makefile-CUDA-6.0_cc20  Makefile
  40c40
  < CUDA_PATH ?= "/opt/apps/cuda/6.0"
  ---
  > CUDA_PATH ?= "/opt/apps/cuda/5.5"

  43c43
  < NVCCFLAGS   += -arch compute_20 -m${OS_SIZE}
  ---
  > NVCCFLAGS   += -m${OS_SIZE}
```


Make

```
  bash $ make clean
  rm -f transpose.o transpose
  rm -rf /home1/02990/gmateesc/bin/x86_64/linux/release/transpose
  
  bash $ make -f Makefile-CUDA-6.0_cc20
  "/opt/apps/cuda/6.0"/bin/nvcc -ccbin g++ -I"/opt/apps/cuda/6.0"/samples/common/inc  \
          -arch compute_20 -m64  -o transpose.o -c transpose.cu
  "/opt/apps/cuda/6.0"/bin/nvcc -ccbin g++   -arch compute_20 -m64    \
          -o transpose transpose.o  -lcublas
  mkdir -p /home1/02990/gmateesc/bin/x86_64/linux/release
  cp transpose /home1/02990/gmateesc/bin/x86_64/linux/release

```



NOTE:  

  The above nvcc command specifies compute capability 2.0
```
  $ /opt/apps/cuda/6.0/bin/nvcc -arch compute_20 -ccbin g++ -I/opt/apps/cuda/6.0/samples/common/inc  \
                                -o transpose.o -c transpose.cu
```






## Run code with srun


### Get device info


Script
```
  bash $ more   ~/bin/run_nvidia-smi.sh
  #!/bin/bash -l

  # srun -N 1 -n 1 -t 05:00 -p gpudev /bin/bash -l

  module load cuda/6.0

  nvidia-smi -q
```




Run

```
  bash $ srun -N 1 -n 1 -t 01:00 -p gpudev  ~/bin/run_nvidia-smi.sh

--> Verifying valid submit host (login1)...OK
--> Verifying valid jobname...OK
--> Enforcing max jobs per user...OK
--> Verifying availability of your home dir (/home1/02990/gmateesc)...OK
--> Verifying availability of your work dir (/work/02990/gmateesc)...OK
--> Verifying availability of your scratch dir (/scratch/02990/gmateesc)...OK
--> Verifying valid ssh keys...OK
--> Verifying access to desired queue (gpudev)...OK
--> Verifying job request is within current queue limits...OK
--> Checking available allocation (TG-ECS140003)...OK

  ==============NVSMI LOG==============

  Timestamp                           : Sun Jan 25 18:15:29 2015
  Driver Version                      : 331.67

 Attached GPUs                       : 1
  GPU 0000:03:00.0
    Product Name                    : Tesla K20m
    Display Mode                    : Disabled
    Display Active                  : Disabled
    Persistence Mode                : Disabled
    Accounting Mode                 : Disabled
    Accounting Mode Buffer Size     : 128
    Driver Model
        Current                     : N/A
        Pending                     : N/A
    Serial Number                   : 0324712062794
    GPU UUID                        : GPU-21e1c112-2192-d89b-0112-e34ee6f77cb2
    Minor Number                    : 0
    VBIOS Version                   : 80.10.11.00.0B
    Inforom Version
        Image Version               : 2081.0208.01.07
        OEM Object                  : 1.1
        ECC Object                  : 3.0
        Power Management Object     : N/A
    GPU Operation Mode
        Current                     : All On
        Pending                     : All On
    PCI
        Bus                         : 0x03
        Device                      : 0x00
        Domain                      : 0x0000
        Device Id                   : 0x102810DE
        Bus Id                      : 0000:03:00.0
        Sub System Id               : 0x101510DE
        GPU Link Info
            PCIe Generation
                Max                 : 2
                Current             : 2
            Link Width
                Max                 : 16x
                Current             : 16x
        Bridge Chip
            Type                    : N/A
            Firmware                : N/A
    Fan Speed                       : N/A
    Performance State               : P0
    Clocks Throttle Reasons
        Idle                        : Not Active
        Applications Clocks Setting : Active
        SW Power Cap                : Not Active
        HW Slowdown                 : Not Active
        Unknown                     : Not Active
    FB Memory Usage
        Total                       : 4799 MiB
        Used                        : 13 MiB
        Free                        : 4786 MiB
    BAR1 Memory Usage
        Total                       : 256 MiB
        Used                        : 2 MiB
        Free                        : 254 MiB
    Compute Mode                    : Default
    Utilization
        Gpu                         : 96 %
        Memory                      : 6 %
    Ecc Mode
        Current                     : Enabled
        Pending                     : Enabled
    ECC Errors
        Volatile
            Single Bit            
                Device Memory       : 0
                Register File       : 0
                L1 Cache            : 0
                L2 Cache            : 0
                Texture Memory      : 0
                Total               : 0
            Double Bit            
                Device Memory       : 0
                Register File       : 0
                L1 Cache            : 0
                L2 Cache            : 0
                Texture Memory      : 0
                Total               : 0
        Aggregate
            Single Bit            
                Device Memory       : 0
                Register File       : 0
                L1 Cache            : 0
                L2 Cache            : 0
                Texture Memory      : 0
                Total               : 0
            Double Bit            
                Device Memory       : 0
                Register File       : 0
                L1 Cache            : 0
                L2 Cache            : 0
                Texture Memory      : 0
                Total               : 0
    Retired Pages
        Single Bit ECC              : 0
        Double Bit ECC              : 0
        Pending                     : No
    Temperature
        Gpu                         : 19 C
    Power Readings
        Power Management            : Supported
        Power Draw                  : 41.06 W
        Power Limit                 : 225.00 W
        Default Power Limit         : 225.00 W
        Enforced Power Limit        : 225.00 W
        Min Power Limit             : 150.00 W
        Max Power Limit             : 225.00 W
    Clocks
        Graphics                    : 705 MHz
        SM                          : 705 MHz
        Memory                      : 2600 MHz
    Applications Clocks
        Graphics                    : 705 MHz
        Memory                      : 2600 MHz
    Default Applications Clocks
        Graphics                    : 705 MHz
        Memory                      : 2600 MHz
    Max Clocks
        Graphics                    : 758 MHz
        SM                          : 758 MHz
        Memory                      : 2600 MHz
    Compute Processes               : None
```







### The run scripts


#### Script for CUDA-5.5

```
  bash $ more run_transpose.sh 
  #!/bin/bash -l

  # srun -N 1 -n 1 -t 05:00 -p gpudev /bin/bash -l

  module load cuda/5.5

  $HOME/bin/x86_64/linux/release/transpose
```


#### Script for CUDA-6.0

```
  bash $ more run_transpose_cuda-6.0.sh 
  #!/bin/bash -l

  # srun -N 1 -n 1 -t 05:00 -p gpudev /bin/bash -l

  module load cuda/6.0

  $HOME/bin/x86_64/linux/release/transpose
```



### Run the scripts with srun


Run
```
  $ srun -N 1 -n 1 -t 01:00 -p gpudev run_transpose.sh
```


#### Run CUDA-5.5 code
```
  bash $ srun -N 1 -n 1 -t 01:00 -p gpudev  /home1/02990/gmateesc/bin/run_transpose_32_32_32_8.sh

  --> Verifying valid submit host (login1)...OK
  --> Verifying valid jobname...OK
  --> Enforcing max jobs per user...OK
  --> Verifying availability of your home dir (/home1/02990/gmateesc)...OK
  --> Verifying availability of your work dir (/work/02990/gmateesc)...OK
  --> Verifying availability of your scratch dir (/scratch/02990/gmateesc)...OK
  --> Verifying valid ssh keys...OK
  --> Verifying access to desired queue (gpudev)...OK
  --> Verifying job request is within current queue limits...OK
  --> Checking available allocation (TG-ECS140003)...OK

  Device : Tesla K20m
  Matrix size: 1024 1024, Block size: 32 8, Tile size: 32 32
  dimGrid: 32 32 1. dimBlock: 32 8 1
                  Routine         Bandwidth (GB/s)
                     copy              135.85
       shared memory copy              152.66
          naive transpose               53.83
      coalesced transpose               93.96
  conflict-free transpose              142.91
```




#### Run CUDA-6.0 code



##### CUDA-6.0 Compute capability 1.0

```
  bash $ srun -N 1 -n 1 -t 01:00 -p gpudev run_transpose_cuda-6.0.sh 

  --> Verifying valid submit host (login1)...OK
  --> Verifying valid jobname...OK
  --> Enforcing max jobs per user...OK
  --> Verifying availability of your home dir (/home1/02990/gmateesc)...OK
  --> Verifying availability of your work dir (/work/02990/gmateesc)...OK
  --> Verifying availability of your scratch dir (/scratch/02990/gmateesc)...OK
  --> Verifying valid ssh keys...OK
  --> Verifying access to desired queue (gpudev)...OK
  --> Verifying job request is within current queue limits...OK
  --> Checking available allocation (TG-ECS140003)...OK

  Device : Tesla K20m
  Matrix size: 1024 1024, Block size: 32 8, Tile size: 32 32
  dimGrid: 32 32 1. dimBlock: 32 8 1
                  Routine         Bandwidth (GB/s)
                     copy              135.66
       shared memory copy              152.50
          naive transpose               53.85
      coalesced transpose               94.08
  conflict-free transpose              143.30
```




##### CUDA-6.0 Compute capability 2.0

This is worse than using compute capability 1.0:

```
  bash $ srun -N 1 -n 1 -t 01:00 -p gpudev  /home1/02990/gmateesc/bin/run_transpose_32_32_32_8_cuda-6.0.sh

  --> Verifying valid submit host (login1)...OK
  --> Verifying valid jobname...OK
  --> Enforcing max jobs per user...OK
  --> Verifying availability of your home dir (/home1/02990/gmateesc)...OK
  --> Verifying availability of your work dir (/work/02990/gmateesc)...OK
  --> Verifying availability of your scratch dir (/scratch/02990/gmateesc)...OK
  --> Verifying valid ssh keys...OK
  --> Verifying access to desired queue (gpudev)...OK
  --> Verifying job request is within current queue limits...OK
  --> Checking available allocation (TG-ECS140003)...OK

  Device : Tesla K20m
  Matrix size: 1024 1024, Block size: 32 8, Tile size: 32 32
  dimGrid: 32 32 1. dimBlock: 32 8 1

                  Routine         Bandwidth (GB/s)
                     copy              136.41
       shared memory copy              141.43
          naive transpose               53.92
      coalesced transpose               87.73
  conflict-free transpose              130.56
```


