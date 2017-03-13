#ifndef _MAIN_H_
#define _MAIN_H_


#include "match.h"
#include "sff.h"
#include "log.h"


/* DEFINES */
#define VERSION                      "1.0.0"
#define PRG_NAME                     "split_sff"
#define FILENAME_MAX_LENGTH          1024
#define FASTQ_FILENAME_MAX_LENGTH    FILENAME_MAX_LENGTH
#define SFF_FILENAME_MAX_LENGTH      FILENAME_MAX_LENGTH
#define ADAPTER_FILENAME_MAX_LENGTH  FILENAME_MAX_LENGTH

#define MAX_NUM_ADAPTERS 256


void sig_handler(int signo);

void help_message(void);
void version_info(void);

void process_options(int argc, 
		     char *argv[]
		     );

void split_sff_using_adapters(char *sff_file);


void finalize_file_write( 
			  int pat_idx
			  );


void init_split_file_arrays( int num_patterns );


#endif
