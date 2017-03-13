#ifndef _MATCH_H_
#define _MATCH_H_

#include "sff.h"
#include "log.h"




void match_read_pattern (	  
	    sff_common_header * ch, 
	    sff_read_header   * rh, 
	    sff_read_data     * rd, 
	    char              * pattern, 
            int                 pat_idx, 
	    FILE              * sff_fp, 
            FILE             ** sff_split_fp,
	    uint32_t          * nreads_split_file, 
	    uint32_t            read_num, 
	    int                 opt_no_clipping, 
	    int                 dry_run
				  );

int     match(char text[], char pattern[]);

int get_patterns(char * file_name,  char *** ptr); 

char * get_adapter ( char * line );

char * get_string (void *buf, int * start);

void * load_file (char *file_name, size_t * file_size);

#endif


