#include <stdio.h>

#include "wac/ac.h"
#include "wh/regular.h"
#include "wh/acedb.h"
#include "wh/mytime.h"
#include "wh/dict.h"
#include "wac/ac_.h"

AC_HANDLE ac_new_handle (void)
{ 
  /*
   * creates a new AC_HANDLE, 
   * freeing the handle will free recursivelly all memory allocated on the handle
   * The handle library was written for acedb by Simon Kelley
   */
  return (AC_HANDLE) handleCreate () ;
}

AC_HANDLE ac_handle_handle (AC_HANDLE parent_handle)
{    
  /*
   * creates a new AC_HANDLE on a parent handle, 
   * freeing the parent_handle will free the
   * new handle and recursivelly all memory allocated on it 
   * The handle library was written for acedb by Simon Kelley
   */
  return (AC_HANDLE) handleHandleCreate ((STORE_HANDLE) parent_handle) ;
}

struct ac_table_internal
{
  int 	magic;
  int	rows;
  /*
   * number of rows in table == public view of arrayMax(lines)
   */

  int	cols;
  /*
   * number of columns in the widest row of the table
   * (tables extracted from objects do not necessarily
   * have the same number of columns in each row )
   */

  /*------------------------------------------------------------*/
  /* before this line, the structure layout must match exactly  */
  /* the layout of struct ac_table in ac.h                      */
  /*------------------------------------------------------------*/
  Array lines ; /* Array of arrays of cells */
  int	def_cols;
  /*
   * this is the default number of columns to use when
   * creating a new row.
   */
  AC_DB db ;
  DICT *text_dict ; 		/* place to store the texts */
  DICT *tag_dict;	/* place to store the tags */

  AC_TABLE parent ;
  int parentRow, parentCol ;
};

typedef struct cell
{
  ac_type	type;
  /*
   * type of data in this cell.  It is
   * ac_type_empty if there is no data in this cell.
   */
  union
  { int	i ;
    float f ;
    KEY   k ;
    mytime_t time ;
    AC_OBJ object;
  } u ;
} AC_CELL ;

static AC_CELL *ac_table_cell (struct ac_table_internal *t, int row, int col);


static void ac_table_finalize (void *tt)
{
  struct ac_table_internal *t;
  int x,y;
  AC_CELL *c;

  t = (struct ac_table_internal *)tt;
  if (!t->magic || t->parent)
    return ;
  for (x=0; x< t->rows; x++)
    {
      for (y=0; y< t->cols; y++)
	{
	  c = ac_table_cell(t, x, y);
	  if (c && (c->type == ac_type_obj))
	    ac_free(c->u.object);
	}
      arrayDestroy( array(t->lines, x, Array ) );
    }
  t->magic = 0;
  arrayDestroy(t->lines);
  dictDestroy(t->text_dict);
  dictDestroy(t->tag_dict);
}


AC_TABLE ac_empty_table (int rows, int cols, AC_HANDLE handle )
{
  struct ac_table_internal *new;
  
  new = halloc (sizeof(struct ac_table_internal), handle);
  blockSetFinalise (new, ac_table_finalize) ;

  /*
   * rows and cols are a hint of the expected buffer size 
   * but we return an empty table, hence size (0,0)
   */
  new->rows = 0 ;
  new->cols = 0 ;
  new->def_cols = cols ;

  /* We allocate to the required size */
  new->lines = arrayHandleCreate (rows, Array, 0) ;

  /*
   * each cell is initialised at zero by arrayCreate which
   * is the correct "nothing here" value for all fields of
   * struct row.
   */
  /*
   * all that is left is the magic number, and to return it
   */
  new->magic = MAGIC_BASE + MAGIC_AC_TABLE;

  return (AC_TABLE) new;
}  /* ac_empty_table */

/*
 * PRIVATE
 */
AC_TABLE ac_db_empty_table (AC_DB db, int rows, int cols, AC_HANDLE handle)
{
  AC_TABLE new = ac_empty_table (rows, cols, handle) ;
  ((struct ac_table_internal *)new)->db = db ;
  return new ;
}

/*
 * PRIVATE
 */
AC_DB ac_table_db (AC_TABLE atable) 
{ return  ((struct ac_table_internal *)atable)->db ; }

/*
 * PRIVATE
 */
static AC_CELL *ac_table_new_cell (struct ac_table_internal *t, int row, int col)
{
  Array line = array (t->lines, row, Array) ; /* automatic reallocation */
  AC_CELL *c ;

  if (!line)
    line = array (t->lines, row, Array) = arrayHandleCreate (t->def_cols, AC_CELL, 0) ;

  c = arrayp (line, col, AC_CELL) ;

  if (arrayMax (line) > t->cols)
    t->cols = arrayMax (line) ;
  if (arrayMax(t->lines) > t->rows)
    t->rows = arrayMax(t->lines) ;
  return c ; /* cannot fail */  
}

/*
 * PRIVATE
 */
static AC_CELL *ac_table_cell (struct ac_table_internal *t, int row, int col)
{
  Array *linep ;
  
  if (! t)
    return 0;
  if (row < 0)
    return 0;
  if (row >= t->rows)
    return 0;
  if (col >= t->cols)
    return 0;
  while (t->parent)
    { 
      row += t->parentRow ; col += t->parentCol ;
      t =  (struct ac_table_internal *) (t->parent) ;
    }
  linep = arrp (t->lines, row, Array);
  if (!linep)
    return 0;
  if (! * linep)
    return 0;
  if (col < 0)
    return 0;
  if (col >=  arrayMax(*linep))
    return 0;

  return arrp(*linep, col, AC_CELL) ;
}

/****************************************************************/
/* PRIVATE: called by ac_parse_buffer
 *  we make no check 
 */
void ac_tableFillUp (AC_TABLE table, int row)
{
  struct ac_table_internal *t =  (struct ac_table_internal *) table ;
  int i, j ;
  AC_CELL *c, *c2 ;
  Array ll, l0 ;
  
  for (i = row + 1 ; i < t->rows ; i++)
    {
      ll = arr (t->lines, i, Array) ;
      l0 = arr (t->lines, i - 1, Array) ;
      for (j = arrayMax (ll) - 1, c = arrp (ll, j, AC_CELL) ; j >= 0 ; c--, j--)
	if (c->type != ac_type_empty) break ; /* find a filled cell */
      for (; j >=0 ; c--, j--)
	if (c->type == ac_type_empty) /* find an empty cell to its left */
	  for (c2 = arrp  (l0, j, AC_CELL) ; j >= 0 ; c--, c2--, j--)
	    *c = *c2 ; /* copy the line above */
    }
}

void ac_table_copydown(AC_TABLE table, int row, int col)
{
  struct ac_table_internal *t = (struct ac_table_internal *)table;
  int prev, x;
  AC_CELL *above, *below;

  prev = row - 1;
  for (x = 0; x< col; x++)
    {
      above = ac_table_cell(t, prev, x);
      below = ac_table_new_cell(t, row, x);
      *below = *above;
      if (above->type == ac_type_obj)
	below->u.object = ac_copy_obj (above->u.object, 0);
    }
}

/****************************************************************/
/* PRIVATE: called by ac_tag_table
 *
 * Implement ac_tag_table().  row, col points at the tag that we are looking
 * for.  That is pparent[row,col] = tag.  We want to find the table in col+1
 * from row to row+n for as many rows as contain tag in [row,col]
 */
AC_TABLE ac_subtable (AC_TABLE pparent, int row, int col, AC_HANDLE handle)
{
  AC_TABLE new = 0 ;
  struct ac_table_internal *parent =  (struct ac_table_internal *) pparent ;
  int i, j, rows, cols ;
  KEY k ;
  AC_CELL *c, *c2 ;
  ac_type type ;

  /* 
   * Subtables have no data of their own.  It is all in the parent.  If we
   * are looking in a subtable, we need to walk up through the parent pointer
   * to find the table with the real data in it.
   */
  rows = cols = 0 ;
  while (parent->parent)
    { 
      row += parent->parentRow ; 
      col += parent->parentCol ;
      parent =  (struct ac_table_internal *) (parent->parent) ;
    }

  /*
   * type is the type of the cell we are pointing at.  We assume that it
   * is a tag.  k is the identity of the tag (currently an int pointing
   * into a dict, but in principle any unique identifier is good).
   */
  c = ac_table_cell (parent, row, col) ;
  type = c->type ; 
  k = c->u.i ;

  /*
   * walk down the rows, starting at the current row.  Find the last
   * row that still has tag k in column col.  While doing that, find
   * the widest row that we have.
   */
  for (i = 0 ; ; i++)
    {
      /*
       * if the cell in the next row is still the same thing, 
       */
      c2 = ac_table_cell (parent, row + i, col) ;
      if (c2 && c2->type == type && c2->u.i == k)
	{
	  rows++ ;
	  /*
	   * find the max column in that row
	   */
	  for (j = cols ; ; j++) 
	    {
	      c2 = ac_table_cell (parent, row + i, col + j) ;
	      if (!c2 || c2->type == ac_type_empty)
		break ;
	      if (j >= cols) cols = j + 1 ;
	    }
	}
      else
	{
	  /*
	   * stop when we find a different tag in the first column.
	   */
	  break ;
  	}
    }

  /*
   * I don't understand why this is conditional.  It looks like
   * rows and cols are both guaranteed to be at least 1 because of 
   * the code above.
   */
  if (rows && cols)
    {
      new = ac_copy_table (pparent, row, col+1, rows, cols, handle);
    }
  return (AC_TABLE) new;
}


AC_TABLE ac_copy_table (AC_TABLE xfrom, int row, int col, int copy_rows, int copy_cols , AC_HANDLE handle)
{
struct ac_table_internal *from =  (struct ac_table_internal *) xfrom ;
AC_TABLE new;
int i, j;

/*
* copy a square area out of a table into a new table
*/
new = ac_empty_table(copy_rows, copy_cols, handle);

for (i=0; i < copy_rows; i++)
for (j=0; j < copy_cols; j++)
  {
    /*
      new[i,j] = old[i+row, j+col]
    */
    AC_CELL *c;
    c = ac_table_cell(from, i+row, j+col);
    if (! c)	
      continue;
    switch (c->type)
      {
      case ac_type_empty:
	break;
      case ac_type_bool:
      case ac_type_int:
      case ac_type_float:
      case ac_type_date:
	ac_table_insert_type(new , i, j, &c->u, c->type);
	break;
      case ac_type_text:
	{
	  char *s = dictName (from->text_dict, c->u.i) ;
	  ac_table_insert_type(new , i, j, &s, c->type);
	}
	break;
      case ac_type_tag:
	{
	  char *s = dictName (from->tag_dict, c->u.i) ;
	  ac_table_insert_type(new , i, j, &s, c->type);
	}
	break;
      case ac_type_obj:
	{
	  /*
	   * do not need to ac_copy_obj () the c->u.object here because
	   * ac_table_insert_type does that internally.
	   */
	  ac_table_insert_type(new , i, j, &c->u.object, c->type);
	}
	break;
      }
  }
return new;
}




ac_type ac_table_type ( AC_TABLE tbl, int row, int col )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  return c ? c->type : ac_type_empty;
} /* ac_table_type */


/*
 * to get values from a (row,col) of the table, we have a common
 * theme:  if the type of the cell is right, return the value,
 * else return a default.  Note that if the type of the cell is
 * not ac_type_empty, then you know that the cell exists, so you
 * do not need any array checking when fetching the value.
 */

BOOL ac_table_bool ( AC_TABLE tbl, int row, int col, int deflt )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  if (c && c->type == ac_type_bool)
    return c->u.k ? TRUE : FALSE ;
  return deflt;
}

int ac_table_int( AC_TABLE tbl, int row, int col, int deflt )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  if (c && c->type == ac_type_int)
    return c->u.i ;
  return deflt;
}

char * ac_table_tag ( AC_TABLE tbl, int row, int col, char *deflt )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  if (c && c->type == ac_type_tag)
    return dictName (t->tag_dict, c->u.i) ;
  return deflt;
}


float ac_table_float( AC_TABLE tbl, int row, int col, float deflt )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  if (c && c->type == ac_type_float)
    return c->u.f ;
  return deflt;
}

mytime_t ac_table_date ( AC_TABLE tbl, int row, int col, mytime_t deflt )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  if (c && c->type == ac_type_date)
    return c->u.time ;
  return deflt;
}

char * ac_table_text( AC_TABLE tbl, int row, int col, char * deflt )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  if (c && c->type == ac_type_text )
    return dictName (t->text_dict, c->u.i) ;
  else if (c && c->type == ac_type_obj && !strcmp (ac_class(c->u.object),"Text"))
    return ac_name(c->u.object) ;
  return deflt;
}

AC_OBJ ac_table_obj( AC_TABLE tbl, int row, int col, AC_HANDLE handle)
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;

  if (c && c->type == ac_type_obj)
    return ac_copy_obj (c->u.object, handle);
  return NULL;
}

/*
 * inserting data into the table
 */

int
ac_table_insert_type(AC_TABLE tbl, int row, int col, void *dat, ac_type type)
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_new_cell (t, row, col) ;

  c->type = type;
  if (dat)
    switch (type)
      {
      case ac_type_empty:
	break;
      case ac_type_bool:
	c->u.k = *(BOOL*)dat ? TRUE : FALSE ;
	break;
      case ac_type_int:
	c->u.i = *(int *)dat;
	break;
      case ac_type_float:
	c->u.f = *(float *)dat;
	break;
      case ac_type_text:
	{
	  char *cp = *(char**) dat ;
	  if (! t->text_dict) 
  	    t->text_dict=dictCaseSensitiveHandleCreate(200, 0);
	  if (cp)
	    dictAdd (t->text_dict, cp, &(c->u.i)) ;
	}
	break;
      case ac_type_date:
	c->u.time = *(mytime_t*) dat ;
	break;
      case ac_type_tag:
	{
	  char *cp = *(char**) dat ;
	  if (! t->tag_dict) 
  	    t->tag_dict=dictHandleCreate(200, 0);
	  if (cp)
	    dictAdd (t->tag_dict, cp, &(c->u.i)) ;
	}
	break;
      case ac_type_obj:
	c->u.object = ac_copy_obj (* (AC_OBJ *)dat,0);
	break;
      default:
	messcrash ("bad type provided to ac_table_insert_type") ;
      }
  return TRUE ; /* we garantee success */
} /* ac_table_insert_type */

/*
 * an easier to use insert for text.
 */

int ac_table_insert_text (AC_TABLE tbl, int row, int col, char *value)
{
  return ac_table_insert_type (tbl, row, col, (void *)&value, ac_type_text );
}

/*
 * a function to help in displaying a table.  I originally considered
 * a complete print_table() with callbacks, but it would be so complicated
 * to use that it is not worth doing.  This converts a single cell
 * to a string.
 */

char * ac_table_printable (AC_TABLE tbl, int row, int col, char *deflt )
{
  struct ac_table_internal *t = (struct ac_table_internal *) tbl;
  AC_CELL *c = ac_table_cell (t, row, col) ;
  static char b[100];

  if (c)
    switch (c->type)
      {
      case ac_type_empty:
	return deflt;
      case ac_type_bool:
	return c->u.k ? "TRUE" : "FALSE" ; 
      case ac_type_int:
	sprintf(b,"%d", c->u.i);
	return b;
      case ac_type_float:
	sprintf(b,"%g", c->u.f);
	return b;
      case ac_type_text:
	return dictName (t->text_dict, c->u.i) ;
      case ac_type_date:
	return timeShow (c->u.time) ;
      case ac_type_tag:	
	return dictName (t->tag_dict, c->u.i) ;
      case ac_type_obj:
	return ac_name(c->u.object);
      default:
	messcrash ("bad type in ac_table_printable") ;
      }
  return deflt ;
}


FILE *ac_print_table_fp = 0;

/*
* This function is really just here for use in the debugger.
*/
int ac_print_table(FILE *fp, AC_TABLE t)
{
	int x,y;
	if (!fp)
		fp = ac_print_table_fp;
	if (!fp)
		fp = stderr;
	for (x=0; x<t->rows; x++)
		{
		for (y=0; y< t->cols; y++)
			fprintf(fp,"%d %d -%s-\n",x,y,ac_table_printable(t,x,y,""));
		fprintf(fp,"\n");
		}
	return 0;
}
