/*  File: intron2.c
 *  Author: rd
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
 * -------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
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
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: 
 * Exported functions:
 * HISTORY:
 * Last edited: Aug 26 17:27 1999 (fw)
 * Created: Thu Aug 26 17:27:02 1999 (fw)
 * CVS info:   $Id: intron2.c,v 1.4 1999-09-01 11:22:23 fw Exp $
 *-------------------------------------------------------------------
 */

#include "acedb.h"

#define F(x,eff,ech) ((int)(0.5 + (-5.22- log( ((double) x) / ((double) eff) ) ) * (ech/(-5.22)) ));
#define WC(x)  ((x==2) || (x==12))
#define GC(x)  (x==12)
#define AT(x)  (x==2)
#define GU(x)  (x==10)
#define WCGU(x)  ((x==12) || (x==2) || (x==10))
#define PAIRE(x,y)  ((x*10)+y)

#define INC(y)  ( (y >= -4) && (y <= 4) )
#define ABS(y)  ( (y >= 0) ? y : -y )

/****************************************************************************/
/****************************************************************************/
						/****************************/
						/*  Creation de la fenetre  */
						/****************************/

static int fenetre(FILE *fep,FILE *fseq,int pos_p7,long pos_ab,int **ad_pc,
			int *ad_lg,long lgt,long *adlgt,int test)
  {
    int *p_c;
    static int c[4300];
    static char s[2];
    int si,stop;
    int lg_seq,f_i;
    int deb,fin;

    p_c=c; fin=0; deb=0;

    if (pos_ab==0) {
      pos_p7=4300;
      s[0] = (char)getc(fep);
      if (s[0]=='I') {
		stop=0;
		while (stop!=1) {
	      s[0] = (char)getc(fep);
	  	  if (s[0]=='S') {
	    	s[0] = (char)getc(fep);
	        if (s[0]=='Q')
	          stop=1;
	  	  }
		}
      }
    }
    for (f_i=pos_p7; f_i<4300; f_i++) {
      *(p_c+f_i-pos_p7)=*(p_c+f_i);
    }

    for (f_i=4300-pos_p7; f_i<4300; f_i++) {
      s[0]=' ';
      while ( (s[0]!='A') && (s[0]!='T') && (s[0]!='U') && (s[0]!='G')
      && (s[0]!='C') && (s[0]!='Y') && (s[0]!='N') && (s[0]!='a')
      && (s[0]!='t') && (s[0]!='u') && (s[0]!='g') && (s[0]!='c')
      && (s[0]!='y') && (s[0]!='n') && (s[0]!=EOF) )
	    s[0] = (char)getc(fep);
        if (s[0]==EOF) {
	      fin=1; fprintf(fseq,"%i //",-1); break;
        }
        switch(s[0]) {
	      case 'a':
  	      case 'A': if (test==0)
  	      			  si=0;
  	      			else
  	      			  si=2;
  	      			break;
          case 't':
	      case 'T':
	      case 'u':
	      case 'U': if (test==0)
	      			  si=2;
	      			else
	      			  si=0;
	      			break;
	      case 'G':
	      case 'g': if (test==0)
	      			  si=8;
	      			else
	      			  si=4;
	      			break;
	      case 'c':
	      case 'C': if (test==0)
	      			  si=4;
	      			else
	      			  si=8;
	      			break;
	      default : si=1; break;
        }
        (*(p_c+f_i))=si;
        fprintf(fseq,"%i ",si);
        fflush(fseq);
        lgt++;
        if (lgt%40==0)
          fprintf(fseq,"\n");
    } /* end for */
    lg_seq=f_i;
    *(ad_pc) = p_c;
    *(ad_lg) = lg_seq;
    *(adlgt) = lgt;
    return(fin);
  }
/****************************************************************************/
					/*******************************/
					/*   Creation des 3 matrices   */
					/*  des scores pour p7 et p7'  */
					/*******************************/

void mat_P7(int ***admat1,int ***admat2,int ***admat3)
  {
    int **p_mat1;            /* pointeur sur le pointeur de lignes */
    static int mat1[4][6];      /* matrice des scores */
    static int *pl_mat1[4];     /* pointeur sur les lignes de la matrice */
    int **p_mat2;            /* pointeur sur le pointeur de lignes */
    static int mat2[4][6];      /* matrice des scores */
    static int *pl_mat2[4];     /* pointeur sur les lignes de la matrice */
    int **p_mat3;            /* pointeur sur le pointeur de lignes */
    static int mat3[4][6];      /* matrice des scores */
    static int *pl_mat3[4];     /* pointeur sur les lignes de la matrice */
    FILE *fp,*fopen();
    int l,c;
    int val;

    for (l=0; l<4; l++)
      pl_mat1[l]=&mat1[l][0];

    p_mat1=pl_mat1;

    if ((fp=fopen("P7VAL","r"))==NULL) {
      printf("Probleme d'ouverture du fichier de la matrice de score de P7"); exit(0);
    }
    for (l=0; l<4; l++)
	  for (c=0; c<6; c++)
	  	if (val!=EOF) {
	      fscanf(fp,"%i\n",&val );
	      if (val!=0) {
	        (*(*(p_mat1+l)+c))=F(val,93,100);
          }
	      else {
	        (*(*(p_mat1+l)+c))=val;
	      }
		}
    filclose(fp);
    *(admat1)=p_mat1;

    for (l=0; l<4; l++)
      pl_mat2[l]=&mat2[l][0];

    p_mat2=pl_mat2;

    if ((fp=fopen("J8_7","r"))==NULL) {
      printf("Probleme d'ouverture du fichier de la matrice de score de J8_7"); exit(0);
    }
    for (l=0; l<4; l++)
	  for (c=0; c<6; c++)
        if (val!=EOF) {
	      fscanf(fp,"%i\n",&val);
	      if (val!=0) {
	        (*(*(p_mat2+l)+c))=F(val,93,100);
	      }
	      else {
	        (*(*(p_mat2+l)+c))=val;
	      }
	    }
    filclose(fp);
    *(admat2)=p_mat2;

    for (l=0; l<4; l++)
      pl_mat3[l]=&mat3[l][0];

    p_mat3=pl_mat3;

    if ((fp=fopen("P7c","r"))==NULL) {
      printf("Probleme d'ouverture du fichier de la matrice de score de P7c"); exit(0);
    }
    for (l=0; l<4; l++)
	  for (c=0; c<6; c++)
	    if (val!=EOF) {
          fscanf(fp,"%i\n",&val );
	      if (val!=0) {
	        (*(*(p_mat3+l)+c))=F(val,93,100);
	      }
	      else {
	        (*(*(p_mat3+l)+c))=val;
	      }
	    }
    filclose(fp);
    *(admat3)=p_mat3;
  }
/****************************************************************************/
			 			/***********************************/
			 			/*  Lecture des parametres a fixer */
			 			/***********************************/

void lecture(int *plancher,int *penabonus)
  {
    int i;
    FILE *fopen(),*fpl,*fpe;

    if ((fpl=fopen("plancher","r"))==NULL) {
      printf("Probleme d'ouverture du fichier plancher"); exit(0);
    }
	if ((fpe=fopen("penabonu","r"))==NULL) {
      printf("Probleme d'ouverture du fichier penabonu"); exit(0);
    }
    for (i=0; i<9; i++)
      fscanf(fpl,"%i\n",plancher+i);
    for (i=0; i<5; i++)
      fscanf(fpe,"%i\n",penabonus+i);
    filclose(fpl); filclose(fpe);
  } /* End */

/****************************************************************************/
			 			/*************************/
			 			/*  Localisation des P7  */
			 			/*************************/

void P7(int *ad_np7,int *ad_depart,int *pch,int ***adp7,int **p_mat,
		int finr,int *plancher)
  {
    int i,depart;            /* position de depart pour la recherche des p7' */
    int **p7;
    int np7;
    int l,j,c;               /* l et c sont des increments pour la matrice,
				c est un increment pour la chaine */
    int passe;               /* increment pour le parcours des 2000
				nucleotides testes */
    int SD;                   /* scores pour les doublets */
    int score;

     /* p7[0] recueille le score intermediaire puis le score final,
	p7[1] la pos du debut */

    np7 = 0;
    p7 = (int **) calloc (2, sizeof(int *));
    for (i=0; i<2; i++)
      p7[i] = (int *) calloc (2000, sizeof(int ));

    for (passe=30; passe<finr; passe++) {
      if ( ((*(pch+passe+2))==0) && ((*(pch+passe+5))==8) && (
	    ((*(pch+passe+6))==0) || ((*(pch+passe+7))==0) ||
	    ((*(pch+passe+8))==0) || ((*(pch+passe+9))==0) ) ) {
		score=0;
		for (j=0;j<10;j++) {
	  	  if (j<2)
	    	c=j;
	  	  else
	    	c=j-4;

	  	  if ( (j!=2) && (j!=3) && (j!=4) && (j!=5) ) {
	        l=(*(pch+j+passe));
	        if (l!=1) {
	          if (l==2 || l==4 || l==8)
			    l= ((int) (log((double) l )))+1;
	          score+=(*(*(p_mat+l)+c));
	        }
	  	  } /* end if */
	 	  else
  	  	    if (j==3) {
	    	  l=PAIRE( (*(pch+j+passe)),(*(pch+j+1+passe)) );
	    	  switch (l) {
	      	    case 40 : SD=59; break;
	     	    case 80 : SD=87; break;
  	   	        case  4 : SD=80; break;
	      	    case 24 : SD=40; break;
	      	    default : SD=-100;  break;
	    	  } /* end switch */
	  		  score+=SD;
	  	    } /* end if */
	    } /* end for */
	    if ( (score>*plancher) && (passe+3>50) ) {
	  	      /* selection des scores > 400 */
	  	  (*(*p7+np7))=score;             /* je remplis la case score */
	  	  (*(*(p7+1)+np7))=passe+3;         /* debut du p7 */
		         /* fin du p7 = debut du p7 + 6 */
	  	  np7++;
	    } /* fin du if (score>400) */
      } /* fin du if (pch...) */
    } /* fin des 2000 passes */
    depart=(*(*(p7+1)))+6+25;    /* depart pour la recherche
						 du p7' */
    (*(adp7))=p7;                     /* je renvoie la matrice au main */
    (*(ad_depart))=depart;             /* je renvoie 'depart' au main */
    (*(ad_np7))=np7;
  }
/****************************************************************************/
			 			/**************************/
			 			/*  Localisation des P7'  */
			 			/**************************/

void P7c(int *ad_np7c,int ***ad_p7c,int **p_matj8,int **p_matp7c,int depart,
		 int *pch,int lgseq,int *plancher)
  {
    int **p7c;             /* pointeur sur la matrice pl_p7c */
    int np7c;
    int l,j;                 /* l et c sont des increments pour la matrice,
				c est un increment pour la chaine */
    int passe;               /* increment pour le parcours des 2000
				nucleotides testes */
    int val;                 /* valeur que prend la matrice suivant qu'on a
				j8/7 ou p7c */
    int score;

    /* p7c[0][] recueille tout les scores intermediaires puis le score final,
       p7c[1][] est la position absolue du debut du candidat P7 */

    np7c=0;
    p7c = (int **) calloc (2, sizeof(int *));
    for (l=0; l<2; l++)
      p7c[l] = (int *) calloc (3000, sizeof(int));

    for (passe=depart; passe<lgseq-12+1; passe++)  {
      if ( ( ((*(pch+passe+10))==4) || ((*(pch+passe+10))==2) ) &&
	   ( ((*(pch+passe+6))==2) || ((*(pch+passe+7))==2) ||
	     ((*(pch+passe+8))==2) || ((*(pch+passe+9))==2) ) ) {
		score=0;
		for (j=0;j<12;j++) {
	  	  l=(*(pch+j+passe));
	  	  if (l!=1) {
	    	if (l==2 || l==4 || l==8)
	      	  l=( (int) (log( (double) l )))+1;
	    	if (j<6) {
	      	  val=(*(*(p_matj8+l)+j));
	    	}
	    	else {
	      	  val=(*(*(p_matp7c+l)+j-6));
	    	}
	    	score+=val;
	  	  } /* End if (l!=1) */
		} /* End for (j=0;j<12... */
		if (score>*(plancher+1)) {
	      /* selection des scores > 800 */
	  	  (*(*(p7c)+np7c))=score;
	  	  (*(*(p7c+1)+np7c))=passe+6;   /* debut du p7' */
					/* fin du p7' = debut du p7' + 5 */
	  	  np7c++;
		} /* End if (score>... */
      } /* End if */
    } /* End for */
    *(ad_p7c)=p7c;
    *(ad_np7c)=np7c;
  }

/****************************************************************************/
		    		/**********************************/
		    		/*  Selection des couples P7-P7'  */
		    		/**********************************/

int p7_p7c(int *pch,int **p7,int **p7c,int np7,int np7c,int nc7,long **cp7,
		   int *ad_nc7,int *plancher,long posab)
  {
    int i,j,l,l2,c,f ;
    int wc,gu,et,gc,penalite,gucont,gccont;
    int score;
    int existe;  /* determine si j'ai trouve des p7-p7' dans cette fenetre */

    existe=0;

    for (i=0; i<np7; i++) {
      for (j=0; j<np7c; j++) {
	    score=(*(*p7+i))+(*(*p7c+j));
	    if ( (score>*(plancher+2)) && ((*(*(p7c+1)+j))-(*(*(p7+1)+i)+1)-1>=15)
	        && ((*(*(p7c+1)+j))-(*(*(p7+1)+i)+1)-1<=2300) ) {

	      wc=0; gu=0; et=0; gc=0; penalite=0;

      /* 'wc' est un compteur de paires watson-crick, 'gu' compte les paires
	 GU, 'gc' les paires GC, 'et' les  paires  etranges. 'prem' teste si
	 la paire etrange est en  premiere (prem=1) ou en  derniere position
	 (prem=2) */
	      gccont = 0; gucont = 0;
	      for (c=0; c<7; c++) {

	   /* si on a plus d'1 paire etrange, ou plus  de 2 paires GU,
	      ou si et+gu depasse 2 - cad qu'on n'a pas 4 paires W-C -
	      ou enfin si la  paire etrange  est en premiere position,
	      on abandonne  directement  cette boucle  pour  passer au
	      couple suivant */
	        if ( (et>=2) || (gu>=3) || (et+gu>=3) )
	          break;
	        if (c!=1) {
	          if (c==0)   /* j'associe le 1er nucl. de p7 au dernier de p7' */
		        f=5;
	          else        /* je decale les nucl. de p7' */
		        f=6-c;
	          l=(*(pch + (*(*(p7+1)+i) + c)))
	         		+(*(pch + (*(*(p7c+1)+j) + f)));
	          if (c!=6)
		        l2=(*(pch + (*(*(p7+1)+i) + c+1)))
				      +(*(pch + (*(*(p7c+1)+j) + f-1)));
	          if ( WC(l) ) {
		        wc++;
			    if ( GC(l) ) {
		  	      gc++;
			    } /* End if GC(l) */
	      	  } /* End if WC(l) */
	          else {
		        if ( GU(l) ) {
		  		  gu++;
		  		  if (c==2)
		  		    penalite += 100;
		  		  if ( GU(l2) )
		    	    gucont=1;
			    } /* End if GU(l) */
			    else {
		          et++;
		          if (c==0)    /* paire etrange en 1ere position */
		            break;
		          if (c!=6)            /* paire etrange ailleurs qu'en */
		            penalite += 400;      /* derniere position */
		   	    } /* End else */
	          } /* End else */
	        } /* End if c!=1 */
	      }  /* End for */
	      if ( gc>1 )
	        score += 100;
	      if ( gucont==1 )
	        score -= 300;
	      score -= penalite;
	      score /= 10;
	      if ( (wc>=4) && (gc>=1) && (wc+gu+et==6) && (score>=*(plancher+3)) ) {
	        		/*if ( !((wc==5) && (et==1)) ) {
	              		 seul cas subsistant a rejeter */
  	          *(*(cp7+1)+nc7) = (long) score;
	          *(*(cp7+2)+nc7) = (long) (*(*(p7+1)+i)) + posab;
	          *(*(cp7+3)+nc7) = (long) (*(*(p7c+1)+j)) + posab;
	          nc7++;
	          existe = 1;
	      } /* End if */
	    } /* End if */
      }
    }
    *(ad_nc7)=nc7;
    return(existe);
  }
/****************************************************************************/
							/*************/
							/*  Fenetre  */
							/*************/

void fenet(long **cp7,long **fen,long lgt,int nc7,int *adnfen,int disti)
  {
    int nfen,i;
    long dif,maxp7c,dist;

    dist = (long)disti;
    nfen = 0;
    if ( **(cp7+2)-dist < 0 )
      **fen = 0;
    else
      **fen = **(cp7+2)-dist;
    maxp7c = **(cp7+3);
    **cp7=0;

    for (i=1; i<nc7; i++) {
      dif = (*(*(cp7+2)+i))-(*(*(cp7+2)+i-1));
      if (dif>dist) {
        if (maxp7c+40>lgt)
          *(*(fen+1)+nfen) = lgt-1;
        else
          *(*(fen+1)+nfen) = maxp7c+40;
        nfen++;
        if ( *(*(cp7+2)+i)-dist < 0 )
          *(*fen+nfen) = 0;
        else
          *(*fen+nfen) = *(*(cp7+2)+i)-dist;
        maxp7c = *(*(cp7+3)+i);
      } /* fin du if (dif> ...) */
      if (*(*(cp7+3)+i)>maxp7c)
        maxp7c = *(*(cp7+3)+i);
      *(*cp7+i) = (long) nfen;
    } /* fin du for (i=1 ...) */
    if (maxp7c+40>lgt)
      *(*(fen+1)+nfen) = lgt-1;
    else
      *(*(fen+1)+nfen) = maxp7c+40;
    nfen++;
    *adnfen=nfen;
  }
/****************************************************************************/
					/************************************/
					/*  creation de la fenetre en sqce  */
					/************************************/

int sequence(int on,int fen1,int fen2,int *seq,int *adfinfep,long **fen,
			 FILE *fep)
  {
    long prec,test,passe,k;
    int deb,fin,i,j;
    int si;
    int s;

    on=1;
    if ( (fen1==0) && (fen2==0) ) {
      prec=-1;
    }
    else
      prec=(*(*fen+fen1))+19999;

    test=(*(*fen+fen2))-prec;
    if ( test<=0 ) {
    		/* les deux fenetres se chevauchent */
      fin =(int) (prec+1-(*(*fen+fen2)));
      for (i=0; i<fin; i++) {
        j=(int)((*(*fen+fen2))-(*(*fen+fen1)));
	    *(seq+i)=*(seq+i+j);
      } /* end for i=..*/
      deb=(int)(prec+1-(*(*fen+fen2)));
      fin=20000;
    } /* end if pfen+1..<=0 */
    else {
    		/* les deux fenetres ne se chevauchent pas */
      if (*adfinfep != 1 ) {
        passe=((*(*fen+fen2))-prec-1);
        for (k=0; k<passe; k++) {
          si=fscanf(fep,"%i ",&s);
	      if (s==-1) {
	        *adfinfep=1; break;
          } /* End if (s[0 ... */
	    } /* End for (k=0 ... */
	  } /* End if (*adfinfep != 1 ...*/
      deb=0;
      fin=20000;
    } /* end else donc pfen+1..>0 */
    if (*adfinfep != 1 ) {
      for (i=deb; i<fin; i++) {
        si=fscanf(fep,"%i ",&s);
        if (s==-1) {
          *adfinfep=1; break;
	    } /* End if (s[0... */
	    *(seq+i) = s;
	  } /* end for i=deb */
    } /* End if ( *adfinfep != 1... */
    return(on);
  } /* end void */
/****************************************************************************/
						/**********************/
						/*  Recherche des P4  */
						/**********************/

void p4(int *seq,int **ad_p4,int *ad_np4,int derp7)
  {
    int *p4,np4,i,l;

    np4=0;  p4 = (int *) calloc (2000, sizeof (int));

    for (i=0; i<derp7; i++) {
      l=PAIRE((*(seq+i+7)),(*(seq+i+8)));
      if( l==0 || l==80 )
	    if( (*(seq+i+1)==4) || (*(seq+i+2)==4) || (*(seq+i+3)==4) ) {
	      *(p4+np4)=i;        /* debut du p4 */
			  /* fin du p4 = debut du p4 + 6 */
	      np4++;
	    }  /* end if */
    }  /* end for */
    *ad_p4=p4;
    *ad_np4=np4;
  } /* end void */
/****************************************************************************/
						/***********************/
						/*  Recherche des p4'  */
						/***********************/

void p4d(int *seq,int **ad_p4c,int *ad_np4c,int prem4,int derp7)
  {
    int *p4c,np4c,i;

    np4c=0;  p4c = (int *) calloc (2500, sizeof(int));

    for (i=prem4+18; i<derp7; i++) {
      if (*(seq+i-1)==0)
	    if ( (*(seq+i+3)==8) || (*(seq+i+4)==8) || (*(seq+i+5)==8) ) {
	      *(p4c+np4c)=i;         /* debut du p4' */
			   /* fin du p4' = debut du p4' + 6 */
	      np4c++;
	    }  /* end if */
    }  /* end for */
    *ad_p4c=p4c;
    *ad_np4c=np4c;
  }
/****************************************************************************/
		       			/********************/
		       			/*  Fonction score  */
		       			/********************/

void scores(int lg,int *seg1,int *seg2,int *adscore,int *admaxpos,
			int *adminpos,int *adgc)
  {
    int score,maxpos,minpos,max,min,i,s,l[20],p1,*pscore,gc,p;

    max=-30; min=300; score=0; p1=0; gc=0; minpos=0; maxpos=0;

    pscore = (int *) calloc (15, sizeof(int));
    for (i=0; i<lg; i++) {
      l[i]=(*(seg1+i))+(*(seg2+i));
    }
    for (i=0; i<lg; i++) {
      if ( WCGU(l[i]) ) {
        p=i;  break;
      }
      else
        l[i]=-20;
	}
		/* Si le nucleotide est le premier */

	  if ( GC(l[p]) )
	    p1=1;
	  if ( AT(l[p]) )
	    p1=2;
	  if ( GU(l[p]) )
	    p1=3;

    for (i=p+1; i<lg; i++) {

      if ( GC(l[i]) )
		s=7;
      if ( AT(l[i]) )
		s=5;
      if ( GU(l[i]) )
		s=3;
      if (l[i]==4)     /* vers un AC ou TT */
		s=-2;
	  if ( (!WCGU(l[i])) && (l[i]!=4) && (l[i]!=18) )
		s=-10;           /* toute autre paire que les precedentes */
	  if (l[i]==18)
	    s=0;

	  score += s;   /* pour ce nucleotide je somme les scores */
      (*(pscore+i))=score;

      if (score>max) {
	       /* je recherche le maximum */
		max=score;
		maxpos=i;
      }   /* end if */
    }   /* end for (i=0;... */

	/* je recherche le minimum precedent le maximum */
	if (p+1==maxpos)
	  minpos=p+1;
	else {
      for (i=p+1; i<maxpos; i++) {
        if ( ((*(pscore+i))<min) && ((*(pscore+i)) > -5) ) {
		  min=(*(pscore+i));
		  minpos=i;
        } /* end if */
      } /* End for (i=p+1;... */
    } /* End else */
    score = max;
    if (minpos==p+1) {
      switch(p1) {
		case 1 : score += 7; minpos=p; break;  /* 1ere paire = (G,C) */
		case 2 : score += 5; minpos=p; break;  /* 1ere paire = (A,U) */
		case 3 : score += 3; minpos=p; break;  /* 1ere paire = (G,U) */
		default: break;
      }   /* end switch */
    }

    for (i=minpos; i<maxpos+1; i++) {
      if (GC(l[i]))
        gc++;
    }
    free(pscore);
    *adgc=gc;
    *admaxpos=maxpos;
    *adminpos=minpos;
    *adscore=score;
  }   /* end */
/****************************************************************************/
						/*************************************/
						/*  Rabotage ddes seq.chevauchantes  */
						/*************************************/

void rabot(int *seq,int dp,int fp,int dpc,int fpc,int *coord,int *adlg,
		   int *ps1,int *ps2)
  {
    int vect,lg,i,l;

    lg = fp-dp+1;
    for (i=0; i<lg; i++) {
      vect = fpc-dp-(2*i);
      if (vect <= 3) {
        lg = i;
        break;
      } /* End if (vect... */
    l=(*(ps1+lg-1))+(*(ps2+lg-1));
    if (!WCGU(l)) lg--;
    } /* End for (i=0;... */
    for (i=0; i<lg; i++) {
      *(ps1+i) = *(seq+dp+i);
      *(ps2+i) = *(seq+fpc-i);
    } /* End for (i=0;... */
    *coord = dp;
    *(coord+1) = dp + lg -1;
    *(coord+2) = fpc - lg +1;
    *(coord+3) = fpc;
    *adlg = lg;
  } /* End void */
/****************************************************************************/
						/**************************/
						/*  Assoc. des p4 et p4'  */
						/**************************/

void ass_p4(int *c1,int *c2,int *pos,int debp4,int finp4c,int *seq,int bulge)
	/*  Quand bulge=0 il n'y a pas de bulge, quand bulge=1 le bulge est */
	/* en p4 pos. 5, quand bulge=2 le bulge est en p4 pos. 2, quand */
	/* bulge=3 le bulge  est  en p4' pos. 2, quand bulge=4 le bulge est */
	/* en p4' pos. 5 */
  {
    int i,j;

    j=0;
    if (bulge==0 || bulge==1 || bulge==2) {
      		/* b=0, b=1, b=2  XX-XX-XX */
      *pos=debp4+1; *(pos+1)=debp4+6;
      for (i=debp4+1; i<debp4+7; i++) {
		*(c1+j)=(*(seq+i));
		if (j==1 || j==4) {
	  	  *(c1+j+1)=-10;
	  	  j++;
		}
		j++;
      }
    }
    if (bulge==3 || bulge==4) {
    		/* b=4  XX-XXXXX */    /* b=3  XXXXX-XX */
      *pos=debp4; *(pos+1)=debp4+6;
      for (i=debp4; i<debp4+7; i++) {
		*(c1+j)=(*(seq+i));
		if ( (j==4 && bulge==3) || (j==1 && bulge==4) ) {
	  	  *(c1+j+1)=-10;
	  	  j++;
		}
		j++;
      }
    }
    j=0;
    if (bulge==0 || bulge==3 || bulge==4) {
    			/* b=0, b=3, b=4  XX-XX-XX */
      *(pos+2)=finp4c-6; *(pos+3)=finp4c-1;
      for (i=finp4c-1; i>finp4c-7; i--) {
		*(c2+j)=(*(seq+i));
		if (j==1 || j==4) {
	  	  *(c2+j+1)=-10;
	  	  j++;
		}
		j++;
      }
    }
    if (bulge==1 || bulge==2) {
    			/* b=1  XX-XXXXX */    /* b=2  XXXXX-XX */
      *(pos+2)=finp4c-6; *(pos+3)=finp4c;
      for (i=finp4c; i>finp4c-7; i--) {
		*(c2+j)=(*(seq+i));
		if ( (j==1 && bulge==1) || (j==4 && bulge==2) ) {
	  	  *(c2+j+1)=-10;
	  	  j++;
		}
		j++;
      }
    }
  }
/****************************************************************************/
							/******************/
							/*  score p4_p4'  */
							/******************/

int score44c(int *ps1,int *ps2,int m)
  {
    int score;
    int zx,gc,at,tol,poset,etadj,et,l;
    int no,rep,xz;

    score=0; no=0;
			/* pas de paire (G,U) en pos. 1 ni en 2 */
    if ( (!GU((*ps1)+(*ps2))) && (!GU((*(ps1+1))+(*(ps2+1)))) ) {
      score += F(93,93,100);
    }
			/* pas de paire repetees dans les 5 1eres pos. */
    for (zx=0; zx<4; zx++) {
      rep=1;
      for (xz=zx+1; xz<5; xz++)  {
		if ( (PAIRE((*(ps1+zx)),(*(ps2+zx))))==(PAIRE((*(ps1+xz)),(*(ps2+xz)))) )
	  	  rep++;
	  }
      if ( rep>=3 )
	    no=1;
    }
    if ( no==0 )
      score += F(93,93,100);
			/*  pas de bulge  */
    if ( m==0 ) {
      score += F(75,93,100);
    }
    if ( AT((*ps1)+(*ps2)) ) {
      score += F(89,93,100);
    }
    else {
      if ( (GC((*(ps1+1))+(*(ps2+1)))) && (GC((*(ps1+3))+(*(ps2+3)))) ) {
        score += F(4,4,10);
      }
      else {
        score -= F(4,4,10);
      }
    }
			/* pas de UA en 4eme position */
    if ( PAIRE((*(ps1+3)),(*(ps2+3)))!=20 )
      score += F(92,93,100);
			/* pas de CG en 5eme ni en 7eme position */
    if ( (PAIRE((*(ps1+4)),(*(ps2+4)))!=48) && (PAIRE((*(ps1+6)),(*(ps2+6)))!=48) )
      score += F(93,93,100);
    gc=0; at=0; tol=0; poset=0; etadj=0; et=0;
    for (zx=0; zx<8 ;zx++) {
      l=((*(ps1+zx))+(*(ps2+zx)));
      switch(l) {
		case 12 : gc++; break;
		case  2 : at++; break;
		case 10 : break;
		default : if (l>0) {
		    		et++;
		    		if (zx==3 || zx==7)
		      		  poset=1;
		    		if (m!=0) {
		      		  if ( ( (*(ps1+zx-1))+(*(ps2+zx-1))<0 && (*(ps1+zx-1))+(*(ps2+zx-1))>-20 ) ||
		    			 ( (*(ps1+zx+1))+(*(ps2+zx+1))<0 && (*(ps1+zx+1))+(*(ps2+zx+1))>-20) )
						etadj=1;
					}
		  		  }  /* end if */
		  		  break;
      }  /* end switch */
      l=PAIRE((*(ps1+zx)),(*(ps2+zx)));
      if (l==40 || l==4 || l==24 || l==42 ) {
				  tol++;
				  if (zx==3 || zx==7)
		    		poset=1;
		  		  if (m!=0) {
		    		if ( ( (*(ps1+zx-1))+(*(ps2+zx-1))<0 && (*(ps1+zx-1))+(*(ps2+zx-1))>-20 ) ||
		    			 ( (*(ps1+zx+1))+(*(ps2+zx+1))<0 && (*(ps1+zx+1))+(*(ps2+zx+1))>-20) )
		      		  etadj=1;
		      	  }
		  		  break;
	  } /* End if */
    }  /* end for */
    if ( (gc>1) || ((gc==1) && (et+tol)==0) ) /*  une seule (G,C) et pas de paire et. */
      score += F(93,93,100);
    if (poset==0)                 /*  pas de paire et. en pos. 4 ni 8  */
      score += F(93,93,100);
      				/*  pas de paire etrange  */
    if ((et+tol)==0)
      score += F(81,93,100);

    if (tol>0)                    /*  paires et. tolerees (U,U) et (A,C)  */
      score += F(11,12,20);

    if (m!=0) {
			/*  trois (G,C)  */
      if (gc>=3) {
        score += F(17,18,20);
      }
      else {
        score -= F(17,18,20);
      }
			/*  pas de paire et. adjacente  */
      if (etadj==0) {
        score += F(16,18,20);
      }
      else {
        score -= F(16,18,20);
      }
    }
    score /= 10;
    return (score);
  }
/****************************************************************************/
						/********************/
						/*  couples p5_p5'  */
						/********************/

void p5_p5c(int *seq,int *ppo,int *adscore2,int *adp5d,int *adp5f,
			int *adp5cf,int *adp5cd)
  {
    int sc,cs,l;
    int minp,maxp,*pl1,*pl2;
    int score1,score2,gc,at,gt;
    int pp5d,pp5f,pp5cf,pp5cd;
    int p5d,p5cf,gcex,lg,*coord;

    score2=0;

    coord  = (int *) calloc (4, sizeof(int));
    for (sc=0; sc<2; sc++)  {
      pl1 = (int *) calloc (8, sizeof(int));
      pl2 = (int *) calloc (8, sizeof(int));
      p5d=(*(ppo+1))+sc+2+1;
      p5cf=(*(ppo+2))-2-1;
	  gc=0; at=0; gt=0; gcex=0;
	  for (cs=0; cs<8; cs++) {
		(*(pl1+cs))=(*(seq+p5d+cs));
		(*(pl2+cs))=(*(seq+p5cf-cs)); }
      for (cs=0; cs<8; cs++) {
		l=(*(pl1+cs))+(*(pl2+cs));
		switch(l) {
	  	  case 12 : gc++; break;
	  	  case  2 : at++; break;
		  case 10 : gt++; break;
	  	  default : gc=0; at=0; gt=0; break;
		}   /* end switch */
        if ( ((gc==1 && at>=1) || (gc>=2) || (at>=3)) && gt<=1 ) {
		  score1=0;
		  scores(8,pl1,pl2,&score1,&maxp,&minp,&gcex);
		  *coord = p5d+minp;      *(coord+1) = p5d+maxp;
		  *(coord+2) = p5cf-maxp;  *(coord+3) = p5cf-minp;
	      if (*(coord+2)-(*(coord+1))-1 < 3) {
	        rabot(seq,*coord,*(coord+1),*(coord+2),*(coord+3),coord,&lg,pl1,pl2);
	  	    scores(lg,pl1,pl2,&score1,&maxp,&minp,&gcex);
	  	    *(coord+1) = *(coord)+maxp;	*coord = *(coord)+minp;
		    *(coord+2) = *(coord+3)-maxp; *(coord+3) = *(coord+3)-minp;

	      } /* end if (*(coord+2... */
	      if ( gcex == 0 )
	        score1 -= 5;
	      if ( score1 > score2 ) {
            score2=score1;
	        pp5d=*coord;
	        pp5f=*(coord+1);
	        pp5cd=*(coord+2);
	        pp5cf=*(coord+3);
	      }  /* end if ( score1 > ...*/
	      break;
        }  /* end if */
      }   /* end for */
      free(pl1); free(pl2);
    }  /* end for */
    free(coord);
    *adscore2=score2;
    *adp5d=pp5d;    *adp5f=pp5f;
    *adp5cd=pp5cd;  *adp5cf=pp5cf;
  }
/****************************************************************************/
						/********************/
						/*  couples p4_p4'  */
						/********************/

void p4_p4c(int dist5,int *p4,int *p4c,int np4,int np4c,int **cp4,int nc4,
			int *adnc4,int *seq,int *plancher)
  {
    int *cp4b,ip,k,l,m,xm;
    int l1,wc,gu,et;
    int *pc1,*pc2;    /* ps1 et ps2 sont les seq. numeriques des p4 et p4' */
    int *ppos;     /* ppo contient les debuts et fins des p4 et p4' */
    int score ;        /* positions relatives */
    int d5,sc5,p5d,p5f,p5cd,p5cf;

    for (k=0; k<np4; k++) {
      for (l=0; l<np4c; l++) {
		if (*(p4c+l)-(*(p4+k))-1>=14) {
	  	  cp4b = (int *) calloc (11, sizeof(int));
	  	  for (m=0; m<5; m++) {
	    	pc1 = (int *) calloc (8, sizeof(int));
	    	pc2 = (int *) calloc (8, sizeof(int));
	    	ppos = (int *) calloc (4, sizeof(int));
	    	wc=0; gu=0; et=0;

	    	ass_p4(pc1,pc2,ppos,*(p4+k),*(p4c+l)+6,seq,m);
	    	  if ( PAIRE((*(pc1+1)),(*(pc2+1)))==48 ||    /* crible (a3) */
		 		   PAIRE((*(pc1+3)),(*(pc2+3)))==48 ) {
		     			/* CG en pos. 2 ou 4 */
	      		if ( AT((*pc1)+(*pc2)) ||               /* crible (b3) */
		   		     GC((*(pc1+1))+(*(pc2+1))) )  {
		      			/* AT en pos. 1 ou GC en 2 */
				  for (xm=0; xm<8; xm++) {
		  			l1=(*(pc1+xm))+(*(pc2+xm));
		  			switch (l1) {
		      		  case 12 :                      /* paires W-C */
		    		  case  2 : wc++; break;
		    		  case 10 : gu++; break;    /* paire GU ou GT */
		    		  case -20: break;                /* "double" bulge */
		    		  default : if (l1>=-10 && l1<0) {
						   			/* test de possibilite du bulge */
								  if ( m<3 && m!=0 ) {
				  		  			l1=(*(pc1+xm-1))+(*(pc2+xm));
								  }
								  else  {
								    if ( m!=0 ) {
				  		  			  l1=(*(pc1+xm))+(*(pc2+xm-1));
				  		  			}
								  }
								  if ( WC(l1) ) {
				  		  		    et=4;
				  		  			break;    /* crible (e3) */
								  }
			      	  			}
			      	  			else
								  et++;
			      	  			break;
		  			}   /* end switch */
		  			if (et>1 || gu>2)
		    		  break;
				  }   /* end for */

				  if (wc>=4 && et<2 && gu<3) {
						/* crible (c3) */
		  			if ( ( wc==4 && ((et==1 && gu==1) ||
		    			  (et==0 && gu==2)) ) || wc>4 ) {
			     			/* crible (d3) */
		    		  score = score44c(pc1,pc2,m);
		    		  if (score>*(plancher+4)) {
		      			p5_p5c(seq,ppos,&sc5,&p5d,&p5f,&p5cf,&p5cd);
		      			d5=p5cd-p5f;
		      			if ((sc5>0) && (score>(*cp4b)) && (d5<=dist5)) {
			    		  *cp4b=score;
			     		  switch (m) {
			      			case 0 : *(cp4b+1)=-1; break;
			      			case 1 : *(cp4b+1)=4; break;
			      			case 2 : *(cp4b+1)=2; break;
			      			case 3 : *(cp4b+1)=12; break;
			      			case 4 : *(cp4b+1)=14; break;
			      			default: break;
			    		  }
			    		  *(cp4b+2)=*ppos;
			    		  *(cp4b+3)=*(ppos+1);
			    		  *(cp4b+4)=*(ppos+2);
			    		  *(cp4b+5)=*(ppos+3);
			    		  *(cp4b+6)=sc5;
			    		  *(cp4b+7)=p5d;
			    		  *(cp4b+8)=p5f;
						  *(cp4b+9)=p5cd;
						  *(cp4b+10)=p5cf;
						  if (m==0)
			  	  			break;
		      			}
		    		  }   /* end if */
		  			}   /* end if */
				  }   /* end if */
	      		}   /* end if */
	    	  }   /* end if */
	    	  free(pc1); free(pc2); free(ppos);
	  	  }   /* end for */
	  	  if (*cp4b>0) {
	    	for (ip=0; ip<11; ip++)
	          *(*(cp4+ip)+nc4)=*(cp4b+ip);
	    	nc4++;
	  	  }
	  	  free(cp4b);
		}
      }
    }
    *adnc4 = nc4;
    free(p4); free(p4c);
  }
/****************************************************************************/
						/**************************/
						/*  Selection des p8-p8'  */
						/**************************/

int select_p8(int *pc1,int *pc2,int insert,int lg)
  {
    int selection;      /* booleen, 0 si couple non selectionne,
			   1 si couple selectionne */
    int n,l,wc,gu;
			/* 'wc' compte les paires W-C et 'gu' les paires GU */
    int sel,et,etdb,pos;         /* sel compte le nombre de paires W-C ou GU suivies */

    sel=0; wc=0; gu=0; et=0; etdb=0; selection=0; pos=0;

      for (n=0;n<lg;n++) {
		l=(*(pc1+n))+(*(pc2+n));
		switch (l) {
		  case 12 :
		  case  2 : sel++; wc++; break;
		  case 10 : sel++; gu++; break;
	  	  default : etdb++;
	  	  			if (etdb!=n+1) { et++; pos=n; }break;
	  	} /* End switch */
	  	if (et>1) {
	  	  sel=0; et=0; }
		if ( (sel>=3) && (insert<=0) ) {
	  	  if ( (wc>=3) && (gu<=4) ) {
	    	selection++;
	    	break;
	  	  }   /* end if */
		}   /* end if */
		else {
	  	  if ( (sel>=4) && (insert==1) ) {
	    	if ( (wc+gu>=6) && (gu<=5) ) {
	      	  selection++;
	      	  break;
	    	}   /* end if */
	  	  }   /* end if */
		}   /* end else */
      }   /* end for */
      if (et==1 && sel>=3) {
        if (pos>1 && lg-1-pos>1) {
          *(pc1+pos)=9; *(pc2+pos)=9;
        }
      }
      if (et==1 && sel<3) {
        selection=0; }
      return(selection);
  }   /* end */
/****************************************************************************/
		 			    /*********************/
		  			    /*  Fonction decale  */
		  			    /*********************/

void decale(int *seq,int **p7s,int np7s,int **p8,int *plancher)
  {
    int *ps1,*ps2,*ps3,s1[12],s2[12],s3[12];
    int dm,dn,dy,dk,insert;
    int i,j,k,debp8,finp8c;
    int *sc8,*dp8,*fp8,*dp8c,*fp8c;
    int score,max,min,gcex;
    int selection,lg,*coord ;

    ps1=s1;  ps2=s2;  ps3=s3;

  for ( i=0; i<np7s; i++ ) {
    coord = (int *) calloc(4, sizeof(int));
    sc8 = (int *) calloc (10, sizeof(int));
    dp8 = (int *) calloc (10, sizeof(int));
    fp8 = (int *) calloc (10, sizeof(int));
    dp8c = (int *) calloc (10, sizeof(int));
    fp8c = (int *) calloc (10, sizeof(int));
    for ( j=0; j<5; j++ ) {
      switch (j) {
        case 0  : debp8=*(*(p7s+1)+i) +6+9;
        		  finp8c=*(*(p7s+2)+i) -7; break;
        case 1  : debp8=*(*(p7s+1)+i) +6+9;
        		  finp8c=*(*(p7s+2)+i) -6; break;
        case 2  : debp8=*(*(p7s+1)+i) +6+9;
        		  finp8c=*(*(p7s+2)+i) -5; break;
        case 3  : debp8=*(*(p7s+1)+i) +6+10;
        		  finp8c=*(*(p7s+2)+i) -7; break;
        case 4  : debp8=*(*(p7s+1)+i) +6+11;
        		  finp8c=*(*(p7s+2)+i) -7; break;
        default : break;
      } /* End switch */
      for ( k=0; k<12; k++) {
        *(ps1+k) = *(seq+debp8+k);
        *(ps2+k) = *(seq+finp8c-k);
      } /* End for ( k=0; k<10;... */
      lg=12;
      *coord = debp8;			*(coord+1) = debp8+11;
	  *(coord+2) = finp8c-11;	*(coord+3) = finp8c;
      if (finp8c-11-debp8-11-1 < 3) {
        rabot(seq,debp8,debp8+11,finp8c-11,finp8c,coord,&lg,ps1,ps2); }
      selection = select_p8(ps1,ps2,0,lg);
      if ( selection != 0 ) {
        scores(lg,ps1,ps2,&score,&max,&min,&gcex);
	  	     /* il faut d(P8-P8') >= 3 */
	  	*(coord+1) = *coord+max;	*coord = *coord+min;
	  	*(coord+2) = *(coord+3)-max; *(coord+3) = *(coord+3)-min;
	  	score += 20;  /* Pas d'insert */
	    if (gcex==0)
	      score -= 5;
	  	if (score>*(plancher+5)) {
		  for (dm=0; dm<10; dm++) {
	      	if ( score>=*(sc8+dm) ) {
		  	  for (dn=8; dn>dm-1; dn--) {
		    	if ( *(sc8+dn) != 0 ) {
		      	    (*(sc8+dn+1))=*(sc8+dn);
		      		(*(dp8+dn+1))=*(dp8+dn);
		      		(*(fp8+dn+1))=*(fp8+dn);
		      		(*(dp8c+dn+1))=*(dp8c+dn);
		      		(*(fp8c+dn+1))=*(fp8c+dn);
		    	} /* End if ( *(sc8+dn... */
		      } /* End for (dn=8;... */
		  	  *(sc8+dm)=score;
		  	  *(dp8+dm)=*coord;
		  	  *(fp8+dm)=*(coord+1);
		  	  *(dp8c+dm)=*(coord+2);
		  	  *(fp8c+dm)=*(coord+3);
			  break;
	      	} /* End if ( score... */
	      } /* End for (dm=0;... */
        } /* End if (score>15) */
      } /* End if (selection==1) */
    } /* End for ( j=0; j<5;... */

    debp8=*(*(p7s+1)+i)+6+12;
    finp8c=*(*(p7s+2)+i) -7;
    for (k=0; k<12; k++)
	  *(ps2+k) = *(seq+finp8c-k);
    for (dk=debp8; dk<(debp8+141); dk++) {
      for (dy=0; dy<12; dy++) {
		*(ps1+dy) = *(seq+dk+dy);
		*(ps3+dy) = *(ps2+dy); }
		if ( dk-debp8>1 )
	  	  insert=1;
		else
	  	  insert=0;

	  	lg=12;
        *coord = dk;			*(coord+1) = dk+11;
	    *(coord+2) = finp8c-11;	*(coord+3) = finp8c;
        if (finp8c-11-dk-11-1 < 3) {
          rabot(seq,dk,dk+11,finp8c-11,finp8c,coord,&lg,ps1,ps3); }
        selection = select_p8(ps1,ps3,insert,lg);
        if ( selection != 0 ) {
          scores(lg,ps1,ps3,&score,&max,&min,&gcex);
	  	     /* il faut d(P8-P8') >= 3 */
	      *(coord+1) = *coord+max;	*coord = *coord+min;
	      *(coord+2) = *(coord+3)-max; *(coord+3) = *(coord+3)-min;

	  	  if (insert==0)
	  	    score += 20;  /* Pas d'insert */
	      if (gcex==0)
	        score -= 5;
	  	  if (score>*(plancher+5)) {
	  	    for (dm=0; dm<10; dm++) {
	      	  if ( score>=*(sc8+dm) ) {
		  	    for (dn=8; dn>dm-1; dn--) {
		    	  if ( *(sc8+dn) != 0 ) {
		      	    (*(sc8+dn+1))=*(sc8+dn);
		      		(*(dp8+dn+1))=*(dp8+dn);
		      		(*(fp8+dn+1))=*(fp8+dn);
		      		(*(dp8c+dn+1))=*(dp8c+dn);
		      		(*(fp8c+dn+1))=*(fp8c+dn);
		    	  } /* End if ( *(sc8+dn... */
		        } /* End for (dn=8;... */
		        *(sc8+dm)=score;
		  	    *(dp8+dm)=*coord;
		  	    *(fp8+dm)=*(coord+1);
		  	    *(dp8c+dm)=*(coord+2);
		  	    *(fp8c+dm)=*(coord+3);
			    break;
	      	  } /* End if ( score... */
	        } /* End for (dm=0;... */
		  } /* End if (score>15) */
        } /* End if (selection==1) */
      }   /* end for (dk=debp8;... */
      if (*sc8!=0) {
        for (k=0; k<10; k++) {
		  *(*(p8+(k*5)+0)+i) = *(sc8+k);
	   	  *(*(p8+(k*5)+1)+i) = *(dp8+k);
      	  *(*(p8+(k*5)+2)+i) = *(fp8+k);
      	  *(*(p8+(k*5)+3)+i) = *(dp8c+k);
      	  *(*(p8+(k*5)+4)+i) = *(fp8c+k);
        } /* End for (k=0; k<np8;... */
      } /* End if (*sc8!=0) */
      free(coord);
      free(sc8); free(dp8); free(fp8); free(dp8c); free(fp8c);
    } /* End for i<np7s */
  }   /* end */
/****************************************************************************/
		   			/************************************/
		   			/*  rass. des p4-p4' et des p7-p7'  */
		   			/************************************/

void ens1(int *seq,int d4c7,int **cp7,int **cp4,int nc4,int nc7,int **ras,
		  int nras,int *adnras,int *plancher)
  {
    int gc,at,gu,pd,pf;
    int max,min,score,penal;
    int *pl1,*pl2,gcex;
    int es,i,zx,xz;
    int debp7,finp4c,lg,*coord;

    coord = (int *) calloc(4, sizeof(int));
    pl1 = (int *) calloc (5, sizeof(int));
    pl2 = (int *) calloc (5, sizeof(int));
    for (xz=0; xz<nc7; xz++) {
      debp7=(*(*(cp7+1)+xz));
      for (zx=0; zx<nc4; zx++) {
		gc=0; at=0; gu=0; score=0; gcex=0;
		penal = 0;
		finp4c=(*(*(cp4+5)+zx));
		  if (debp7-finp4c-1>=14 && debp7-finp4c<d4c7) {
	  		for (es=0; es<5; es++) {
	    	  *(pl1+es)=(*(seq+finp4c+es+1));
	    	  *(pl2+es)=(*(seq+debp7-es-3-1));
	  		}
	  		if (WCGU( *(pl1+1)+(*(pl2+1)) ) ){
	  		  pd=1; pf=1; lg=1;
	  		  if (WCGU( *pl1+(*pl2) ) ) {
	  		    pd=0; lg++; }
	  		  if (WCGU( *(pl1+2)+(*(pl2+2)) ) ) {
	  		    pf=2; lg++; }
	  		  if (lg==1) continue;
	  		  for (i=0; i<3; i++) {
	    	    switch( *(pl1+i)+(*(pl2+i)) ) {
				  case 12 : gc++; break;
			      case  2 : at++; break;
				  case 10 : gu++; break;
				  default : if (i==0)
			   		    	  penal += 5; break;
		  	    }   /* end switch */
		      }   /* end for i=0; i<3... */

		      if ( (lg==2 && gc>=1) || (lg==3 && gu<3) ) {
		        if ( pf==2 && WCGU(*(pl1+3)+(*(pl2+3))) ) {
		          pf=3; lg++;
		        }
		        if ( pf==3 && WCGU(*(pl1+4)+(*(pl2+4))) ) {
		          pf=4; lg++;
		        }
		      }
		      else continue;
		      if (pd==1)
		        for (i=0; i<4; i++) {
		          *(pl1+i)=*(pl1+i+1);
		          *(pl2+i)=*(pl2+i+1);
		        }
		      scores(lg,pl1,pl2,&score,&max,&min,&gcex);
              *coord = finp4c+1+pd;      *(coord+1) = finp4c+1+pf;
		      *(coord+2) = debp7-4-pf;  *(coord+3) = debp7-4-pd;
	          if (*(coord+2)-(*(coord+1))-1 < 3) {
	      	    rabot(seq,*coord,*(coord+1),*(coord+2),*(coord+3),coord,&lg,pl1,pl2);
	  	  	    scores(lg,pl1,pl2,&score,&max,&min,&gcex);
	          }

	  	      if ( gcex == 0 )
			    penal += 5;
		      score -= penal;
		      if (score<0) continue;
	  	      if ( (*(*cp7+xz))+(*(*cp4+zx))+(*(*(cp4+6)+zx))+score>=*(plancher+6)) {
	    	   (*(*ras+nras))=xz;                /* ind. du couple p7-p7' */
	    	   (*(*(ras+1)+nras))=zx;            /* ind. du couple p4-p4' */
	    	   (*(*(ras+2)+nras))=score;         /* score du p6  */
	    	   (*(*(ras+3)+nras))=lg;   /* lg du p6-p6' */
	    	   (*(*(ras+4)+nras))=*coord;      /* debut du p6  */
	    	   (*(*(ras+5)+nras))=*(coord+2); /* debut du p6' */
 	    	   nras++;
	  	      }  /* end if */
	  	    } /* end if (WCGU... */
	  	}   /* end if (debp7-... */
      }  /* end for */
    }   /* end for */
    free(coord);
    *adnras = nras;
  } /* end */
/****************************************************************************/
			   /*****************/
			   /*  P6a  -  P6b  */
			   /*****************/

void P6a_b(int fp4c,int dp7,int *ps,int *adp6a,int *afp6a,int *adp6b,
		   int *afp6b,int *asc6)
  {
    int i,j,y,l[5],lc[5],ll,score1,score2,mxp,mnp;
    int p2,p3,p4,d2,d3,d4,dp6a,fp6a,dp6b,fp6b;
    int *pl,*plc,gcex;

    pl = l; plc = lc;
    score1=0; score2=0; mxp=0; mnp=200;
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
      	p2=0; p3=0; p4=0;
        for (y=0; y<5; y++) {
          gcex = 0;
          *(pl+y)=*(ps+fp4c+y+6+i);
          *(plc+y)=*(ps+dp7-y-8-j);
          ll=*(pl+y)+(*(plc+y));
          switch (ll) {
            case 12 :   p2++; p3++; p4++;
						d2 = (p2==1) ? y : d2;
						d3 = (p3==1) ? y : d3;
						d4 = (p4==1) ? y : d4;
						break;
	    	case  2 :   p3++; p4++;
						p2 = (p2<2) ? 0 : p2;
						d3 = (p3==1) ? y : d3;
						d4 = (p4==1) ? y : d4;
						break;
	    	case 10 :   p4++;
						p2 = (p2<2) ? 0 : p2;
						p3 = (p3<3) ? 0 : p3;
						d4 = (p4==1) ? y : d4;
						break;
	    	default :   p2 = (p2<2) ? 0 : p2;
						p3 = (p3<3) ? 0 : p3;
						p4 = (p4<4) ? 0 : p4;
						break;
	  	  } /* END switch */
		} /* END y=... */
		if (p4>=4 || p3>=3 || p2>=2) {
		  scores(5,pl,plc,&score2,&mxp,&mnp,&gcex);
	  	  if ( score2>score1 ) {
		    score1=score2;
	       /*  je  garde  cette  solution  */
	        dp6a=fp4c+y+6+i+mnp;
	        fp6a=fp4c+y+6+i+mxp;
	        dp6b=dp7-y-8-j-mxp;
	        fp6b=dp7-y-8-j-mnp;
		  } /* END if (score2... */
		} /* END if (p4>... */
		else {
		  score2=0;
		}
      } /* END j=... */
    } /* END i=... */
    *adp6a=dp6a; *afp6a=fp6a;
    *adp6b=dp6b; *afp6b=fp6b;
    *asc6=score1;
  }
/****************************************************************************/
						/************************/
						/*  Solutions globales  */
						/************************/

int sol2(int **p4,int **p7,int **p8,int **ras,int nras,int *p3b,int **p3,
		 int n3,int **s38,int n38,int i7,int i4,int i,int *plancher,
		 int *penabonus)
  {
    int i8,i3,id3,val;
    int score,ins73,ins34;

  for (i8=0; i8<50; i8 += 5) {
    if ( *(*(p8+i8)+i7)>=0) {
      for (i3=0; i3<n3; i3++) {
      	id3 = (*(p3b+i3));
		if ( *(*(p7+2)+i7)-(*(*(p8+i8+4)+i7))-1>=5 ) {
		  if ( (*(*(p8+i8+1)+i7)-(*(*(p3+4)+id3))-1<4) &&
		       (*(*(p8+i8+1)+i7)-(*(*(p3+4)+id3))-1>=0) ) {
		    if ( *(*(p3+3)+id3)-(*(*(p7+1)+i7))-6-1>=1 ) {
		      if ( *(*(p8+i8+1)+i7)-(*(*(p7+1)+i7))-6-1<150 ) {
		        score = ( (*(*p3+id3))+(*(*(p8+i8)+i7))+(*(*p4+i4))+
				   (*(*(p4+6)+i4))+(*(*p7+i7))+(*(*(ras+2)+i)) );
				ins73=0; ins34=0;
		  		if ((*(*(p4+2)+i4))-(*(*(p3+2)+id3))-1>6) {
		  				/* insert 3-4 */
		  			  ins34=1; score -= *penabonus;
		  		}
		  		if ((*(*(p3+3)+id3))-((*(*(p7+1)+i7))+6)-1>3) {
		  					/* insert 7-3 */
		  			  ins73=1; score -= *(penabonus+1);
		  		}
		  		if ((((*(*(p7+2)+i7))+5)-(*(*(p3+1)+id3))+1)>2600)	/* intron de plus de 3kb */
	    			  score -= *(penabonus+2);
	    			  /* Ecart p3-p8==1 */
	      		if (ins34==0 && ins73==0) {
	      		  val = ( (*(*(p4+2)+i4))-(*(*(p3+2)+id3))-1 )-( (*(*(p3+3)+id3))-((*(*(p7+1)+i7))+6)-1);
			  	  if ( ABS(val)>=2 )
                        score -= *(penabonus+3); }
                val = ( (*(*(p8+1+i8)+i7))-(*(*(p3+4)+id3))+1)-( (*(*(p7+2)+i7))-((*(*(p8+4+i8)+i7))+6)+1);
  		  		if ( ABS(val)<=2 )
	    			  score += *(penabonus+4);
	    	    if ( score>*(plancher+7)) {
                  *(*s38+n38)      = score;
                  *(*(s38+1)+n38)  = *(*p3+id3);
                  *(*(s38+2)+n38)  = *(*(p3+1)+id3);		/* P3 */
                  *(*(s38+3)+n38)  = *(*(p3+2)+id3);
                  *(*(s38+4)+n38)  = *(*(p3+3)+id3);     /* P3' */
                  *(*(s38+5)+n38)  = *(*(p3+4)+id3);
                  *(*(s38+6)+n38)  = *(*(p8+i8)+i7);
                  *(*(s38+7)+n38)  = *(*(p8+i8+1)+i7);  /* P8 */
                  *(*(s38+8)+n38)  = *(*(p8+i8+2)+i7);
                  *(*(s38+9)+n38)  = *(*(p8+i8+3)+i7);  /* P8' */
                  *(*(s38+10)+n38) = *(*(p8+i8+4)+i7);
                  n38++;
	  			} /* End if (msc>340) */
		      } /* End if */
		    } /* End if */
		  } /* End if */
		} /* End if */
      } /* End for (i3=0; i3<n3;... */
    } /* End if ( *(*(p8+i8)+i7)>0... */
  } /* End for (i8=0; i8<50;... */
  return(n38);
}
/****************************************************************************/

void mem(int **s38,int n38,int *m38,int *adnm38)
  {
    int i,j,k,save,dif[8],nm38,ind;

      nm38=0; *m38 = 0; nm38++;

      for (i=1; i<n38; i++) {
        save=1;
      	for (j=0; j<nm38; j++) {
      	  ind = *(m38+j);
          for (k=0; k<4; k++) {
          	dif[k] = *(*(s38+k+2)+i)-(*(*(s38+k+2)+ind));
          }
          for (k=4; k<8; k++) {
          	dif[k] = *(*(s38+k+3)+i)-(*(*(s38+k+3)+ind));
          }
          if ( INC(dif[0]) && INC(dif[1]) && INC(dif[2]) &&
        	   INC(dif[3]) && INC(dif[4]) && INC(dif[5]) &&
        	   INC(dif[6]) && INC(dif[7]) ) {
        	save=0;
            if ( *(*s38+i)>*(*s38+ind) ) {
              *(m38+j) = i;
      		  break;
      		} /* End if ( *(*s38+i)>*(*s38+j) ) */
          } /* End if ( INC(-4,dif[0],4)... */
        } /* End for (j=0;... */
        if (save==1) {
          *(m38+nm38) = i;
      	  nm38++;
        } /* End if (save==1) */
      } /* End for (i=0;... */

    *(adnm38) = nm38;
  } /* End void */
/****************************************************************************/

void distr(int **psol,int nsol,Array histo)
{
  int i ;
  Array index ;
  
  index = arrayCreate(nsol,int) ;
  
  for (i=0; i < nsol-1; i++) 
    {
      if ( *(*(psol+16)+i) == *(*(psol+16)+i+1)  &&
	   *(*(psol+24)+i) == *(*(psol+24)+i+1) ) 
	{
	  array(index, i+1, int) = arr(index,i, int) ;
	  if (!arr(histo,arr(index,i,int), int))
	    arr(histo,arr(index,i,int), int)++ ;
	} /* End if ( (*(*(psol+1)+i... */
      else 
	{
	  array(index,i+1, int) = array(index,i, int) + 1 ;
	  if (!arr(histo,arr(index,i,int), int))
	    arr(histo,arr(index, i+1, int), int)++;
	  if (i == nsol-2)
	    arr(histo,arr(index,i+1,int), int)++;
	} 
    }
  arrayDestroy(index) ;
}

/****************************************************************************/

static int garde(Array histo,int **psol,int *result)
{
  int j,k,l,**meill3 ;
  static Array mm = 0 ;
  int i = -1, n=0, ntot=0, ii;

  while (i++, ii = arr(histo,i, int))
    {
      if (ii > 3)
	{ mm = arrayReCreate(mm, 6, int) ;
	  
	  for (j = 0; j < ii ; j++) 
	    for (k=0; k < 3; k++) 
	      if ( *(*psol+n+j) > arr(mm,k,int) )
		{
		  for (l = 1; l > k-1; l--) 
		    { 
		      *(*meill3+l+1)=(*(*meill3+l));
		      *(*(meill3+1)+l+1)=(*(*(meill3+1)+l));
		    }
		  arr(mm ,k, int) = *(*psol+n+j);
		  arr (mm, k + 3 , int) = n + j;
		  break;
		} 
	  
	  for ( j = 0; j < 3 ; j++)
	    *(result + ntot++ ) = arr(mm, j, int) ; /* *(*(meill3+1)+j);*/
	}
      else 
	for (j = 0; j < ii ; j++) 
	  *(result + ntot++ ) = n + j; 
      n += ii ;
    } 
  return ntot ;
} 

/****************************************************************************/

static void P33(int *seq,int **p4,int **p7,int **p8,int **ras,int nras,int **ssol,
		 int **psol,int *adnsol,int *plancher,int *penabonus,int *result)
{
  int i,j,k1,k2,k,i4,i7,i8;
  int **p3,*s3,*s3c,n3,*p3bis,n3bis;
  int l,wc,gc,gu,score,min,max;
  int dp4,fp7,dp8,nsol,sel,deb;
  int **s38,n38,*m38,nm38,ind,gcex;
  int dp6a,fp6a,dp6b,fp6b,sc6;
  Array histo ;
  int nfin,memp3;
  
  nsol = 0;

  for (i=0; i<nras; i++) {
    s38 = (int **) calloc (11, sizeof(int *));
    for (j=0; j<11; j++)
      s38[j] = (int *) calloc (2000, sizeof(int));
    n38=0;

    m38 = (int *) calloc (2000, sizeof(int));
    nm38=0;

    p3 = (int **) calloc (5, sizeof(int *));
    for (j=0; j<5; j++)
     p3[j] = (int *) calloc (2000, sizeof(int));
  	n3 = 0;

  	i7 = *(*ras+i); i4 = *(*(ras+1)+i); dp4 = *(*(p4+2)+i4);
						/* HYPOTHESE 1 */
  	fp7 = *(*(p7+1)+i7)+6;
  	for (k1=2; k1<5; k1++) {
	  for (k2=2; k2<6; k2++) {
	    s3  = (int *) calloc (9, sizeof(int));
	    s3c = (int *) calloc (9, sizeof(int));
	  	score = 0; gc = 0; wc = 0; gu = 0; gcex = 0;
	  	if (dp4-8-k2<0)
	  	  break;
	  	for (j=0; j<9; j++) {
		  *(s3+8-j) = *(seq+dp4-j-k2);
		  *(s3c+8-j) = *(seq+fp7+j+k1);
	  	} /* End for (j=0; j<9;... */
	    for (j=0; j<9; j++) {
		  l = *(s3+j)+(*(s3c+j));
		  switch(l) {
		  	case  2 : wc++; break;
		  	case 12 : gc++; wc++; break;
		  	case 10 : gu++; break;
		  	default : wc=0; gu=0; break;
		  } /* End switch */
		  if ( wc>=3 && gu<=2 ) {
		   	scores(9,s3,s3c,&score,&max,&min,&gcex);
		  	if (gcex==0)
		      score -= 5;
		  	if ( score>=*(plancher+8) ) {
		  	  *(*p3+n3) = score;
  	  		  *(*(p3+1)+n3) = dp4-k2-8+min;
  	  		  *(*(p3+2)+n3) = dp4-k2-8+max;
  	  		  *(*(p3+3)+n3) = fp7+k1+8-max;
  	  		  *(*(p3+4)+n3) = fp7+k1+8-min;
  	  		  n3++;
  	  		       break;
		  	} /* End if ( score>22 ) */
		  } /* End if ( wc>=3... */
	  	} /* End for (j=0; j<9;... */
	  		free(s3); free(s3c);
	  } /* End for (k2=2; k2<4;... */
  	} /* End for (k1=2; k1<4;... */

  						/* HYPOTHESE 2 */
  	for (i8=0; i8<50; i8 += 5) {
      if ( *(*(p8+i8)+i7)>0 ) {
        dp8 = *(*(p8+1+i8)+i7);
		for (k2=2; k2<6; k2++) {
		  s3  = (int *) calloc (9, sizeof(int));
	      s3c = (int *) calloc (9, sizeof(int));
		  score = 0; gc = 0; wc = 0; gu = 0; gcex = 0;
	  	  for (j=0; j<9; j++) {
			*(s3+8-j) = *(seq+dp4-j-k2);
			*(s3c+j)  = *(seq+dp8-j-1);
		  } /* End for (j=0; j<9;... */
		  for (j=0; j<9; j++) {
			l = *(s3+j)+(*(s3c+j));
			switch(l) {
		  	  case  2 : wc++; break;
		  	  case 12 : gc++; wc++; break;
		  	  case 10 : gu++; break;
		  	  default : wc=0; gu=0; break;
			} /* End switch */
			if ( wc>=3 && gu<=2 ) {
			  scores(9,s3,s3c,&score,&max,&min,&gcex);
		  	  if (gcex==0)
		    	score -= 5;
			  if ( score>=*(plancher+8) ) {
			    *(*p3+n3) = score;
  		  		*(*(p3+1)+n3) = dp4-k2-8+min;
  		  		*(*(p3+2)+n3) = dp4-k2-8+max;
  		  		*(*(p3+3)+n3) = dp8-1-max;
  		  		*(*(p3+4)+n3) = dp8-1-min;
  		  		n3++;
    		  		break;
			  } /* End if ( score>26 ) */
		  	} /* End if ( wc>=3... */
	  	  } /* End for (j=0; j<9;... */
	  	  free(s3); free(s3c);
		} /* End for (k2=2; k2<4;... */
      } /* End if (*(*(p8+i8)+i7)>0... */
  	} /* End for (i8=0; i8<50;... */

  						/* HYPOTHESE 3 */

  	for (i8=0; i8<50; i8 += 5) {
      if ( *(*(p8+i8)+i7)>=35) {
      	dp8 = *(*(p8+1+i8)+i7);
      	s3c = (int *) calloc (6, sizeof(int));
      	for (j=0; j<6; j++)
          *(s3c+j) = *(seq+dp8-j-1);
        deb = (dp4-200<0) ? 0 : dp4-200;
      	for (j=deb; j<dp4-3-6; j++) {
      	  s3  = (int *) calloc (6, sizeof(int));
	      score = 0; gc = 0; wc = 0; gu = 0; gcex = 0;
          for (k=0; k<6; k++) {
          	*(s3+k) = *(seq+j+k);
		  	l = *(s3+k)+(*(s3c+k));
          	switch(l) {
		  	  case  2 : wc++; break;
		  	  case 12 : gc++; wc++; break;
		  	  case 10 : gu++; break;
		  	  default : wc=0; gc=0; gu=0; break;
		  	} /* End switch */
		  } /* End for (k=0; k<6;... */
		  if ( wc>=5 && gu<=1 && gc>=1 ) {
			  scores(6,s3,s3c,&score,&max,&min,&gcex);
			  if (score>=*(plancher+9)) {
			  	*(*p3+n3) = score;
  		  	  	*(*(p3+1)+n3) = j;
  		  	  	*(*(p3+2)+n3) = j+5;
		  	  	*(*(p3+3)+n3) = dp8-1-5;
  		  	  	*(*(p3+4)+n3) = dp8-1;
  		  	  	n3++;
			  } /* End if (score>=32) */
		  } /* End if ( wc>=5... */
		  free(s3);
      	} /* for (j=deb; j<dp4-3-6;... */
      	free(s3c);
      } /* End if (*(*(p8+i8)+i7)>35... */
  	} /* End for (i8=0; i8<50;... */

    p3bis = (int *) calloc (2000, sizeof(int));
    n3bis = 0;

  	if ( n3!=0 ) {
	  (*p3bis) = 0;
  	  n3bis++;
  	  for (j=1; j<n3; j++) {
  	    memp3=0;
  	    for (k1=0; k1<n3bis; k1++) {
  	      sel=0;
  	      for (l=0; l<5; l++) {
  	        if ( *(*(p3+l)+(*(p3bis+k1)))==(*(*(p3+l)+j)) ) {
  	          sel++;
  	        }
  	      }
  	      if (sel==5) {
  	        memp3=1;
  	        break;
  	      }
   	    } /* End for (k1=0; k1<n3bis;... */
   	    if (memp3==0) {
   	      (*(p3bis+n3bis)) = j;
  	      n3bis++;
  	    }
  	  } /* End for (j=1; j<n3;... */
  	} /* End if (n3!=0) */

                   /*printf(" nbre p3bis = %i\n",n3bis);*/
	n38 = sol2(p4,p7,p8,ras,nras,p3bis,p3,n3bis,s38,n38,i7,i4,i,plancher,penabonus);

    free(p3bis);
    for (j=0; j<5; j++)
      free(p3[j]);
    free(p3);

	if ( n38>0 ) {
	  mem(s38,n38,m38,&nm38);
	}

              if (nm38>0) {
                for (j=0; j<nm38; j++) {
                  ind = *(m38+j);
                  P6a_b( (*(*(p4+5)+i4)),(*(*(p7+1)+i7)),seq,&dp6a,&fp6a,&dp6b,
                  					&fp6b,&sc6 );
        /* printf("score av = %i sc6ab = %i \n",(*(*m38+j)),sc6); */
    			  *(*(ssol+0)+nsol)=(*(*(s38+1)+ind));		/* score P3 */
	  			  *(*(ssol+1)+nsol)=(*(*p4+i4));        /* score P4 */
	  			  *(*(ssol+2)+nsol)=(*(*(p4+6)+i4));    /* score P5 */
	  			  *(*(ssol+3)+nsol)=(*(*(ras+2)+i));    /* score P6 */
	  			  *(*(ssol+4)+nsol)=(*(*p7+i7));        /* score P7 */
	  			  *(*(ssol+5)+nsol)=(*(*(s38+6)+ind));      /* score P8 */

	  			  *(*psol+nsol)=(*(*s38+ind))+sc6;      /* Score total */

	  			  *(*(psol+ 1)+nsol)=(*(*(p4+1)+i4));
	  			  *(*(psol+ 2)+nsol)=(*(*(s38+2)+ind));     /* p3 */
	  			  *(*(psol+ 3)+nsol)=(*(*(s38+3)+ind));
	  			  *(*(psol+ 4)+nsol)=(*(*(p4+2)+i4));   /* p4 */
				  *(*(psol+ 5)+nsol)=(*(*(p4+3)+i4));
	  			  *(*(psol+ 6)+nsol)=(*(*(p4+7)+i4));   /* p5 */
	  			  *(*(psol+ 7)+nsol)=(*(*(p4+8)+i4));
	  			  *(*(psol+ 8)+nsol)=(*(*(p4+9)+i4));   /* p5' */
	  			  *(*(psol+ 9)+nsol)=(*(*(p4+10)+i4));
	  			  *(*(psol+10)+nsol)=(*(*(p4+4)+i4));  /* p4' */
	  			  *(*(psol+11)+nsol)=(*(*(p4+5)+i4));
	  			  *(*(psol+12)+nsol)=(*(*(ras+4)+i)); /* p6 */
	  			  *(*(psol+13)+nsol)=(((*(*(ras+4)+i))+(*(*(ras+3)+i)))-1);
				  *(*(psol+14)+nsol)=(*(*(ras+5)+i)); /* p6' */
	  			  *(*(psol+15)+nsol)=(((*(*(ras+5)+i))+(*(*(ras+3)+i)))-1);
 	  			  *(*(psol+16)+nsol)=(*(*(p7+1)+i7));  /* p7 */
	  			  *(*(psol+17)+nsol)=((*(*(p7+1)+i7))+6);
				  *(*(psol+18)+nsol)=(*(*(s38+4)+ind));   /* p3' */
	  			  *(*(psol+19)+nsol)=(*(*(s38+5)+ind));
	  			  *(*(psol+20)+nsol)=(*(*(s38+7)+ind));  /* p8 */
	  			  *(*(psol+21)+nsol)=(*(*(s38+8)+ind));
	  			  *(*(psol+22)+nsol)=(*(*(s38+9)+ind));  /* p8' */
  	  			  *(*(psol+23)+nsol)=(*(*(s38+10)+ind));
      			  *(*(psol+24)+nsol)=(*(*(p7+2)+i7));  /* p7' */
	  			  *(*(psol+25)+nsol)=(*(*(p7+2)+i7))+5;
      			  nsol++;
      			} /* End for */
      		  } /* End if */
   free(m38);
    for (j=0; j<11; j++)
      free(s38[j]);
    free(s38);


  } /* End for (i=0; i<nras;... */
  if (nsol>1) {
    histo = arrayCreate(nsol, int) ;
    distr(psol,nsol,histo);
    nfin=garde(histo,psol,result);
  }
  else nfin=nsol;

  *(adnsol) = nfin;
} /* End void */
/****************************************************************************/
							/******************/
							/*  Dessin final  */
							/******************/

void dessin(int nimp,int lg,char **ad_pch)
  {
    int i,d;
    static char s[20];
    static char n[]="345546673887";

    d=(nimp/2)-1;
    s[1] = 'P'; s[2] = n[d];
    switch(nimp) {
      case  2:
      case  4:
      case  6:
      case 12:
      case 16:
      case 20: s[0]='<'; s[3]='-'; break;
      case  8:
      case 10:
      case 14:
      case 18:
      case 22:
      case 24: s[0]='>'; s[3]='c'; break;
      default : break;
    } /* END switch */
    if (lg>4)
      for (i=4; i<lg; i++)
	s[i] = '-';
    s[lg]='\0';
    *ad_pch=s;
  }
/****************************************************************************/
							/*******************/
							/*  Fichier final  */
							/*******************/

void final(int **ssol,int **psol,long debfen,int nsol,int *ps,char nom[],
		   FILE *lp,int test,int *result)
  {
    int i,j,deb,fin,nimp,suite,imp1,imp2,saut,z,y;
    char *pch,c[2];

    for (i=0; i<nsol; i++) {
      fprintf(lp,"\n");
      for (j=0; j<6; j++) {
        fprintf(lp," score %i  ",(*(*(ssol+j)+(*(result+i)))));
      }

	  deb = (int) ((*(*(psol+2)+(*(result+i))))-30);
	  fin = (int) ((*(*(psol+25)+(*(result+i))))+30);
	  if (deb<0)
	  	deb = 0;
	  if (fin>19999)
	  	fin = 19999;
      fprintf(lp,"\n   Sequence : %s  -=- ",nom );
      fprintf(lp," Score %i  -=-  Pos. 1er nucl. %ld \n",*(*psol+(*(result+i))),(long)deb+debfen );

      if (test==1)
        fprintf(lp," -=- Brin complementaire \n");
      else
        fprintf(lp,"\n");
      fflush(lp);
      nimp=2; suite=0;    /*initialise au P3 */
      imp1 = (int) ((*(*(psol+nimp)+(*(result+i)))));
      imp2 = (int) ((*(*(psol+nimp+1)+(*(result+i)))));
      dessin(nimp,imp2-imp1+1,&pch);
      for (j=deb; j<fin; j+=78) {
		saut=j;
		fprintf(lp,"\n");
		for (z=j; z<j+78; z++) {
		  if (z==fin)
		    break;
		  switch (*(ps+z)) {
		    case  0 : c[0]='A'; break;
		    case  2 : c[0]='T'; break;
		    case  4 : c[0]='C'; break;
		    case  8 : c[0]='G'; break;
		    default : c[0]='Y'; break;
		  }
	  	  fprintf(lp,"%c",c[0]);
		} /* END du for (z=j ... */
		fflush(lp);
	  	while (imp1 < j+78 && nimp <= 24) {
	      if (saut == j)
	      	fprintf(lp,"\n");
	      for (y=saut; y<imp1; y++)
	      	fprintf(lp," ");
	      saut = imp2+1;
	      if (imp2 < j+78 & suite==0) {
	      	fprintf(lp,"%s",pch);
	      	suite = 0;
	      	nimp=nimp+2;
	      	if (nimp>24)
			  break;
	      	imp1 = (int) (*(*(psol+nimp)+(*(result+i))));
	      	imp2 = (int) (*(*(psol+nimp+1)+(*(result+i))));
	      	dessin(nimp,imp2-imp1+1,&pch);
	      } /* END du if */
	      else {  /* imp2 >= j+78 */
	      	if (suite == 0) {
			  for (y=imp1; y<j+78; y++)
		  	  fprintf(lp,"%c",*(pch+y-imp1));
			  suite=j+78-imp1; imp1 = j+78;
	      	} /* END du if */
	      	else {
			  for (y=imp1; y<imp2+1; y++)
		  	  	fprintf(lp,"%c",*(pch+y-imp1+suite));  suite = 0;
			  nimp=nimp+2;
			  if (nimp>24)
		  		break;
			  imp1 = (int) (*(*(psol+nimp)+(*(result+i))));
			  imp2 = (int) (*(*(psol+nimp+1)+(*(result+i))));
			  dessin(nimp,imp2-imp1+1,&pch);
	      	} /* END du else */
	      } /* END du else */
	  	} /* END du while */
      } /* END du for (j=deb... */
    } /* END du for (i=0 .... */
  } /* End for (i=0; i<nsol... */
/****************************************************************************/
						/*************************/
						/*  Programme principal  */
						/*************************/

void compl(char nom[])
  {
    FILE *fopen(),*fseq,*fseq2;
    char **tab;
    static char s[2];
    long stop,i,j,lgt,reste,ind,fin,saut;

    if ((fseq=fopen(nom,"r"))==NULL) {
      printf("Pb d'ouverture de fichier %s",nom); exit(0);
    } /* End if */
    if ((fseq2=fopen("c:seqbis.dat","w"))==NULL) {
      printf("Pb d'ouverture de fichier c:seqbis.dat"); exit(0);
    } /* End if */

    lgt=0;

    tab = (char **) calloc (10, sizeof (char *));
    s[0] = (char)getc(fseq);
    if (s[0]=='I') {
	  stop=0;
	  while (stop!=1) {
	    s[0] = (char)getc(fseq);
	  	if (s[0]=='S') {
	      s[0] = (char)getc(fseq);
	      if (s[0]=='Q')
	        stop=1;
	  	}
	  }
    }
    ind = 0;
    while (fin!=1) {
      tab[ind] = (char *) calloc ((unsigned)64000, sizeof(char));
      for (i=0; i<64000; i++) {
        s[0]=' ';
        while ( (s[0]!='A') && (s[0]!='T') && (s[0]!='U') && (s[0]!='G')
        && (s[0]!='C') && (s[0]!='Y') && (s[0]!='N') && (s[0]!='a')
        && (s[0]!='t') && (s[0]!='u') && (s[0]!='g') && (s[0]!='c')
        && (s[0]!='y') && (s[0]!='n') && (s[0]!=EOF) )
 	      s[0] = (char)getc(fseq);
          if (s[0]==EOF) {
	        fin=1; break;
          } /* End if */
          if (fin!=1) {
            *(*(tab+ind)+i) = s[0];
            lgt++;
          }
      } /* end for */
      ind++;
    } /* End while */

    reste = lgt - ( (ind-1)*64000);
  	saut = 0;
    for (j=reste-1; j>=0; j--) {
      fprintf(fseq2,"%c",*(*(tab+ind-1)+j));
      fflush(fseq2);
      saut++; if (saut==80) {
    		    fprintf(fseq2,"\n"); saut=0;
    		    fflush(fseq2);
    	      } /* End if (saut... */
    } /* End for j=reste... */
    for (i=ind-1-1; i>=0; i--) {
      for (j=63999; j>=0; j--) {
        fprintf(fseq2,"%c",*(*(tab+i)+j));
        fflush(fseq2);
        saut++; if (saut==80) {
    		      fprintf(fseq2,"\n"); saut=0;
    		      fflush(fseq2);
    	        } /* End if */
      } /* End for (j=63999... */
    } /* End for (i=ind... */
    fprintf(fseq2,"-1 //\n");
    filclose(fseq); filclose(fseq2);
    for (i=0; i<ind; i++)
      free(tab[i]);
    free(tab);
  } /* End void */
/****************************************************************************/
						/*************************/
						/*  Programme principal  */
						/*************************/

void main()
  {
    FILE *gp4,*gp7,*fep,*fseq,*fopen();
    char cont[6],rep[3];
    long pos_ab,lgt,**cp7,**fen;
    int **pp7c,**pp7,pos_p7,*pc,i,j,k;
    int **p7s,old7,*seq,np7s,**p8,np7,np7c,nc7,nfen,existe;
    int **pmat,**pmatJ8,**pmat7c,*plancher,*penabonus;
    int depart,fin,dist5,dist;
    int lgseq,test,*pp4,np4,*pp4c,np4c,**cp4,nc4;
    int **ras,nras,nsol,soltot,**psol,**ssol,*result;
    static char nom_sol[28]="yol/";
    static char nomp7[28]="yol/";
    static char nomp4[28]="yol/";
    static char nomens[28]="yol/";
    int lnom,finfich,fen1,fen2,on;
    char dir[DIR_BUFFER_SIZE], nom[FIL_BUFFER_SIZE] ;

    soltot = 0;
    mat_P7(&pmat,&pmatJ8,&pmat7c);
    plancher = (int *) calloc (9, sizeof(int));
    penabonus = (int *) calloc (5, sizeof(int));

    lecture(plancher,penabonus);

    dir[0] = '.' ;  dir[1] = 0 ; 
    *nom = 0 ;
    fep = filqueryopen(dir, nom, "", "r","sequence a traiter")
    if (!fep)
      messcrash("Can't open sequence file") ;

    lnom=strlen(nom);
    for (i=lnom-1; i>lnom-1-4; i--) {
      if (nom[i]=='.') {
	    i--; break;
      }
      if (i==lnom-1-4+1 && nom[i]!='.') {
	    i=lnom-1; break;   /* pas d'extension */
      }
    }
    j=i;
    while(nom[j]!='/' && nom[j]!=':' ) {
      j--;
      if (j==0 && nom[j]!='/') {
	    j--; break;
      }
    }
    i -= j;

    for ( k=0; k<i; k++ ) {
      nom_sol[k+4]=nom[j+1];
      nomens[k+4]=nom[j+1];
      nomp7[k+4]=nom[j+1];
      nomp4[k+4]=nom[j+1];
      j++;
    }
    nom_sol[k+4]='.'; nom_sol[k+5]='s'; nom_sol[k+6]='o';
    nom_sol[k+7]='l'; nom_sol[k+8]='\0';
    nomp4[k+4]='.'; nomp4[k+5]='P'; nomp4[k+6]='4'; nomp4[k+7]='\0';
    nomp7[k+4]='.'; nomp7[k+5]='P'; nomp7[k+6]='7'; nomp7[k+7]='\0';
    nomens[k+4]='.'; nomens[k+5]='e'; nomens[k+6]='n';
    nomens[k+7]='s'; nomens[k+8]='\0';

    printf("\n Nom du fichier solution : %s ",nom_sol);
    printf(" Nom du fichier p4, i.e dessins solutions : %s\n",nomp4);
    printf(" Nom du fichier p7, i.e coordonnees solutions : %s\n",nomp7);
    printf(" Nom du fichier ens : %s\n",nomens);

    if (gp4=fopen(nomp4,"w")) 
      messcrash ("Probleme d'ouverture de fichier %s",nomp4); 
    if (gp7=fopen(nomp7,"w"))
      messcrash ("Probleme d'ouverture de fichier %s ",nomp7);
    
    test=0;
    for (test=0; test<2; test++) {
      if (test==1) {
	compl(nom);
	if ((fep=fopen("c:seqbis.dat","r"))==NULL) {
	  printf("Probleme d'ouverture de fichier seqbis.dat "); exit(0);
	} /* End if */
      } /* End if (test==1) */
      pos_ab=0; dist=0; dist5=0;
      cp7 = (long **) calloc (4, sizeof(long *));
      for (i=0; i<4; i++)
	cp7[i] = (long *) calloc (5000, sizeof(long));
      
      fen = (long **) calloc (2, sizeof(long *));
      for (i=0; i<2; i++)
	fen[i] = (long *) calloc (500, sizeof(long));
      fflush(stdin);
      if (!messprompt(" Distance entre P4 et P7 (entre 100 et 2000)", "1000", "iz"))
	messcrash("Missing distance p4-p7") ;
      freeint(&dist) ;
      printf(" %i\n",dist);
      
      printf(" Distance entre P5 et P5' (entre 10 et 1500; par defaut 500): ");
      if (!messprompt(" Distance entre P5 et P5' (entre 10 et 1500)", "500", "iz"))
	messcrash("Missing distance p4-p7") ;
      freeint(&dist5) ;
      printf(" %i\n",dist5);
      
    if (fseq=fopen("c:seq.dat","w"))
	messcrash ("Probleme d'ouverture de fichier c:seq.dat"); 

		/*  DEBUT DE RECHERCHE DES P7-P7' */

    pos_p7=4300; pos_ab=0; nc7=0; fin=0; lgt=0;

    while (fin!=1) {
	  fin=fenetre(fep,fseq,pos_p7,pos_ab,&pc,&lgseq,lgt,&lgt,test);
		printf("\nlgseq=%i -lg tot = %ld ",lgseq,lgt);

	  if ( fin==1 )
	    P7(&np7,&depart,pc,&pp7,pmat,lgseq-9-20,plancher);
	  else
        P7(&np7,&depart,pc,&pp7,pmat,2000,plancher);
	  P7c(&np7c,&pp7c,pmatJ8,pmat7c,depart,pc,lgseq,plancher);
	  existe=p7_p7c(pc,pp7,pp7c,np7,np7c,nc7,cp7,&nc7,plancher,pos_ab);

 		if (existe==0) {
 		  pos_p7 = 1950;
 		} /* End if (existe==0) */
		else
		  pos_p7 = (int)((*(*(cp7+2)+nc7-1))-50-pos_ab);
	  pos_ab += (long)pos_p7;
		    printf("nbp7=%i - nbp7'=%i - ",np7,np7c);
		    printf("nbc7=%i ",nc7);


      for (i=0; i<2; i++) {
        free(pp7[i]); free(pp7c[i]);
      }
      free(pp7); free(pp7c);


    }  /*  End du fin while  */
            printf(" Fin de Recherche des P7-P7' \n");
    if (nc7 == 0 ) {
      printf(" pas trouve de P7-P7' \n");
      if (test==0) {
        printf("Voulez-vous tester le brin complementaire (oui) ?");
        scanf("%s",rep);
        if (strcmp(rep,"oui")!=0)
          exit(0);
        else {
          filclose(fseq); filclose(fep);
          for (i=0; i<4; i++)
            free(cp7[i]);
          free(cp7);
          for (i=0; i<2; i++)
            free(fen[i]);
          free(fen);
          continue;
        } /* End else */
      } /* End if (test==0) */
      else exit(0);
    } /* End if (nc7 == 0 ) */
    filclose(fseq); filclose(fep);

          /*  RECHERCHE DES FENETRES CORRESPONDANTES  */
/*    fprintf(gp7,"\nnc7 = %i ",nc7);
    for (i=0; i<nc7; i++) {
      fprintf(gp7,"\n nfen %ld -  sc cple = %ld - deb p7 = %ld - deb p7' = %ld ",*(*cp7+i),*(*(cp7+1)+i),*(*(cp7+2)+i),*(*(cp7+3)+i));
      fflush(gp7);
    }  */

      fenet(cp7,fen,lgt,nc7,&nfen,dist);
               printf("nb de fen = %i ",nfen);
    fprintf(gp7,"\nnc7 = %i ",nc7);
    for (i=0; i<nc7; i++) {
      fprintf(gp7,"\n nfen %ld -  sc cple = %ld - deb p7 = %ld - deb p7' = %ld ",*(*cp7+i),*(*(cp7+1)+i),*(*(cp7+2)+i),*(*(cp7+3)+i));
      fflush(gp7);
    }

	          /*  RECHERCHE DES AUTRES ELEMENTS  */
    	          /*  PARCOURS DES FENETRES  */

    if ((fseq=fopen("c:seq.dat","r"))==NULL) {
      printf("Probleme d'ouverture de fichier %s",nom); exit(0);
    }

  seq = (int *) calloc (20000, sizeof(int)); /* alloc. de la sequence */

  old7 = 0; fen1=0; fen2=0; finfich=0;

  for (i=0; i<nfen; i++) {
  					  /* creation du tableau des solutions */
    ssol = (int **) calloc (6, sizeof(int *));
    for (j=0; j<6; j++)
      ssol[j] = (int *) calloc (1000, sizeof(int));
    psol = (int **) calloc (26, sizeof(int *));
    for (j=0; j<26; j++)
      psol[j] = (int *) calloc (1000, sizeof(int));
    result = (int *) calloc (2000, sizeof(int));
    nsol=0;

    on=0; np7s = 0;   /* nombre de couples p7-p7' dans cette fenetre */
    p7s = (int **) calloc (3, sizeof(int *));
    for (j=0; j<3; j++)
      p7s[j] = (int *) calloc (600, sizeof(int));

      			/*   P7-P7' DE CETTE FENETRE  */

    while ( (old7<nc7) && (*(*cp7+old7)==i) ) {
      *(*p7s+np7s) = (int)( *(*(cp7+1)+old7) );
      *(*(p7s+1)+np7s) = (int)( *(*(cp7+2)+old7)-(*(*fen+i)) );
      *(*(p7s+2)+np7s) = (int)( *(*(cp7+3)+old7)-(*(*fen+i)) );
      np7s++;  old7++;
    }  /* End du while (old7<...) */

    for (j=0; j<np7s; j++) {
      fprintf(gp7,"\n sc cple = %i - deb p7 = %i - deb p7' = %i ",
      	*(*p7s+j),*(*(p7s+1)+j),*(*(p7s+2)+j));
      fflush(gp7);
    }

    p8 = (int **) calloc (600, sizeof(int *));
    for (j=0; j<600; j++)
      p8[j] = (int *) calloc (np7s, sizeof(int));

    fprintf(gp7,"\nnc7 = %i, nc7 tot = %i, deb fen = %ld, fin fen = %ld , fin seq = %ld ",np7s,old7,(*(*fen+i)),(*(*(fen+1)+i)),(*(*fen+i))+19999);

                    /* allocation du tableau des P4s */
    cp4 = (int **) calloc (11, sizeof(int *));
    for (j=0; j<11; j++)
      cp4[j] = (int *) calloc (2000, sizeof(int));
    nc4 = 0;
    ras = (int **) calloc (6, sizeof(int *));
    for (j=0; j<6; j++)
      ras[j] = (int *) calloc (2000, sizeof(int));
    nras = 0;
    		/*  SORTIE DE LA SEQUENCE DE CETTE FENETRE  */
    if ( ((*(*(fen+1)+i))-(*(*fen+i))) < 20000 ) {
      on=sequence(on,fen1,i,seq,&finfich,fen,fseq);
      		printf("\n fen %i / %i ",i+1,nfen);
      fen1=i;
      decale(seq,p7s,np7s,p8,plancher);

    for (j=0; j<np7s; j++) {
      fprintf(gp7,"\n cple7 = %i : ",j);
      for (k=0; k<50; k+=5) {
      		fprintf(gp7,"\n sc cple = %i - deb p8 = %ld - deb p8' = %ld ",
      			*(*(p8+k)+j),*(*(p8+k+1)+j)+(*(*fen+i)),*(*(p8+k+3)+j)+(*(*fen+i)));
      		fflush(gp7);
      }
    }
      			printf(" Recherche des P4-P4' \n");
      p4(seq,&pp4,&np4,*(*(p7s+1)+np7s-1));
      p4d(seq,&pp4c,&np4c,*pp4,*(*(p7s+1)+np7s-1));
      p4_p4c(dist5,pp4,pp4c,np4,np4c,cp4,nc4,&nc4,seq,plancher);

                printf(" Nombre de P4-P4' = %i \n",nc4);

    for (j=0; j<nc4; j++) {
      		fprintf(gp7,"\n sc cple = %i - deb p4 = %ld - deb p4' = %ld ",
      			*(*cp4+j),*(*(cp4+2)+j)+(*(*fen+i)),*(*(cp4+4)+j)+(*(*fen+i)));
      		fflush(gp7);
    }

      ens1(seq,dist,p7s,cp4,nc4,np7s,ras,nras,&nras,plancher);
      	    printf(" Nombre de P6-P6' trouves : %i \n",nras);

    for (j=0; j<nras; j++) 
      {
	fprintf(gp7,"\n sc cple = %i - deb p6 = %ld - deb p6' = %ld ",
		*(*(ras+2)+j),*(*(ras+4)+j)+(*(*fen+i)),*(*(ras+5)+j)+(*(*fen+i)));
	fflush(gp7);
      }
      
      printf(" Recherche des solutions globales \n");
      P33(seq,cp4,p7s,p8,ras,nras,ssol,psol,&nsol,plancher,penabonus,result);
      
      printf(" Nbre sol. = %i \n",nsol);
      soltot += nsol;
    } /* End if ( ((*(*(fen+1)+i))-(*(*fen+i))) < 20000 ) */
    else
      printf("\n Fenetre %i trop grande. Diminuez d(P4-P7).",i);
    
    final(ssol,psol,(*(*fen+i)),nsol,seq,nom,gp4,test,result);
    
    for (j=0; j<6; j++)
      free(ssol[j]);
    free(ssol);
    for (j=0; j<26; j++)
      free(psol[j]);
    free(psol);
    free(result);
    for (j=0; j<6; j++)
      free(ras[j]);
    free(ras);
    for (j=0; j<600; j++)
      free(p8[j]);
    free(p8);
    for (j=0; j<11; j++)
      free(cp4[j]);
    free(cp4);
    for (j=0; j<3; j++)
      free(p7s[j]);
    free(p7s);
  } /* End du for (i=0; i<nfen ... */
    for (i=0; i<4; i++)
      free(cp7[i]);
    free(cp7);
    for (i=0; i<2; i++)
      free(fen[i]);
    free(fen); free(fseq);

    filclose(fseq);
    if (test==0) {
      printf("Voulez-vous tester le brin complementaire (oui) ?");
      scanf("%s",rep);
      if (strcmp(rep,"oui"))
        break;
    } /* End if (test==0) */
  } /* End for (test=0; test<2... */

  printf("\n Nbre total de solutions = %i",soltot);
  printf("\nFin du programme\n");
  filclose(gp4);
  filclose(gp7);
}         /* end du main */
