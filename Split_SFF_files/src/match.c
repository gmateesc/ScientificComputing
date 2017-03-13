/*

  Functions to match a pattern against a text 
  that contains the sequence of bases from 
  an SFF file.

  Author

     Gabriel Mateescu  mateescu@acm.org
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "match.h"



//
// Match the pattern passed as argument against the bases 
// present in the read in the rd structure, then 
// insert the read in the appropriate split
//
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
) 
{     

      //
      // 1. Extract from bases subsequence into which 
      //    to look for the pattern
      //
      
      // 1.1 Set left and right bounds
      
      //
      // Match against complete sequence if opt_no_clipping == 1 
      // i.e., if the -c command option is given
      //
      int left = 0;
      int right = rh->nbases; 
      fprintf_m(stderr, "Matching read number %d with pattern %s\n", read_num, pattern);


      //
      // Obey the clipping params if opt_no_clipping == 0 (default)
      //	
      if ( opt_no_clipping == 0 ) {
	
	//
	// Set left
	//
	if ( rh->clip_qual_left > rh->clip_adapter_left ) {
	  // Cannot use clip_qual_left because it may include the adapter
	  // Start matching after the key
	  left = ch->key_len - 1;
	}
	else {
	  // Can use clip_qual_left because it does not include the adapter
	  // Start matching after the quality clipping point
	  left = rh->clip_qual_left - 1;
	}
	
	//
	// Set right
	//
	if ( rh->clip_adapter_left > 0 ) {
	  
	  right = rh->clip_adapter_left - 1;
	  
	}
	else {
	  right = (int) min(
			    (rh->clip_qual_right    == 0 ? rh->nbases : rh->clip_qual_right   ),
			    (rh->clip_adapter_right == 0 ? rh->nbases : rh->clip_adapter_right)
			    );
	}  
      }
      
      
      //
      // 1.2 Extract from the bases sequence the subsequence in which 
      //     to look for the adapter pattern
      //
      char * text = get_read_bases(rd, left, right);
      fprintf_m(stderr, "\t text[%-4d:%-4d] :     %s\n", left,      right-1, text);
      fprintf_m(stderr, "\t pattern         :     %s\n", pattern);
      //fprintf_m(stderr, "\tbases[%-4d:%-4d]  : %s\n",  left_clip, right_clip-1, bases);
      
      
      
      //
      // 2. Match pattern against text
      //
      int pos = match(text, pattern);
      free(text);

      if( pos == -1 ) {
	fprintf_m(stderr, "\tDid NOT find pattern %s in text %s\n", pattern, text);
	return;
      }

      
      //
      // 3. Found a match for this read; so write this read 
      //    to the split file if this is not a dry-run
      //
            
      fprintf_m(stderr, "\tFound pattern %s in text %s at index %d\n", pattern, text, pos);

      nreads_split_file[pat_idx] += 1;
      
      if ( ! dry_run ) {
	
	//
	// 3.1 For the first write, write the common header
	//	
	if ( nreads_split_file[pat_idx] == 1 ) {
	  
	  fprintf_m(stderr, "\nWrite common header for split %d\n", pat_idx);  
	  write_sff_common_header(sff_split_fp[pat_idx], ch);  
	    
	}
	  
	
	//
	// 3.2 Write the read header for this read
	//	
	fprintf_m(stderr, "Write read header for read number %d\n", read_num);   
	write_sff_read_header(sff_split_fp[pat_idx], rh);  
	
	
	//
	// 3.3 Write the data for this read
	//
	fprintf_m(stderr, "Write data for read number %d\n", read_num);	  
	write_sff_read_data(sff_split_fp[pat_idx], rd, ch->flow_len, rh->nbases, read_num);
	
	
      } // if ( ! dry_run ) { ... }


} // match_read_pattern()




//
// Determine whether the text matches the 
// given pattern
//
int match(char text[], char pattern[]) {

  int text_pos, pat_idx, text_idx, text_len, pat_len;

  int rc = -1;
 
  text_len = strlen(text);
  pat_len  = strlen(pattern);
 
  fprintf2_(stderr, "\ttext=%s length=%d\n", text,    text_len);
  fprintf2_(stderr, "\tpatt=%s length=%d\n", pattern, pat_len);

  if (pat_len > text_len) {
    return rc;
  }
 
  //
  // Scan text
  //
  for (text_pos = 0; text_pos <= text_len - pat_len; text_pos++) {

    // Start pos in text
    text_idx = text_pos;

    char * substr = strdup( &text[text_idx] ); 
    fprintf_m(stderr, "\tMatching substring text[%d]=%s with pattern=%s\n", text_idx, substr, pattern);
    free(substr);

 
    // Compare text[text_pos:text_pos+text_len-1] with text[0:text_len-1]
    for (pat_idx = 0; pat_idx < pat_len; pat_idx++) {

      if ( text[text_idx] == pattern[pat_idx] ) {
	fprintf2_(stderr, "match between pat[%d]=text[%d]=%c\n", 
		  pat_idx, text_idx, text[text_idx]);
        text_idx++;
      }
      else {
	fprintf2_(stderr, "NO match: pat[%d]=%c != text[%d]=%c\n", 
		  pat_idx, pattern[pat_idx], text_idx, text[text_idx]);
        break; // try to match with text starting from next text_pos
      }

    }

    // Match found
    if (pat_idx == pat_len) {
      rc = text_pos; 
      break;
    }

  } // for (pat_idx = 0; pat_idx < pat_len; pat_idx++) { ... }
 
  return rc;

} // match()



//
// Extract the list of adapters from the adapter file
// 
int get_patterns ( char * ad_file, char *** ptr ) {

  int num_patterns = 0; 
  char   *buf, *line;
  int    n_lines, start, i;
  size_t ad_file_sz;


  if ( ad_file == NULL ) {
    fprintf(stderr, "[err] No adapter file name given.\n");
    return num_patterns; 
  }
  

  //
  // 1. Read the adapter file into buf
  //
  buf = load_file (ad_file, &ad_file_sz);
  if ( buf == NULL ) {
    fprintf(stderr, "[err] Could not allocate memory to read file '%s'\n", ad_file);
    return num_patterns; 
  }

  fprintf_m(stderr, "\nRead adapter file '%s' with size %d\n", ad_file, (int)ad_file_sz);


  //
  // 2. Process the file contents
  //

  //
  // 2.1 Compute numer of lines in the file
  //
  n_lines = 0;
  start = 0;
  while ( ( line = get_string(buf, &start) ) ) {
    fprintf2_(stderr, "line[%d] = '%s'\n", n_lines, line); 
    n_lines++;
  }
  fprintf_m(stderr, "  File %s has %d lines\n", ad_file, n_lines);


  //
  // 2.2 Allocate space for patterns
  //
  char ** patterns = malloc ( sizeof(char *) * n_lines);
  *ptr = patterns;


  //
  // 2.3 If the line has an adapter, extract it
  //
  start = 0;

  while ( ( line = get_string(buf, &start) ) ) {

    //
    // 2.3.1 Extract adapter from line
    //
    char * pos = get_adapter(line);

    //
    // 2.3.2 This line has an adapter
    //
    if ( pos ) {
        
      if ( ! num_patterns ) {
	fprintf_m(stderr, "  %-4s  %-14s  %s\n", "i", "adapter", "line");
      }
      fprintf_m(stderr,   "  %-4d  %-14s  %s\n", num_patterns, pos, line ); 
      
      patterns[num_patterns] = strdup(pos);
      
      num_patterns++;
    }

    free (line);

  }

  fflush(stderr);


  /*
  char * patterns[] = {
    "AAGAGGATTC",      // IonXpress_003
    "CTAAGGTAAC",      // IonXpress_001
    "TACCAAGATC"       // IonXpress_004
  };
  */

  free(buf);

  return num_patterns;

} // get_patterns()




//
// Extract the adapter from a line in the adapter file
//

char * get_adapter ( char * line ) {

  char adapter_key[] = "IonXpress_";
      
  // 2.3.1 Look for key in line
  char * pos = strstr(line, adapter_key);
  if ( !pos ) {
    return pos;
  }
  // look for blank
  while ( (*pos) && ! isblank(*pos) ) {
    pos++;
  }
  // skip blanks
  while ( (*pos) && isblank(*pos) ) {
    pos++;
  }

  // if on null delimiter, did not find the adapter
  if ( (*pos) == '\0' ) {
    pos = NULL;
  }

  return pos;

} // get_adapter






//
// Get a record (line) from the file buffer
//
char *get_string (void *buf, int * start) 
{

    char *l_buf = buf;
    int in = *start;
    char *string = NULL;

    int i, j, num;

    //fprintf_m(stderr, "ENTER getString\n");
    
    //
    // Find line's start index i
    //
    for (i = in;  l_buf[i] != '\0' && (l_buf[i] == '\n' || l_buf[i] == '\r'); i++);
    if (l_buf[i] == '\0') {
        /* Never saw the start of a line before the buffer ran out */
        return NULL;
    }


    //
    // Find line's end index j
    //
    for (j = i; l_buf[i] != '\0' && l_buf[j] != '\n' && l_buf[j] != '\r'; j++);
    if (i == j) {
        return NULL;
    }


    // Allocate mem for line 
    num = j - i;
    string = malloc (sizeof(char) * (num + 1));
    if (!string) 
        return NULL;


    // Extract line from buf
    strncpy (string, &l_buf[i], num);
    string[num] = '\0';


    // Update start index of next line
    *start = j;

    return string;

} // get_string()




//
// Read a file into a buffer
//
void *load_file (char *ad_file, size_t * file_size) 
{

    size_t  size;
    FILE *  fp;

    if ( ad_file == NULL ) {
      fprintf(stderr, "[err] No file name given.\n");
      exit(1);
    }
    
    if ( (fp = fopen(ad_file, "r")) == NULL ) {
      fprintf(stderr, "[err] Could not open file '%s' for reading.\n", ad_file);
      exit(1);
    }
    
    // Get the file size
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET); 


    // Allocate buffer for file
    char *buf = malloc (sizeof(char) * (size+1));
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    // Read file into buffer
    size_t size_actual = fread (buf, sizeof(char), size, fp) ;
    *file_size = size_actual;

    if ( size_actual != size ) {
        fprintf(stderr, "[err] Could not read the file '%s'\n", ad_file);
        free (buf);
        buf = NULL;
    }
    else {
        // Add marker for buffer end      
        buf[size] = '\0';
    }

    fclose(fp);

    return buf;

} // load_file()



