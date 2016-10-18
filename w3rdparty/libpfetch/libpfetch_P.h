/*  File: libpfetch_P.h
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
 * Last edited: Nov 21 09:56 2008 (rds)
 * Created: Fri Apr  4 14:20:41 2008 (rds)
 * CVS info:   $Id: libpfetch_P.h,v 1.2 2009-03-30 11:51:47 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef __LIBPFETCH_P_H__
#define __LIBPFETCH_P_H__

#include <libpfetch/libpfetch.h>

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

#define PFETCH_READ_SIZE 80

typedef struct
{
  guint    watch_id;
  GPid     watch_pid;
  gpointer watch_data;
}ChildWatchDataStruct, *ChildWatchData;


PFetchStatus emit_signal(PFetchHandle handle,
			 guint        signal_id,
			 GQuark       detail,
			 char        *text, 
			 guint       *text_size, 
			 GError      *error);

#endif /* __LIBPFETCH_P_H__ */
