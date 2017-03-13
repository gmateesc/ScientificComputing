/*

  Reading and writing SFF files. Author

     Gabriel Mateescu  mateescu@acm.org

 The reading part is based on the code of 

     Indraniel Das <indraniel@gmail.com>

     https://github.com/indraniel/sff2fastq

*/

/** INCLUDES **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sff.h"



/** FUNCTIONS **/

void 
read_sff_common_header(FILE *fp, sff_common_header *h) {


    int header_size;
    size_t actual;


    //
    // 1. Read from SFF the header chunk that does not require allocating memory
    //
    actual = fread(&(h->magic)          , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the magic number", 1);
    }
    actual = fread(h->version           , sizeof(char)    , 4, fp);
    if ( actual != 4 ) {
        bailout(fp, "Could not read the version", 1);
    }
    actual = fread(&(h->index_offset)   , sizeof(uint64_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the index offset", 1);
    }

    actual = fread(&(h->index_len)      , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the index length", 1);
    }

    actual = fread(&(h->nreads)         , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the number of reads", 1);
    }

    actual = fread(&(h->header_len)     , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the header length", 1);
    }

    actual = fread(&(h->key_len)        , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the key length", 1);
    }

    actual = fread(&(h->flow_len)       , sizeof(uint16_t), 1, fp); // number of flows per read
    if ( actual != 1 ) {
        bailout(fp, "Could not read the flow length", 1);
    }

    actual = fread(&(h->flowgram_format), sizeof(uint8_t) , 1, fp); // format code
    if ( actual != 1 ) {
        bailout(fp, "Could not read the flowgram format code", 1);
    }


    //
    // sff files are in big endian notation, so flip bytes 
    // for integral types before writing
    //
    convert_big_endian_common_header_2_host(h);

    // Magic number after conversion to host-order 
    // must be = 779314790 i.e., 0x2e736666 = SFF_MAGIC
    // and it is defined in sff.h 
    //   #define SFF_MAGIC  0x2e736666 



    //
    // 2. Allocate memory for the flow and key parts of the header
    //

    /* allocate and read the flow and key strings */

    h->flow = (char *) malloc( h->flow_len * sizeof(char) );
    if (! h->flow) {
        bailout(fp, "Out of memory! Could not allocate header flow string", 1);
    }

    h->key = (char *) malloc( h->key_len * sizeof(char) );
    if (!h->key) {
        bailout(fp, "Out of memory! Could not allocate header key string", 1);
    }


    //
    // 3. Read the flow and key parts of the header from SFF
    //
    actual = fread(h->flow, sizeof(char), h->flow_len, fp);
    if ( actual != h->flow_len ) {
        bailout(fp, "Could not read the flow chars", 1);
    }

    actual = fread(h->key,  sizeof(char), h->key_len,  fp);
    if ( actual != h->key_len ) {
        bailout(fp, "Could not read the key sequence", 1);
    }


    /* the common header section should be a multiple of 8-bytes 
       if the header is not, it is zero-byte padded to make it so */

    header_size = sizeof(h->magic)
                  + sizeof(*(h->version))*4
                  + sizeof(h->index_offset)
                  + sizeof(h->index_len)
                  + sizeof(h->nreads)
                  + sizeof(h->header_len)
                  + sizeof(h->key_len)
                  + sizeof(h->flow_len)
                  + sizeof(h->flowgram_format)
                  + (sizeof(char) * h->flow_len)
                  + (sizeof(char) * h->key_len);

    //
    // The common header section must be a multiple of 8-bytes 
    // Use zero-byte padding to ensure this
    //
    if ( !(header_size % PADDING_SIZE == 0) ) {
	int remainder = PADDING_SIZE - (header_size % PADDING_SIZE);
        fprintf_s(stderr, "\nRead common hdr of size %d bytes (padding size %d)\n\n", 
		        header_size + remainder, remainder);
        read_padding(fp, header_size);
    }
    else {
        fprintf_s(stderr, "\nRead common hdr of size %d bytes (no padding)\n\n", header_size);
    }

} // read_sff_common_header(...) 




void 
write_sff_common_header(FILE *fp, sff_common_header *h) {

    size_t actual;

    // Make copy of common header into which to apply big endian transformation 
    sff_common_header hl = *h;

    // Compute size in bytes of the common header
    int header_size = sizeof(hl.magic)
                  + sizeof(*(hl.version))*4
                  + sizeof(hl.index_offset)
                  + sizeof(hl.index_len)
                  + sizeof(hl.nreads)
                  + sizeof(hl.header_len)
                  + sizeof(hl.key_len)
                  + sizeof(hl.flow_len)
                  + sizeof(hl.flowgram_format)
                  + (sizeof(char) * hl.flow_len)
                  + (sizeof(char) * hl.key_len);

    //
    // 1. sff f<iles are in big endian notation, so flip bytes 
    //    for integral types before writing
    //
    convert_big_endian_common_header_2_host(&hl);



    //
    // 2. Write to SFF the header chunk that does not require allocating memory
    //
    actual = fwrite(&(hl.magic)          , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the magic number", 1);
    }
    actual = fwrite(hl.version           , sizeof(char)    , 4, fp);
    if ( actual != 4 ) {
        bailout(fp, "Could not write the version", 1);
    }

    actual = fwrite(&(hl.index_offset)   , sizeof(uint64_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the index offset", 1);
    }

    actual = fwrite(&(hl.index_len)      , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the index length", 1);
    }

    actual = fwrite(&(hl.nreads)         , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the number of reads", 1);
    }

    actual = fwrite(&(hl.header_len)     , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the header length", 1);
    }

    actual = fwrite(&(hl.key_len)        , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the key length", 1);
    }

    actual = fwrite(&(hl.flow_len)       , sizeof(uint16_t), 1, fp); // number of flows per read
    if ( actual != 1 ) {
        bailout(fp, "Could not write the flow length", 1);
    }

    actual = fwrite(&(hl.flowgram_format), sizeof(uint8_t) , 1, fp); // format code
    if ( actual != 1 ) {
        bailout(fp, "Could not write the flowgram format code", 1);
    }


    //
    // 3. Write to SFF the flow and key parts of the common header
    //

    actual = fwrite(hl.flow, sizeof(char), h->flow_len, fp);
    if ( actual != h->flow_len ) {
        bailout(fp, "Could not write the flow characters", 1);
    }

    actual = fwrite(hl.key , sizeof(char), h->key_len,  fp);
    if ( actual != h->key_len ) {
        bailout(fp, "Could not write the key sequence", 1);
    }


    //
    // 4. Write padding, if needed
    //

    // The common header section must be a multiple of 8-bytes 
    // Use zero-byte padding to ensure this
    if ( !(header_size % PADDING_SIZE == 0) ) {
	int remainder = PADDING_SIZE - (header_size % PADDING_SIZE);
        fprintf_s(stderr, "\nWrite common hdr of size %d bytes (padding size %d)\n\n", 
		        header_size + remainder, remainder);
        write_padding(fp, header_size);
    }
    else {
        fprintf_s(stderr, "\nWrite common hdr of size %d bytes (no padding)\n\n", header_size);
    }

    fflush(fp);


} // write_sff_common_header(...) 




void convert_big_endian_common_header_2_host(sff_common_header *h) 
{
    h->magic        = htobe32(h->magic);
    h->index_offset = htobe64(h->index_offset);
    h->index_len    = htobe32(h->index_len);
    h->nreads       = htobe32(h->nreads);
    h->header_len   = htobe16(h->header_len);
    h->key_len      = htobe16(h->key_len);
    h->flow_len     = htobe16(h->flow_len);
}




void
read_padding(FILE *fp, int header_size) {
    int remainder = PADDING_SIZE - (header_size % PADDING_SIZE);
    uint8_t padding[remainder];
    fread(padding, sizeof(uint8_t), remainder, fp);
}



void
write_padding(FILE *fp, int header_size) {
    int remainder = PADDING_SIZE - (header_size % PADDING_SIZE);
    uint8_t padding[remainder]; 
    memset(padding, '\0', remainder * sizeof(uint8_t));
    fwrite(padding, sizeof(uint8_t), remainder, fp);
}



void
free_sff_common_header(sff_common_header *h) {
    free(h->flow);
    free(h->key);
}



void 
verify_sff_common_header(char *prg_name, 
                         char *prg_version, 
                         sff_common_header *h) {

    /* ensure that the magic file type is valid */
    if (h->magic != SFF_MAGIC) {
        fprintf(stderr, "The SFF header has magic value '%d' \n", h->magic);
        fprintf(stderr,
                "[err] %s (version %s) %s : '%d' \n", 
                prg_name, 
                prg_version, 
                "only knows how to deal an SFF magic value of type",
                SFF_MAGIC);
        free_sff_common_header(h);
        exit(2);
    }

    /* ensure that the version header is valid */
    if ( memcmp(h->version, SFF_VERSION, SFF_VERSION_LENGTH) ) {
        fprintf(stderr, "The SFF file has header version: ");
        int i;
        char *sff_header_version = h->version;
        for (i=0; i < SFF_VERSION_LENGTH; i++) {
            printf("0x%02x ", sff_header_version[i]);
        }
        printf("\n");
        fprintf(stderr,
                "[err] %s (version %s) %s : ", 
                prg_name, 
                prg_version, 
                "only knows how to deal an SFF header version: ");
        char valid_header_version[SFF_VERSION_LENGTH] = SFF_VERSION;
        for (i=0; i < SFF_VERSION_LENGTH; i++) {
            printf("0x%02x ", valid_header_version[i]);
        }
        free_sff_common_header(h);
        exit(2);
    }
}



void
read_sff_read_header(FILE *fp, sff_read_header *rh) {

    int header_size;
    size_t actual;

    actual = fread(&(rh->header_len)        , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the header length", 1);
    }

    actual = fread(&(rh->name_len)          , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the name length", 1);
    }

    actual = fread(&(rh->nbases)            , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read the number of bases", 1);
    }

    actual = fread(&(rh->clip_qual_left)    , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read clip_qual_left", 1);
    }

    actual = fread(&(rh->clip_qual_right)   , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read clip_qual_right", 1);
    }

    actual = fread(&(rh->clip_adapter_left) , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read clip_adapter_left", 1);
    }

    actual = fread(&(rh->clip_adapter_right), sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not read clip_adapter_right", 1);
    }


    //
    // sff files are in big endian notation, so flip bytes 
    // for integral types before writing
    //
    convert_big_endian_read_header_2_host(rh);

    /* finally appropriately allocate and read the read_name string */
    rh->name = (char *) malloc( rh->name_len * sizeof(char) );
    if ( ! rh->name ) {
        bailout(fp,  "Out of memory! Could not allocate header name string", 1);
    }

    actual = fread(rh->name, sizeof(char), rh->name_len, fp);
    if ( actual != rh->name_len ) {
        bailout(fp, "Could not read the name string", 1);
    }

    header_size = sizeof(rh->header_len)
                  + sizeof(rh->name_len)
                  + sizeof(rh->nbases)
                  + sizeof(rh->clip_qual_left)
                  + sizeof(rh->clip_qual_right)
                  + sizeof(rh->clip_adapter_left)
                  + sizeof(rh->clip_adapter_right)
                  + (sizeof(char) * rh->name_len);

    //
    // The read header section must be a multiple of 8-bytes 
    // Use zero-byte padding to ensure this
    //
    if ( !(header_size % PADDING_SIZE == 0) ) {
	int remainder = PADDING_SIZE - (header_size % PADDING_SIZE);
        fprintf_s(stderr, "\nRead hdr of size %d bytes (padding size %d)\n\n", 
		        header_size + remainder, remainder);
        read_padding(fp, header_size);
    }
    else {
        fprintf_s(stderr, "\nRead hdr of size %d bytes (no padding)\n\n", header_size);
    }

} // read_sff_read_header(..) 




void
write_sff_read_header(FILE *fp, sff_read_header *rh) {

    size_t actual;
    sff_read_header rhl = *rh;


    //
    // 1. Compute size in bytes of the common header
    //
    int header_size = sizeof(rhl.header_len)
                  + sizeof(rhl.name_len)
                  + sizeof(rhl.nbases)
                  + sizeof(rhl.clip_qual_left)
                  + sizeof(rhl.clip_qual_right)
                  + sizeof(rhl.clip_adapter_left)
                  + sizeof(rhl.clip_adapter_right)
                  + (sizeof(char) * rhl.name_len);


    //
    // 2. sff files are in big endian notation, so flip bytes 
    //    for integral types before writing
    //
    convert_big_endian_read_header_2_host(&rhl);


    //
    // 3. Write the read-header
    //
    actual = fwrite(&(rhl.header_len)        , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the header_len", 1);
    }

    actual = fwrite(&(rhl.name_len)          , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the name_len", 1);
    }

    actual = fwrite(&(rhl.nbases)            , sizeof(uint32_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write the number of bases ", 1);
    }

    actual = fwrite(&(rhl.clip_qual_left)    , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write clip_qual_left", 1);
    }

    actual = fwrite(&(rhl.clip_qual_right)   , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write clip_qual_right", 1);
    }

    actual = fwrite(&(rhl.clip_adapter_left) , sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write clip_adapter_left", 1);
    }

    fwrite(&(rhl.clip_adapter_right), sizeof(uint16_t), 1, fp);
    if ( actual != 1 ) {
        bailout(fp, "Could not write clip_adapter_right", 1);
    }

    actual = fwrite(rhl.name                 , sizeof(char), rh->name_len, fp);
    if ( actual !=  rh->name_len ) {
        bailout(fp, "Could not write the read name", 1);
    }
    

    //
    // 4. The read header section must be a multiple of 8-bytes 
    //    Use zero-byte padding to ensure this
    //
    if ( !(header_size % PADDING_SIZE == 0) ) {
	int remainder = PADDING_SIZE - (header_size % PADDING_SIZE);
        fprintf_s(stderr, "\nWrite hdr of size %d bytes (padding size %d)\n\n", 
		        header_size + remainder, remainder);
        write_padding(fp, header_size);
    }
    else {
        fprintf_s(stderr, "\nWrite hdr of size %d bytes (no padding)\n\n", header_size);
    }

    fflush(fp);

} // write_sff_read_header(...) 



void 
convert_big_endian_read_header_2_host(sff_read_header *rh) 
{
    rh->header_len         = htobe16(rh->header_len);
    rh->name_len           = htobe16(rh->name_len);
    rh->nbases             = htobe32(rh->nbases);
    rh->clip_qual_left     = htobe16(rh->clip_qual_left);
    rh->clip_qual_right    = htobe16(rh->clip_qual_right);
    rh->clip_adapter_left  = htobe16(rh->clip_adapter_left);
    rh->clip_adapter_right = htobe16(rh->clip_adapter_right);

}



void
free_sff_read_header(sff_read_header *h) 
{
    free(h->name);
}




void
read_sff_read_data(FILE *fp, 
                   sff_read_data *rd, 
                   uint16_t nflows, 
                   uint32_t nbases, 
		   uint32_t read_num
		   ) 
{

    int data_size;
    register int i;
    size_t actual;


    //
    // 1. Allocate the flowgram, flow-index, bases, and quality arrays 
    //

    rd->flowgram = (uint16_t *) malloc( nflows * sizeof(uint16_t) );
    if ( ! rd->flowgram ) {
        bailout(fp, "Out of memory! Could not allocate for a read flowgram", 1);
    }

    rd->flow_index = (uint8_t *) malloc( nbases * sizeof(uint8_t)  );
    if ( ! rd->flow_index ) {
        bailout(fp, "Out of memory! Could not allocate for a read flow index", 1);
    }

    rd->bases = (char * ) malloc( nbases * sizeof(char) );
    if ( ! rd->bases ) {
        bailout(fp, "Out of memory! Could not allocate for the bases of a read", 1);
    }

    rd->quality = (uint8_t *) malloc( nbases * sizeof(uint8_t) );
    if (! rd->quality ) {
        bailout(fp, "Out of memory! Could not allocate for the read quality string", 1);
    }


    //
    // 2. Read data section from sff file into rd sff_read_data struct
    //

    //
    // 2.1 Read the array of flowgram values (one flowgram per flow)
    //

    actual = fread(rd->flowgram, sizeof(uint16_t), (size_t) nflows, fp);
    if ( actual != nflows ) {
        bailout(fp, "Could not read the flowgrams", 1);
    }

    /* sff files are in big endian notation so adjust appropriately */
    for (i = 0; i < nflows; i++) {
        rd->flowgram[i] = htobe16( rd->flowgram[i] );
	//if ( i < 16 ) {
	//  fprintf(stderr, "\tR_%-10d flow=%8d be_flow=%8d\n", 
	//	  read_num, rd->flowgram[i], htobe16(rd->flowgram[i]));
	//}
    }


    //
    // 2.2 Read the array of flow indexes (one flow index per base)
    //
    actual = fread(rd->flow_index, sizeof(uint8_t), (size_t) nbases, fp);
    if ( actual != nbases ) {
        bailout(fp, "Could not read the flow index", 1);
    }


    //
    // 2.3 Read the array of bases
    //
    actual = fread(rd->bases, sizeof(char), (size_t) nbases, fp);
    if ( actual != nbases ) {
        bailout(fp,  "Could not read the flow index", 1);
    }


    //
    // 2.4 Read the array of quality scores
    //
    actual = fread(rd->quality, sizeof(uint8_t), (size_t) nbases, fp);
    if ( actual != nbases ) {
        bailout(fp,   "Could not read the quality", 1);
    }



    //
    // 3. The read data section must be a multiple of 8-bytes 
    //    Use zero-byte padding to ensure this
    //

    data_size = (sizeof(uint16_t) * nflows)    // flowgram size
                + (sizeof(uint8_t) * nbases)   // flow_index size
                + (sizeof(char) * nbases)      // bases size
                + (sizeof(uint8_t) * nbases);  // quality size

    if ( !(data_size % PADDING_SIZE == 0) ) {
	int remainder = PADDING_SIZE - (data_size % PADDING_SIZE);
        fprintf_s(stderr, "\nRead data section of size %d bytes (padding size %d)\n\n", 
		        data_size + remainder, remainder);
        read_padding(fp, data_size);
    }
    else {
        fprintf_s(stderr, "\nRead data section of size %d bytes (no padding)\n\n", data_size);
    }


} // read_sff_read_data()





void
write_sff_read_data(FILE *fp, 
                   sff_read_data *rd, 
                   uint16_t nflows, 
		   uint32_t nbases, 
		   uint32_t read_num
		   ) 
{

    int data_size;
    register int i;
    size_t actual;

    //
    // Write data section from rd sff_read_data struct into sff file 
    //

    //
    // 1. Write the array of flowgram values (one flowgram per flow)
    //

    uint16_t * flowgram = (uint16_t *) malloc( nflows * sizeof(uint16_t) );
    if ( ! flowgram ) {
        bailout(fp, "Out of memory! Could not allocate flowgram chars for writing", 1);
    }

    // sff files are in big endian notation so adjust appropriately 
    for (i = 0; i < nflows; i++) {

        flowgram[i] = htobe16( rd->flowgram[i] );

	//if ( i < 16) {
	//  fprintf(stderr, "\tW_%-10d flow=%8d be_flow=%8d\n", 
	//	  read_num, rd->flowgram[i], flowgram[i]);
	//}
    }

    actual = fwrite(flowgram, sizeof(uint16_t), (size_t) nflows, fp);
    if ( actual != nflows ) {
        fprintf(stderr, "Could not write all %d flows; only wrote %d\n", nflows, (int)actual);
        exit(1);
    }


    //
    // 2. Write the array of flow indexes (one flow index per base)
    //
    actual = fwrite(rd->flow_index, sizeof(uint8_t), (size_t) nbases, fp);
    if ( actual != nbases ) {    
        bailout(fp, "Could not write the flow index", 1);
    }

    //
    // 3. Write the array of bases
    //
    actual = fwrite(rd->bases, sizeof(char), (size_t) nbases, fp);
    if ( actual != nbases ) {    
        bailout(fp, "Could not write the bases", 1);
    }


    //
    // 4. Write the array of quality scores
    //
    actual = fwrite(rd->quality, sizeof(uint8_t), (size_t) nbases, fp);
    if ( actual != nbases ) {    
        bailout(fp, "Could not write the quality", 1);
    }


    //
    // 5. The read data section must be a multiple of 8-bytes 
    //    Use zero-byte padding to ensure this
    //

    data_size = (sizeof(uint16_t) * nflows)    // flowgram size
                + (sizeof(uint8_t) * nbases)   // flow_index size
                + (sizeof(char) * nbases)      // bases size
                + (sizeof(uint8_t) * nbases);  // quality size

    if ( !(data_size % PADDING_SIZE == 0) ) {
	int remainder = PADDING_SIZE - (data_size % PADDING_SIZE);
        fprintf_s(stderr, "\nWrite data section of size %d bytes (padding size %d)\n\n", 
		        data_size + remainder, remainder);
        write_padding(fp, data_size);
    }
    else {
        fprintf_s(stderr, "\nWrite data section of size %d bytes (no padding)\n\n", data_size);
    }


    fflush(fp);


} // write_sff_read_data()




void
free_sff_read_data(sff_read_data *d) {
    free(d->flowgram);
    free(d->flow_index);
    free(d->bases);
    free(d->quality);
}



void
free_fastq(struct_fastq *fq) {
    free(fq->name);
    free(fq->bases);
    free(fq->quality);
}


/* as described in section 13.3.8.2 "Read Header Section" in the
   454 Genome Sequencer Data Analysis Software Manual
   see (http://sequence.otago.ac.nz/download/GS_FLX_Software_Manual.pdf) */

void
get_clip_values(sff_read_header rh,
                int trim_flag,
                int *left_clip,
                int *right_clip) {
    if (trim_flag) {
        (*left_clip)  =
            (int) max(1, max(rh.clip_qual_left, rh.clip_adapter_left));

        // account for the 1-based index value
        *left_clip = *left_clip - 1;

        (*right_clip) = (int) min(
              (rh.clip_qual_right    == 0 ? rh.nbases : rh.clip_qual_right   ),
              (rh.clip_adapter_right == 0 ? rh.nbases : rh.clip_adapter_right)
        );
    }
    else {
        (*left_clip)  = 0;
        (*right_clip) = (int) rh.nbases;
    }
}

char*
get_read_bases(
	       sff_read_data * rd,
               int left_clip,
               int right_clip
	       ) 
{
    char *bases;

    // account for NULL termination
    int bases_length = (right_clip - left_clip) + 1;

    // inititalize the bases string/array
    bases = (char *) malloc( bases_length * sizeof(char) );
    if (!bases) {
        fprintf(stderr, "Out of memory! For read bases string!\n");
        exit(1);
    }
    memset(bases, '\0', (size_t) bases_length);

    // copy the relative substring
    int start = left_clip;
    int stop  = right_clip;
    int i, j = 0;

    for (i = start; i < stop; i++) {
        *(bases + j) = *(rd->bases + i);
        j++;
    }

    return bases;
}

uint8_t*
get_read_quality_values(sff_read_data rd,
                        int left_clip,
                        int right_clip) {
    uint8_t *quality;

    // account for NULL termination
    int quality_length = (right_clip - left_clip) + 1;

    // inititalize the quality array
    quality = (uint8_t *) malloc( quality_length * sizeof(uint8_t) );
    if (!quality) {
        fprintf(stderr, "Out of memory! For read quality array!\n");
        exit(1);
    }
    memset(quality, '\0', (size_t) quality_length);

    // copy the relative substring
    int start = left_clip;
    int stop  = right_clip;
    int i, j = 0;

    for (i = start; i < stop; i++) {
        *(quality + j) = *(rd.quality + i);
        j++;
    }

    return quality;
}



void 
bailout(FILE *fp, char * msg, int err) {

  fclose(fp);
  fprintf(stderr, "%s\n", msg);
  exit(err);

}

