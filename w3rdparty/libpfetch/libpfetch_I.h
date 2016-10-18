/*  File: libpfetch_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 18 13:22 2009 (edgrif)
 * Created: Fri Apr  4 14:20:41 2008 (rds)
 * CVS info:   $Id: libpfetch_I.h,v 1.3 2009-06-19 08:06:21 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef __LIBPFETCH_I_H__
#define __LIBPFETCH_I_H__

#include <libpfetch/libpfetch.h>

#define PFETCH_PARAM_STATIC (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#define PFETCH_PARAM_STATIC_RW (PFETCH_PARAM_STATIC | G_PARAM_READWRITE)
#define PFETCH_PARAM_STATIC_RO (PFETCH_PARAM_STATIC | G_PARAM_READABLE)

typedef enum
  {
    PFETCH_PIPE,
    PFETCH_HTTP
  }PFetchMethod;

enum
  {
    PROP_0,			/* Zero is invalid property id! */
    PFETCH_FULL,
    PFETCH_ARCHIVE,
    PFETCH_DEBUG,
    PFETCH_LOCATION,
    PFETCH_PORT,
    PFETCH_UNIPROT_ISOFORM_SEQ,
    PFETCH_ONE_SEQ_PER_LINE,
    PFETCH_DNA_LOWER_AA_UPPER,
    PFETCH_BLIXEM_STYLE,
    /* http specific stuff */
    PFETCH_COOKIE_JAR,
    PFETCH_URL,			/* same as location */
    PFETCH_POST,
    PFETCH_WRITE_FUNC,
    PFETCH_WRITE_DATA,
    PFETCH_HEADER_FUNC,
    PFETCH_HEADER_DATA,
    PFETCH_READ_FUNC,
    PFETCH_READ_DATA,
    PFETCH_TRANSFER_CHUNKED
  };


enum
  {
    HANDLE_READER_SIGNAL,
    HANDLE_ERROR_SIGNAL,
    HANDLE_WRITER_SIGNAL,
    HANDLE_CLOSED_SIGNAL,
    HANDLE_LAST_SIGNAL
  };

/* 

        PFETCH SERVER - retrieve entries from sequence databases.

        Usage:
            pfetch [options] <query> [<query2>...]
            pfetch <query:start-end> [<query2:start-end>...]

        eg. To retrieve EMBL clone AP000869 and UNIPROT protein DYNA_CHICK:
            pfetch AP000869 DYNA_CHICK

        eg. To retrieve the length of AP000869 and DYNA_CHICK:
            pfetch -l AP000869 DYNA_CHICK

        Ensembl Usage:
            pfetch [options] [--repeatmask] --asm <asm_name> <chr_name> -s <start_bp> -e <end_bp>
            pfetch --listasm

        eg. To retrieve human chromosome 10, bp 50001-60000 for assembly NCBI35:
            pfetch --asm NCBI35 10 -s 50001 -e 60000

        Options:
        -a <#>        Search EMBL and UniProtKB archive, including current data
                      (Release # optional; otherwise returns latest sequence version)
        -A            Search EMBL and UniProtKB archive, including current data
                      (returns all sequence versions, if no sequence version is given)
        -d            Search only one database
        -s <#>        Return sequence starting at position <#>
        -e <#>        Return sequence ending at position <#>
        -l            Return the length of the sequence only
        -r            Reverse-complement returned sequence (DNA only)
        -F            Return full database entry
        -i            Start interactive session
        -q            Sequence only output (one line)
        -n <name>     Use <name> for output sequence
        -C            Return DNA in lower case, protein in upper case
        -D            Return description (DE) line only
        -Q            Return base quality values (not all databases supported!)
        -t            Search only trace repository
        -S            Show server status
        -v            Show verbose output
        -V            Show version
        -H|h          Show this help message

        --pdb         pdb file retrieval  (old file format, pre 01-Aug-07)
        --dssp        dssp calculations   (based on old file format, pre 01-Aug-07)
        --old_pdb     pdb file retrieval  (old file format, pre 01-Aug-07)
        --old_dssp    dssp calculations   (based on old file format, pre 01-Aug-07)
        --new_pdb     pdb file retrieval
        --new_dssp    dssp calculations

        --md5          Return the MD5 hexdigest of sequence (excludes descr. line)
        --ends         Return first and last 500bp of sequence
        --head <#>     Return first <#>bp of sequence
        --tail <#>     Return last <#>bp of sequence

        --listasm      Show available Ensembl assemblies
        --repeatmask   Return repeat masked sequence (Ensembl queries only)

        use "-t -H" and "-T -H" to see the trace archive and trace internal repository
        specific options, respectively.

        by Tony Cox (avc@sanger.ac.uk)
        Version 5.12, 14-Nov-2007

 */

/* BASE */

typedef struct _pfetchHandleStruct
{
  GObject __parent__;

  char *location;

  struct
  {
    unsigned int full    : 1;	/* -F */
    unsigned int archive : 1;	/* -A */
    unsigned int debug   : 1;	/* internal debug */
    unsigned int isoform_seq : 1; /* internal option */
    unsigned int one_per_line : 1; /* -q */
    unsigned int dna_PROTEIN : 1; /* -C */
  }opts;

} pfetchHandleStruct;


typedef struct _pfetchHandleClassStruct
{
  GObjectClass parent_class;

  /* methods */
  PFetchStatus (* fetch)(PFetchHandle handle, char *sequence);

  /* signals */
  PFetchStatus (* reader)(PFetchHandle handle, char *output, guint *output_size, GError *error);

  PFetchStatus (* error) (PFetchHandle handle, char *output, guint *output_size, GError *error);

  PFetchStatus (* writer)(PFetchHandle handle, char *input,  guint *input_size,  GError *error);

  PFetchStatus (* closed)(void);

  /* signal ids... should they be here? */
  guint handle_signals[HANDLE_LAST_SIGNAL];

} pfetchHandleClassStruct;


/* PIPE */

typedef struct _pfetchHandlePipeStruct
{
  pfetchHandle __parent__;

  guint stdin_source_id;
  guint stdout_source_id;
  guint stderr_source_id;
  guint timeout_source_id;

  ChildWatchDataStruct watch_data;
} pfetchHandlePipeStruct;

typedef struct _pfetchHandlePipeClassStruct
{
  pfetchHandleClass parent_class;

} pfetchHandlePipeClassStruct;


/* HTTP */

typedef struct _pfetchHandleHttpStruct
{
  pfetchHandle __parent__;

  CURLObject curl_object;

  char *post_data;
  char *cookie_jar_location;
  unsigned int http_port;

  unsigned int request_counter;
} pfetchHandleHttpStruct;

typedef struct _pfetchHandleHttpClassStruct
{
  pfetchHandleClass parent_class;

} pfetchHandleHttpClassStruct;


#endif /* __LIBPFETCH_I_H__ */
