/*

  Program to split an SFF file into 
  a number of files, such that each 
  split file contains the reads (from 
  the originial SFF file) that match 
  one of the patterns specified in 
  a list of patterns.

  Author

     Gabriel Mateescu  mateescu@acm.org
*/



/** INCLUDES **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "main.h"




/** GLOBALS **/

char sff_file[SFF_FILENAME_MAX_LENGTH] = { '\0' };
char ad_file[ADAPTER_FILENAME_MAX_LENGTH] = { '\0' };
FILE * sff_split_fp[MAX_NUM_ADAPTERS] = { NULL }; 
char * sff_split_file[MAX_NUM_ADAPTERS] = { NULL }; 

// Ignore clipping values for the discovery of the adapter, 
// i.e., look for the adapter in the whole sequence
int  opt_no_clipping = 0;

// Dry run: do not write the split .sff files
int dry_run = 0;

uint32_t * nreads_split_file = NULL;

sff_common_header ch;


// Number of adapter patterns in the input file
int num_patterns;


/** MAIN **/

int main(int argc, char *argv[]) {

  //
  // Register application-specific handler 
  // for several signals
  //
  signal(SIGINT,  sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGHUP,  sig_handler);
  signal(SIGCHLD, sig_handler);
  signal(SIGPIPE, sig_handler);

    
  process_options(argc, argv);
  
  split_sff_using_adapters(sff_file);

  printf("Completed splitting\n");
  
  return 0;

} // main()



/** FUNCTIONS **/


void 
sig_handler(int signo)
{

  //
  // Finalize writing to the split files 
  // then invoke the default handler
  //
  int pat_idx;

  fprintf2_(stderr, "Received signal %d\n", signo);
  fflush(stderr);

  if ( ! dry_run ) { 

    for (pat_idx = 0; pat_idx < num_patterns; pat_idx++) {
      finalize_file_write(pat_idx);  
    }

  }

  signal(signo, SIG_DFL);
  raise(signo);

}



void
help_message() {
    fprintf(stdout, "Usage: %s %s %s\n", PRG_NAME, "[options]", "<sff_file>");
    fprintf(stdout, "\t%-20s%-20s\n", "-h", "This help message");
    fprintf(stdout, "\t%-20s%-20s\n", "-v", "Program and version information");
    fprintf(stdout, "\t%-20s%-20s\n", "-c", "Ignore clipping limits for adapter match");
    fprintf(stdout, "\t%-20s%-20s\n", "-r", "Dry run: do not write the split sff files");
    fprintf(stdout, "\t%-20s%-20s %s\n",
                    "-a <adapter_file>",
                    "Adapter file containing the list of adapters.",
                    "Must be specified");


}



void
version_info() {
    fprintf(stdout, "%s -- version: %s\n", PRG_NAME, VERSION);
}



void
process_options(int argc, char *argv[]) {
    int c;
    int index;
    char *opt_a_value = NULL;

    while( (c = getopt(argc, argv, "hvcra:")) != -1 ) {
        switch(c) {
            case 'h':
                help_message();
                exit(0);
                break;
            case 'v':
                version_info();
                exit(0);
                break;
            case 'c':
                opt_no_clipping = 1; 
                break;
            case 'r':
                dry_run = 1; 
                break;
            case 'a':
                opt_a_value = optarg;
                break;
            case '?':
                exit(1);
             default:
                abort();
        }
    }


    if ( opt_a_value ) {
	strncpy(ad_file, opt_a_value, ADAPTER_FILENAME_MAX_LENGTH);
    }

    /* ensure that an adapter file was passed in */
    if ( ! strlen(ad_file) ) {
        fprintf(stderr, "%s %s '%s %s' %s\n",
                "[err] Need to specify an adapter file!",
                "See", PRG_NAME, "-h", "for usage!");
        exit(1);
    }



    /* process the remaining command line arguments */      

    // take the last non-getopt argument as the sff file name
    if ( optind < argc ) {
      strncpy(sff_file, argv[argc-1], SFF_FILENAME_MAX_LENGTH);
    }

    // ensure that an sff file name was passed in 
    if ( !strlen(sff_file) ) {
        fprintf(stderr, "%s %s '%s %s' %s\n",
                "[err] Need to specify a sff file!",
                "See", PRG_NAME, "-h", "for usage!");
        exit(1);
    }
}



//
// Read sff and split it according to a set of adapters
//

void 
split_sff_using_adapters(char *sff_file) 
{

    //sff_common_header ch;
    sff_read_header     rh;
    sff_read_data       rd;
    FILE  *         sff_fp;

    register int i, pat_idx;

    char *          str;     // temp string


    //
    // 1. Setup
    //


    //
    // 1.1 Get handle to SFF file
    //
    if ( (sff_fp = fopen(sff_file, "r")) == NULL ) {
        fprintf(stderr,
                "[err] Could not open sff file '%s' for reading.\n", sff_file);
        exit(1);
    }



    //
    // 1.2 Get the list of adapter sequences from the adapter file
    //
    char **patterns  = NULL;
    num_patterns = get_patterns(ad_file, &patterns);
    fprintf_(stderr, "  Size of patterns[] arr  :  %d\n" , num_patterns);

    // DEBUG
    //patterns[0] = strdup("AAGAGGATTC");  // IonXpress_003
    //patterns[1] = strdup("CTAAGGTAAC");  // IonXpress_001
    //patterns[2] = strdup("TACCAAGATC");  // IonXpress_004
    //num_patterns = 36;
    
    for (pat_idx=0; pat_idx < num_patterns; pat_idx++) {
      fprintf_(stderr, "pat[%2d] = %s\n", pat_idx, patterns[pat_idx]);
    }
    //return;

    nreads_split_file = malloc ( num_patterns * sizeof(uint32_t) );
    if ( nreads_split_file == NULL ) {
	fprintf(stderr, "Could not allocate memory for nreads_split[%d] array \n", num_patterns);
	exit(1);
    }



    //
    // 1.4 Initialize arrays nreads_split_file[] and sff_split_file[]
    //
    init_split_file_arrays(num_patterns);


    //
    // 1.5 Build split for each adapter
    //

    //
    // 1.5 Open the sff split files
    //
    if ( ! dry_run ) {

      for (pat_idx = 0; pat_idx < num_patterns; pat_idx++ ) {

	sff_split_fp[pat_idx] = fopen(sff_split_file[pat_idx], "w");    
	if ( sff_split_fp[pat_idx] == NULL ) {
	  fprintf(stderr,
		  "[err] Could not open file '%s' for wrting the split sff number %d.\n",
		  sff_split_file[pat_idx], pat_idx);
	  exit(1);
	}
      }

    }


    //
    // 2. Process the SFF header common to all reads
    //
    read_sff_common_header(sff_fp, &ch);
    verify_sff_common_header(PRG_NAME, VERSION, &ch);

    fprintf_(stderr, "Common header:\n");
    fprintf_(stderr, "\tmagic        : 0x%x (decimal %d)\n", ch.magic, ch.magic);
    fprintf_(stderr, "\tversion      : %x%x%x%x\n",  ch.version[0], ch.version[1], ch.version[2], ch.version[3] );
    fprintf_(stderr, "\tindex_offset : 0x%llx\n", ch.index_offset);
    fprintf_(stderr, "\tindex_len    : %d\n",     ch.index_len);
    fprintf_(stderr, "\tnum_reads    : %d\n",     ch.nreads);
    fprintf_(stderr, "\theader_len   : %d\n",     ch.header_len);
    fprintf_(stderr, "\tkey_len      : %d\n",     ch.key_len);
    fprintf_(stderr, "\tnum_flows    : %d\n",     ch.flow_len);
    fprintf_(stderr, "\tflowgram_fmt : 0x%x\n" ,  ch.flowgram_format);

    str = strndup(ch.flow, ch.flow_len);
    if (!str) {
      fprintf(stderr, "Out of memory when allocating the flow characters\n");
      exit(1);
    }
    fprintf_(stderr, "\tflow_chars   : %s\n  " , str);
    free (str);

    str = strndup(ch.key, ch.key_len);
    if (!str) {
      fprintf(stderr, "Out of memory when allocating the key_seq string!\n");
      exit(1);
    }
    fprintf_(stderr, "\tkey_seq      : %s\n  " , str);
    free (str);

    fprintf_(stderr, "\n");



    //
    // 3. Process each read (header + data)
    //

    // DEBUG
    //for (i = 0; i < min(20000, ch.nreads); i++) {
    for (i = 0; i < ch.nreads; i++) {

      int left_clip = 0, right_clip = 0, nbases = 0;

      fprintf_(stderr, "\n\nProcess read number %d\n" , i);


      //
      // 3.1 Process read header
      //
      read_sff_read_header(sff_fp, &rh);
      
      fprintf_(stderr, "Read header:\n");
      fprintf_(stderr, "\theader_len        : %d\n", rh.header_len);
      fprintf_(stderr, "\tname_len          : %d\n", rh.name_len);
      fprintf_(stderr, "\tnum_bases         : %d\n", rh.nbases);
      fprintf_(stderr, "\tclip_qual_left    : %d\n", rh.clip_qual_left);
      fprintf_(stderr, "\tclip_qual_right   : %d\n", rh.clip_qual_right);
      fprintf_(stderr, "\tclip_adapter_left : %d\n", rh.clip_adapter_left);
      fprintf_(stderr, "\tclip_adapter_right: %d\n", rh.clip_adapter_right);
      
      
      // Convert rh.name (name of the read) to null-terminated string
      str = strndup(rh.name, rh.name_len);
      if (! str ) {
	fprintf(stderr, "Out of memory! For read name string!\n");
	exit(1);
      }
      fprintf_(stderr, "\tname              : %s\n", str);
      free(str);


      //
      // 3.2 Get the data for this read
      //
      fprintf_(stderr, "\nRead data:\n");

      // Fill in the rd structure with the data for this read
      read_sff_read_data(sff_fp, &rd, ch.flow_len, rh.nbases, i);

      

      //
      // 3.3 Match the bases in this read against the adapter
      //     pattern defined by the "pattern" string and 
      //     write the read to the appropriate sff split
      //

#pragma omp parallel for \
  schedule(static,8)     \
  shared(nreads_split_file, patterns, sff_fp, sff_split_fp, ch, rh, rd, i, dry_run) 
      for (pat_idx = 0; pat_idx < num_patterns; pat_idx++ ) {

	match_read_pattern(&ch, &rh, &rd, patterns[pat_idx], pat_idx, 
			   sff_fp,  sff_split_fp, nreads_split_file, 
			   i, opt_no_clipping, dry_run);

      }
      

      
      //
      // 3.5 Free dynamic memory allocated for this read
      //
      free_sff_read_header(&rh);
      free_sff_read_data(&rd);

    } // for (i = 0; i < ch.numreads; ) { ... }
    


    //
    // 4. Update common header
    //

    if ( ! dry_run ) {
      for (pat_idx = 0; pat_idx < num_patterns; pat_idx++ ) {
	finalize_file_write ( pat_idx ); 
      }
    }



    //
    // 5. Clean up
    //
    free_sff_common_header(&ch);
    fclose(sff_fp);



} // split_sff_using_adapters()




//
// Set up the list of names of the split files in
// 
//    sff_split_file[] 
//
void 
init_split_file_arrays( int num_patterns ) 
{

  int pat_idx;
  char * str;

  for (pat_idx = 0; pat_idx < num_patterns; pat_idx++) {

    //
    // Reset number of of matching reads for each pattern
    //
    nreads_split_file[pat_idx] = 0;
    
    //
    // Set the names of the split file 
    // corresponding to a pattern
    //
    
    char template[] = "split_XXX.sff";
    size_t sz = 1 + strlen(template);
    
    str = malloc( sz * sizeof(char));
    if ( ! str ) {
      fprintf(stderr, "Cannot allocate memory for split sff file names\n");
      exit(1);
    }
    strcpy(str, template);
    
    char code[4];
    sprintf(code, "%03d", pat_idx+1);
    
    char * p = strchr(str, 'X');
    p[0] = code[0]; 
    p[1] = code[1]; 
    p[2] = code[2];
	
    sff_split_file[pat_idx] = str;
    
    fprintf2_(stderr, "Filename=%s for pat_idx=%d\n", sff_split_file[pat_idx], pat_idx); 

  } // for (pat_idx= 0; ... ) { ...}


} // init_split_file_arrays()  



void 
finalize_file_write ( int pat_idx ) 
{

  //
  // Update the number of reads field in 
  // the common header and close file
  //

  FILE * fp = sff_split_fp[pat_idx];
  sff_common_header ch_l = ch;


  if ( fp == NULL ) {
    fprintf_(stderr, "WARNING: finalize_file_write(%d) invoked with null fp\n", pat_idx);  
    return;
  }
  else if ( nreads_split_file == NULL ) {
    fprintf_(stderr, "WARNING: finalize_file_write(%d) invoked with null nreads array\n", pat_idx);  
    return;
  }


  // Set updated number-of-reads
  ch_l.nreads = nreads_split_file[pat_idx];
  
  // Rewind the file
  fseek(fp, 0L, SEEK_SET);
  
  // Update number-of-reads field in common header
  fprintf_(stderr, "Update common header for split %d\n", pat_idx);  
  write_sff_common_header(fp, &ch_l);  
  
  // Close file
  fprintf_(stderr, "Closing file split %d\n", pat_idx);  
  fclose( fp ); 
  sff_split_fp[pat_idx] = NULL;

} //  finalize_file_write( ) 





