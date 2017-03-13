

# Program to split an SFF file based on a list of adapters

```
       Gabriel Mateescu
       mateescu@acm.org



Table of Contents


  1. Summary

  2. Installation

  3. Example Usage

  4. Highlights of the code
      4.1 Description of the code
      4.2 Key Ideas
      4.3 Splittig kernel
      4.4 Limitations 

```


## Summary

This program splits an input SFF file into a number of 
files, such that each split file contains the reads that 
match an adapter specified in a list of adapters 
that are given in an input adapter list. 

If the adapters are specified as a key, value pair, e.g., 

```
  IonXpress_001   CTAAGGTAAC
  IonXpress_002   TAAGGAGAAC
  ...
```
then the split SFF files have the names
```
   split_01.sff - contain the adapter CTAAGGTAAC with the key IonXpress_001   
   split_02.sff - contain the adapter TAAGGAGAAC with the key IonXpress_002   
   ...
```

If the pattern IonXpress_0XX does not occur in any read, 
then the file splut_0XX.sff has ony the common block, 
whose number-of-reads field has the value 0.




## Installation

Pull the code from git, then execute
```
   $ cd Split_SFF_files

   $ make all
   gcc -g -Iinclude  -fopenmp -c src/main.c
   gcc -g -o split_sff  main.o sff.o match.o  -fopenmp 
   gcc -g -Iinclude  -o main_ser.o -c src/main.c
   gcc -g -o split_sff_ser  main_ser.o sff_ser.o match_ser.o
```
The outcome of running make includes two executables

- split_sff       parallel OpenMP code 
- split_sff_ser   serial code

The part of the code that is parallelized is described 
below in the section "Splittig kernel".




## Example Usage


To do the splitting taking into account the clipping values 
for each sequence of bases, i.e., looking for the adapter 
sequence in the adapter region at the beginning of the 
sequence:
```
  split_sff  -a ionXpress_barcode.txt  data.sff 
```
or 
```
  split_sff_ser  -a ionXpress_barcode.txt  data.sff 
```
To do the splitting without taking into account the 
clipping values, i.e., looking for the adapter 
sequence in the full sequence of bases of the read, run
```
  split_sff  -c -a ionXpress_barcode.txt  data.sff 
```
or
```
  split_sff_ser  -c -a ionXpress_barcode.txt  data.sff 
```


For full usage options, run 
```
   split_sff -h
```




## Highlights of the code



### Description of the code


The code I wrote contains three modules:
  - sff.c 
  - match.c
  - main.c

where
```
sff.c  The module for reading an writing sff files

       The part for reading .sff files is based on the 
       public domain code authored by 

           Indraniel Das <indraniel@gmail.com>

       and available at

         https://github.com/indraniel/sff2fastq

       I improved this part by adding error checking, 
       e.g., after each fread() call.

       I added code to write the common header 
       of the SFF file, the header for each read, 
       and the data for each read.


match.c  Contains functions to match a pattern against 
         a text that contains the sequence of bases from 
         an SFF file.


main.c  Contains the main function and related 
        functions for splitting an SFF file into 
        a number of files, such that each 
        split file contains the reads (from 
        the originial SFF file) that match 
        one of the patterns specified in 
        a list of patterns.

        The two main functions called by main() are

        process_options   based on the code at
                          https://github.com/indraniel/sff2fastq

        split_sff_using_adapters the high level function for 
                                 splitting the .SFF file, which 
                                 uses helper functions in match.c 
                                 and sff.c
```


### Key Ideas

The key idea in organizing the splutting code has 
been to read each data section from the SFF file 
only once and process that section -- potentially in 
parallel -- writing a split file for the pattern 
that matches the read.

Parallel processing works because, even if a read 
matches two adapters, a separate file is written 
for each adapter, so one can have as many parallel 
processes as there are adapters in the adapter list.

Below I describe how these ideas are implemented 
in the code.



### Splittig kernel

The kernel code doing the splitting is in the function:
```
   split_sff_using_adapters()
```
using two loops
```
   for (i = 0; i < ch.nreads; i++) {

      #pragma omp parallel for schedule(static,8) 
      for (pat_idx = 0; pat_idx < num_patterns; pat_idx++ ) {
            ...
      }
   }
```

    
These nested loops are in the split_sff_using_adapters() 
in the main.c module.


Parallelization of the inner loop is enabled in the 
executabls
```
   split_sff      parallel OpenMP code 
```

by compiling and linking the code with the -fopenmp 
option, while the executable
```
   split_sff_ser  
```
is the serial code.

For the parallel version, the environment variable 
OMP_NUM_THREADS can be used to control the number 
of threads, e.g.,
```
   export OMP_NUM_THREADS=4 

   split_sff  -a ionXpress_barcode.txt  data.sff 
```



### Limitations 

The current version does not create a split file for the reads that 
do not match any adapter.


