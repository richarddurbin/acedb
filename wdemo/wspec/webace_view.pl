# view preferences for humace and chr acedb db's at sanger
# Tim Hubbard, 13/2/97
# $Id: webace_view.pl,v 1.1 2001-01-17 14:31:31 edgrif Exp $

# This can be either a database specific or global file affecting 
# behaviour of 'display' CGI script.

# Description of variables
# things that you can edit are preceeded by '# EDITABLE'
#
# $use_fmap_view and $use_gmap_view are flags
# - set either of these to =0 if you want to disable that view control
#
# @fmap_view, %fmap_view are parameters used by CGI.pm popup_menu 
# call and define the available options for the user.
# - edit these to define list of views available
# [these are defined inside fmap_view or gmap_view]
#
# %fmap_head specifies the header to be displayed for each view 
# and @fmap_head specifies the html for that header (similarly
# for %fmap_key and @fmap_key).  These must be defined in the subroutine
# $gmap_view and $fmap_view as the relevant one only is return depending
# on the view selected.
#
# for gmap, the mapping between webace viewnames and acedb viewnames is 
# provided by %gmap_acedb_viewcom (a global)
#
# for fmap, the sets of elements of a view are defined by 
# %fmap_col, which is indexed by %fmap_col_index.
#

# A. fmap:
$use_fmap_view=1;

# default = as in acedb by default
# def     = assembly_tags off (default view we want)
# hom     = at, repeats, genepred off
# gene    = at, repeats, genepred, homol off

# 1. subroutine rule for semantic view
$fmap_view=sub{
    my($rview,$object,$rafmap_view,$rhfmap_view,
       $rfmap_head,$rfmap_key)=@_;
    my($lenview,%fmap_head,@fmap_head,
       %fmap_key,@fmap_key,$view2);

# EDITABLE
    @$rafmap_view=(
		   'auto',
		   'def',
		   'hom',
		   'ggp',
		   'gene',
		   );
# EDITABLE
    %$rhfmap_view=(
		   'auto','Auto Select',
		   'def','All',
		   'hom','Genes and Homology',
		   'ggp','Genes and Predictions',
		   'gene','Genes Only',
		   );
# EDITABLE
    %fmap_head=(
		'auto',1,
		'def',1,
		'hom',1,
		'ggp',1,
		'gene',1,
		);
# EDITABLE
    @fmap_head[1]='';
# EDITABLE
    %fmap_key=(
	       'auto',1,
	       'def',1,
	       'hom',1,
	       'ggp',2,
	       'gene',1,
	       );
# EDITABLE
#    @fmap_key[1]=<<ENDOFTEXT;
#Feature colour key: 
#<img src="/webace/I/darkgreen.gif"> Genes for which there is supporting evidence
#<img src="/webace/I/darkred.gif"> mRNA<p>
#ENDOFTEXT
# EDITABLE
#@fmap_key[2]=<<ENDOFTEXT;
#Feature colour key: 
#<img src="/webace/I/darkgreen.gif"> Genes for which there is supporting evidence
#<img src="/webace/I/darkred.gif"> mRNA
#<img src="/webace/I/blue.gif"> Ab initio gene predictions<p>
ENDOFTEXT

# EDITABLE to define semantic rules
    if($begin && $end){
	$lenview=$end-$begin;
	if($lenview>40000){
#	    $view2='gene';
	    $view2='ggp';
	}elsif($lenview>4000){
	    $view2='hom';
	}else{
	    $view2='def';
	}
    }else{

# if no begin or end, assume clone -> minimal view
#	$view2='gene';
	$view2='ggp';
    }

# check view exists or default
    unless($$rhfmap_view{$$rview}){
	$$rview='def';
    }
# use defined here only if auto
    if($$rview ne 'auto'){
	$view2=$$rview;
    }
# select head/key
    $$rfmap_head=$fmap_head[$fmap_head{$view2}];
    $$rfmap_key=$fmap_key[$fmap_key{$view2}];
    return $view2;

};

# 2. set of columns for views

# EDITABLE
%fmap_col_index=(
		 'default',0,
		 'def',1,
		 'hom',2,
		 'gene',3,
		 'ggp',4,
		 );

# default, def, hom, gene
# EDITABLE
%fmap_col=(
	   
	   # methods
	  
	   # real genes
	   'supported_CDS',
	   [1,1,1,1,1,],
	   'supported_mRNA',
	   [1,1,1,1,1,],

	   # repeats
	   'RepeatMasker',
	   [1,1,2,2,2,],
	   'RepeatMasker_SINE',
	   [1,1,2,2,2,],
	   'hmmfs_alu',         #obs
	   [1,1,2,2,2,],
	   'repbase_tblastx',   #obs
	   [1,1,2,2,2,],
	   
	   # homologies
	   'BLASTX',
	   [1,1,1,2,2,],
	   '-BLASTX',
	   [1,1,1,2,2,],
	   'BLASTN',
	   [1,1,1,2,2,],
	   'est_hmmfs',
	   [1,1,1,2,2,],
	   'vertebrate_mRNA',
	   [1,1,1,2,2,],
	   'EST_clusters',
	   [1,1,1,2,2,],
	   'sanger_sts_hmmfs',
	   [1,1,1,2,2,],
	   'sts_hmmfs',
	   [1,1,1,2,2,],
	   'tandem',
	   [1,1,1,2,2,],
	   'cgidb_hmmfs',
	   [1,1,1,2,2,],

	   # gene prediction
	   'GENSCAN',
	   [1,1,2,2,1,],
	   
	   # exon predictions
	   'Grail1',            #old
	   [1,1,2,2,1,],
	   'Grail1.3',
	   [1,1,2,2,1,],
	   'Grail2',
	   [1,1,2,2,1,],
	   'FEXH',
	   [1,1,2,2,1,],
	   'HEXON',
	   [1,1,2,2,1,],
	   'Xpound',
	   [1,1,2,2,1,],
	   
	   # other predictions
	   'Predicted_CpG_island',
	   [1,1,2,2,2,],
	   
	   # hardcode acedb
	   # gene prediction
	   '-Coding',
	   [1,1,2,2,1,],
	   'Coding',
	   [1,1,2,2,1,],

	   # always off
	   'Assembly Tags',
	   [1,2,2,2,2,],
	   
	   'Locator',
	   [1,1,1,1,1,],
	   'Sequences & ends',
	   [1,1,1,1,1,],
	   '-Confirmed introns',
	   [1,1,1,1,1,],
	   'Restriction map',
	   [1,1,1,1,1,],
	   'Summary bar',
	   [1,1,1,1,1,],
	   'Scale',
	   [1,1,1,1,1,],
	   'Confirmed introns',
	   [1,1,1,1,1,],
	   'EMBL features',
	   [1,1,1,1,1,],
	   'CDS Lines',
	   [2,2,2,2,2,],
	   'CDS Boxes',
	   [2,2,2,2,2,],
	   'Alleles',
	   [1,1,1,1,1,],
	   'cDNAs',
	   [2,2,2,2,2,],
	   'Gene Names',
	   [1,1,1,1,1,],
	   'Oligos',
	   [1,1,1,1,1,],
	   'Oligo_pairs',
	   [1,1,1,1,1,],
	   '3 Frame Translation',
	   [2,2,2,2,2,],
	   'ORF\'s',
	   [2,2,2,2,2,],
	   'Coding Frame',
	   [1,1,1,1,1,],
	   'ATG',
	   [2,2,2,2,2,],
	   'Gene Translation',
	   [2,2,2,2,2,],
	   'GC',
	   [1,1,1,1,1,],
	   'Coords',
	   [2,2,2,2,2,],
	   'DNA Sequence',
	   [2,2,2,2,2,],
	   'Brief Identifications',
	   [2,2,2,2,2,],
	   'Text Features',
	   [1,1,1,1,1,],
	   );

# B. gmap
$use_gmap_view=1;

# 1. define list of views
%gmap_acedb_viewcom=(
		     'def','',
		     'chr','-view webace_chr',
		     'chrctg1','-view webace_chrctg1',
		     'chrctg2','-view webace_chrctg2',
		     );

# 2. subroutine rule for view selection by maptype
$gmap_view=sub{
    my($rview,$object,$rafmap_view,$rhfmap_view,
       $rfmap_head,$rfmap_key)=@_;
    my(%fmap_head,@fmap_head,
       %fmap_key,@fmap_key,$view2);

# EDITABLE to define view/object selection rules
    if($object=~/^Chr_\w+ctg\d+$/){

# EDITABLE
	@$rafmap_view=(
		       'def',
		       'chrctg1',
		       'chrctg2',
		       );
# EDITABLE
	%$rhfmap_view=(
		       'def','Default view',
		       'chrctg1','All clones',
		       'chrctg2','Selected clones',
		       );
# EDITABLE
	%fmap_head=(
		    'def',1,
		    'chrctg1',1,
		    'chrctg2',1,
		    );
# EDITABLE
	$fmap_head[1]='';
# EDITABLE
	%fmap_key=(
		   'def',1,
		   'chrctg1',1,
		   'chrctg2',1,
		   );
# EDITABLE
	$fmap_key[1]=<<ENDOFTEXT;
Clone status colour key: 
<img src="/webace/I/black.gif"> Archived/Submitted
<img src="/webace/I/red.gif"> Finished
<img src="/webace/I/lightred.gif"> Contiguous
<img src="/webace/I/yellow.gif"> Assembly_start
<img src="/webace/I/green.gif"> Shotgun_complete
<img src="/webace/I/lightgray.gif"> Shotgun
<img src="/webace/I/blue.gif"> Library_made
<img src="/webace/I/lightblue.gif"> Fingerprinted
<img src="/webace/I/white.gif"> DNA_made/Streaked<p>
ENDOFTEXT
# EDITABLE
	$view2='chrctg2';
    }else{
# EDITABLE
	@$rafmap_view=(
		       'def',
		       'chr',
		       );
# EDITABLE
	%$rhfmap_view=(
		       'def','Default view',
		       'chr','Contigs only',
		       );
# EDITABLE
	%fmap_head=(
		    'def',1,
		    'chr',1,
		    );
# EDITABLE
	$fmap_head[1]='';
# EDITABLE
	%fmap_key=(
		   'def',1,
		   'chr',1,
		   );
# EDITABLE
	$fmap_key[1]=<<ENDOFTEXT;
Key: red bars are clone contigs.<p>
ENDOFTEXT
# EDITABLE
        $view2='chr';
    }

# in this case, set view if set to auto
# [no semantic zooming in this case]
    if($$rview eq 'auto'){
	$$rview=$view2;
    }

# check view exists or default
    unless($$rhfmap_view{$$rview}){
	$$rview='def';
    }
# use defined here only if auto
    if($$rview ne 'auto'){
	$view2=$$rview;
    }
    $$rfmap_head=$fmap_head[$fmap_head{$view2}];
    $$rfmap_key=$fmap_key[$fmap_key{$view2}];
    return $view2;

};

# C. External Links

$use_external_link=1;

$external_link=sub{
    my($label)=@_;
    my($type,@keys);
    if($label=~/^Sequence:em:(\w+)/i){
	$keys[0]=$1;
	$type="EMBLAC";
    }elsif($label=~/^Protein:sw:(\w+)/i){
	$keys[0]=$1;
	$type="SwissProtAC";
    }elsif($label=~/^Protein:tr:(\w+)/i){
	$keys[0]="tr:$1";
	$type="TremblAC";
    }elsif($label=~/^Protein:wp:(\w+)/i){
	$keys[0]="wp:$1";
	$type="WPAC";
    }
    return ($type,@keys);
};

# D. External Help

$use_external_help=0;

$external_help=sub{
    my($label,$type)=@_;
    my($link,$stem,$ext);
    $stem="/webace/methods-help.html#";
    if($label=~/^Sequence:/i && $type=~/Map/i){
	$ext="mapsequence";
    }elsif($label=~/^PAC:/i){
	$ext="mappac";
    }elsif($label=~/^STS:/i){
	$ext="mapsts";
    }elsif($label=~/^Probe:/i){
	$ext="mapprobe";
    }elsif($label=~/^Sequence:(em|sp|wp)/i){
	$ext="sequence";
    }elsif($label=~/^Protein:/i){
	$ext="protein";
    }elsif($label=~/^Sequence:\w+\.(fgeneh|genscan)/i){
	$ext="gene_predictions";
    }
    if($ext){
	$link=$stem.$ext;
    }
    return $link;
};

