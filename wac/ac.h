/*
*
*

freeing data: 

 The rule is that any time you have any AC_* data item, you must
 ac_free it.  When you ac_free it, it becomes immediately invalid
 and all memory dependant on that object also dies.

 This applies to all data types with names beginning AC_ :
	AC_DB
	AC_OBJ
	AC_ITER
	AC_TABLE
	AC_KEYSET

 All of these items MUST be freed by passing them to ac_free

 This does not apply to data types with names beginning ac_ :
	ac_type
	enum ac_tablemaker_where


Objects and lazy loading

 The AC_OBJ is the only object identity exposed in this interface.
 Objects contain data which is lazily loaded.  The minimal object
 content is just a class name and object name.  Touching the data
 inside the object will cause the object data to be loaded.

 Some functions have a fillhint parameter.  If this parameter is true,
 the objects they return will have the data filled already.  From the
 user perspective, it doesn't matter because the lazy loading will
 guarantee that the data is there when you need it, but using fillhint
 can help efficiency.  fillhint=1 reduces the number of transactions
 needed to load the data because the object identity and data come at
 the same time.  fillhint=0 reduces the size of the transaction because
 only the object identity is present in the server response.  For
 small objects, the difference is negligible; for large objects, the
 difference can be substantial.

 fillhint parameters are available on the iterator constructors
 ac_iter_query() and ac_iter_keyset(), and the get-by-name function
 ac_get().  It is not present on other functions that return objects
 because those objects have already been fetched, so it is too late
 to gain an advantage over lazy loading.
 
*
*
*/

#ifndef AC_H_DEF
#define AC_H_DEF
 
#include <wh/regular.h>
#include <wh/mytime.h>


typedef struct ac_db *AC_DB;
	/*
	* a descriptor for an open database - freeing it
	* closes the database
	*/

typedef struct ac_object *AC_OBJ;
	/*
	* an object that you found in the database
	*/

typedef struct ac_iter *AC_ITER;
	/*
	* iterates over keysets
	*/

typedef struct ac_keyset *AC_KEYSET;
	/*
	* a set of objects - created by a query, or by set operations
	* on other keysets
	*/


typedef struct ac_table
	{
	int private;	
		/*
		* magic number used internally by library
		*/

	int	rows;		
		/*
		 * number of rows in table
		 */

	int	cols;
		/*
		 * number of columns in the widest row of the table
		 * (tables extracted from objects do not necessarily
		 * have the same number of columns in each row)
		 */
	}
 * AC_TABLE;
	/*
	 * a row/colum representation of object data or query results
	 */

typedef enum	
	{ 
	ac_type_empty = 0,        /* no data present - i.e. error */
        ac_type_bool,
	ac_type_int,         
	ac_type_float, 
	ac_type_text,
	ac_type_date,        /* acedb date encoding */
	ac_type_tag,
	ac_type_obj
	}
ac_type;
	/*
	 * all the data types that may exist in an object
	 */
typedef STORE_HANDLE AC_HANDLE ;
        /*
	 * all memory allocated on a AC_HANDLE is freed by a single call to ac_free
	 * all the AceC funtion allocating memory take AC_HANDLE as their last parameter
	 * this greatly simplifies memory managment in the client code 
	 */
AC_HANDLE ac_new_handle (void) ;
        /*
	 * creates a new AC_HANDLE, 
	 * freeing the handle will free recursivelly all memory allocated on the handle
	 * The handle library was written for acedb by Simon Kelley
	 */
AC_HANDLE ac_handle_handle (AC_HANDLE parent_handle) ;
        /*
	 * creates a new AC_HANDLE on a parent handle, 
	 * freeing the parent_handle will free the
	 * new handle and recursivelly all memory allocated on it
	 */

#define ac_free(__vp) messfree(__vp)
	/*
	 * general purpose free anything function - including a AC_HANDLE
	 * note that you do NOT
	 * need to cast the data type to (void *) when passing it; that
	 * is done implicitly.
	 * use the ac_free macro, it resets the argument to zero
	 */

/*
* opening the database
*/

AC_DB ac_open_db (char *database, char **error) ;
	/*
	* how to get access to a database - returns a descriptor
	* that can be used to make queries of the database
	*
	* database is one of
	*
	*	r:hostname:program_number
	*	rpc:hostname:program_number
	*		contact an RPC ace server at the given host
	*		and program number.  e.g. r:annie:2000100
	*
	*	a:hostname:port
	*	acetcp:hostname:port
	*		contact an AceTCP ace server at the given host
	*		and port number.  e.g. acetcp:cerleris:12345
	*
	*	s:hostname:port:username/password
	*	socket:hostname:port:username/password
	*		contact a Sanger "socket server" at the given
	*		host and port, using username/password for
	*		authentication  
	*		(much of the server side is not implemented)
	*
	* For all of the above, hostname can be blank to indicate "localhost"
	*
	*	hostname:port
	*		assumed to be old-style RPC server specifier - for
	*		compatibility when updating old scripts
	*
	*
	* *error is set to zero in case of success
	*        is set to a constant error string otherwise
	*        do NOT free the returned error.
	*/

/*
 * closing the database
 */

#define ac_db_close(__db) ac_free(__db)
/* All objects, tables  and functionalities associated to the database 
 * become invalid, including the function ac_name () 
 * it is implemented as a macro so that __db it self is reset to zero
 */

void ac_db_refresh (AC_DB db) ;
/* Calling ac_db_refresh insures that all objects in the client cache
 * are now considered invalid
 * any future call to ac_get_obj will get a new version from the server
 *
 * ac_db_refresh should be called after a call to ac_parse
 * to be sure that the modified data will be visible on the client side
 */
/**********************************************************************
* Keyset operations - note that keysets exist inside a database.
* You can't move a keyset from one database to another, but you
* can add objects with the same names to a keyset in another 
* database.
*
* There are four kinds od constructors:
*	new/copy   : create an empty keyset
*	query      : get the result of the query (the most frequent operation)
*	command    : get the active set following any acedb command
*	read/write : into an external file
* Several modifiers
*       and/or/minus/xor : boolean operations on 2 keysets
*       add/remove       : a single object to the keyset
* Several Utilities and conversion tools
*       keyset_count : the number of objects in the keyset
*       keyset_table : creates a one column table from the keyset
*       keyset_iter  : creates an iterator to loop over the keyset
*/

AC_KEYSET ac_new_keyset (AC_DB db, AC_OBJ obj, AC_HANDLE handle) ;
	/*
	* makes an empty keyset to use keyset operators on.
	*
	* If obj is not NULL, it is added to the keyset.  (It seems
	* to be quite common to create a keyset containing a single
	* object.)
	*/

AC_KEYSET ac_copy_keyset (AC_KEYSET aks, AC_HANDLE handle) ;
	/*
	* makes another keyset with identical contents.  Because
	* you don't have the 3 operand instructions, you copy the
	* keyset and then use read-modify-write operations on it.
	*/

AC_KEYSET ac_dbquery_keyset (AC_DB db, char *queryText, AC_HANDLE handle) ;
	/*
	* performs a query over the whole database, creates a keyset of the result
	*/

AC_KEYSET ac_ksquery_keyset (AC_KEYSET initial_keyset, char *queryText, AC_HANDLE handle) ;
	/*
	* performs a query over the initial keyset,  creates a keyset of the result
	*/

AC_KEYSET ac_objquery_keyset (AC_OBJ aobj, char *queryText,  AC_HANDLE handle) ;
	/*
	* performs a query initialized on a single object, creates a keyset of the result
	*/

AC_KEYSET ac_command_keyset (AC_DB db, char *command, AC_KEYSET iks,
	AC_HANDLE handle) ;
	/*
	* execute an ACEDB command and make a keyset of whatever the active
	* keyset is at the end of the command.  If iks is not NULL, it
	* is made the active keyset before executing the command.
	*/

AC_KEYSET ac_read_keyset (AC_DB db, char *name, AC_HANDLE handle);
BOOL  ac_keyset_write (AC_KEYSET aks, char *name);
	/*
	* read/write keyset in non-volatile storage.  In the current
	* client implementation, name is a file name on the server's
	* disk.
	* ac_write writes a standard ace file, 
	*   the name of the file becomes the name of the keyset
        *   returns FALSE if !aks OR if the file cannot be opened
	* ac_read only imports the recognised keys
 	*
	* ac_read_keyset returns NULL if there is no file with that name.
	*/

int ac_keyset_and (AC_KEYSET aks1, AC_KEYSET aks2) ;
int ac_keyset_or (AC_KEYSET aks1, AC_KEYSET aks2) ;
int ac_keyset_xor (AC_KEYSET aks1, AC_KEYSET aks2) ;
int ac_keyset_minus (AC_KEYSET aks1, AC_KEYSET aks2) ;
	/*
	* perform boolean operation aks1 = aks1 OP aks2
	* on a keyset.
	* returns the number of element in the result
	*
	* 3 operand instructions might look more reasonable, but 
	* then you have to free a lot more keysets
	*/

BOOL ac_keyset_add (AC_KEYSET aks1, AC_OBJ obj) ;
BOOL ac_keyset_remove (AC_KEYSET aks1, AC_OBJ obj) ;
	/*
	* add/remove object in keyset 
	*
	* If ac_keyset_add() is given NULL for the keyset, it 
	* creates a new keyset containing only the single object.
	*
	*/
AC_KEYSET ac_subset_keyset (AC_KEYSET aks, int x0, int nx, AC_HANDLE handle) ;
        /*
	 * returns a subset strating at x0 of length nx
	 * index start at zero and are ordered alphabetically
	 * which is usefull to get slices for display
	 */
int ac_keyset_count (AC_KEYSET aks);
	/*
	* return the number of keys in a keyset
	*/

/**********************************************************************/
/***********************************************************************
* Iterator functions
*/

AC_ITER ac_query_iter (AC_DB db, int fillhint, char *query, AC_KEYSET initial_keyset, AC_HANDLE handle) ;
	/*
	* make a query, dump resulting keyset directly into an iterator
	*	db = database to query
	* 	fillhint = true means load object data during iteration,
	*		false means use lazy data loading
	*	query = query to use
	*	initial_keyset = keyset to query in
	*/

AC_ITER ac_dbquery_iter (AC_DB db, char *query, AC_HANDLE handle) ;
	/*
	* Short hand 1 of previous function, fillhint = true by default
	*	db = database to query
	*	query = query to use
	*/

AC_ITER ac_ksquery_iter (AC_KEYSET ks, char *query, AC_HANDLE handle) ;
	/*
	* Short hand 2 of previous function, fillhint = true by default
	*	this time the query is initialised on the keyset ks
	*	query = query to use
	*/

AC_ITER ac_objquery_iter (AC_OBJ obj, char *query, AC_HANDLE handle) ;
	/*
	* Short hand 3 of previous function, fillhint = true by default
	*	this time the query is initialised on the single object obj
	*	query = query to use
	*/

AC_ITER ac_keyset_iter (AC_KEYSET aks, int fillhint, AC_HANDLE handle) ;
	/*
	* must pick a name
	*/
	/*
	* create an iterator that loops over a keyset
	*	aks = the keyset to loop over
	* 	fillhint = true means load object data during iteration,
	*		false means use lazy data loading
	*/

AC_OBJ ac_next_obj (AC_ITER i) ;
	/*
	* fetch the next object in the keyset being iterated over.
	* The returned object must be free by the user
	*/

BOOL ac_iter_rewind (AC_ITER i) ;
	/*
	* set the iterator back to the beginning
	*/

/*
* Table related functions
*
* Three constructors:
*	tablemaker query
*	aql query
*	extract data from object
*/
AC_TABLE ac_keyset_table (AC_KEYSET aks, int min, int max, int fillhint, AC_HANDLE handle) ;
	/*
	* create a table of objects from the keyset.  There is one row per object.  Each
	* row contains an object in column 0.
	*
	* min, max are the minimum and maximum objects that you want to get.  The objects
 	* appear in rows numbered from min to max.  That is, if min=10 and max=15, then
 	* the resulting table has objects in rows 10 to 15, but rows 0 to 9 are empty.
	* max = -1 implies all objects.
	*
	* fillhint indicates whether the objects' data should be loaded.
	*
 	*/

enum ac_tablemaker_where
	{ 
	ac_tablemaker_db, 
	ac_tablemaker_file, 
	ac_tablemaker_server_file, 
	ac_tablemaker_text 
	};

	/*
	* ac_tablemaker_db => query is name of tablemaker query in database
	* ac_tablemaker_file => query is file in the client disk space
	* ac_tablemaker_server_file => query is file name known by server
	*		Not a particularly secure feature
	* ac_tablemaker_text => query is the actual text of a tablemaker
	*	query that you read into your source code.
	*/

/* Complex queries */
AC_TABLE ac_tablemaker_table (AC_DB db, char *query, AC_KEYSET initial_keyset,
	 enum ac_tablemaker_where where,
	 char *parameters , AC_HANDLE handle) ;
	/* 
	* perform tablemaker query:
	*	db is the database
	* 	query is the query
	*	initial_keyset is a keyset to begin the query from
	*		(i.e. like table-maker -active), or NULL to 
	*		begin without a keyset
	* 	where describes how to interpret the value of query
	*	parameters are parameters to substitute in the query
	*/

AC_TABLE ac_aql_table (AC_DB db, char *query, AC_KEYSET initial_keyset, AC_HANDLE handle) ;
	/*
	* perform AQL query
	*/

AC_TABLE ac_tag_table (AC_OBJ obj, char *tag, AC_HANDLE handle) ;
	/*
	* fetch a table from a tag in an object
	*/

AC_TABLE ac_empty_table( int rows, int cols, AC_HANDLE handle) ;
	/*
	* make an empty table.  You can insert data with ac_table_insert.
	* rows and cols are the initial size, but the table will grow if
	* you exceed those limits.  You do not need to open the database
	* to have a table.
	*/

AC_TABLE ac_copy_table (AC_TABLE from, int row, int col, int copy_rows, int copy_cols , AC_HANDLE handle) ;
	/*
	* copy a square area out of a table into a new table.
	*/

ac_type	ac_table_type (AC_TABLE tbl, int row, int col) ;
	/*
	* return the type of the data item at row, col
	* or ac_none if there is no entry at that position
	*/

char * ac_table_tag (AC_TABLE atbl, int row, int col, char * deflt) ;
	/*
	* returns the value of the tag at row, col
	* of deflt if there is no data or the data is not a tag
	*/

int ac_table_int (AC_TABLE tbl, int row, int col, int deflt) ;
	/*
	* return the integer at row, col
	* or deflt if there is no data at that position or the data is
	* not an int
	* (should it convert floats?)
	*/

float 	ac_table_float	( AC_TABLE tbl, int row, int col, float  deflt) ;
	/*
	* return the float at row, col
	* or deflt if there is no data at that position or the data is
	* not a float
	* (should it convert ints?)
	*/

char *	ac_table_text ( AC_TABLE tbl, int row, int col, char * deflt) ;
	/*
	* return the string at row, col
	* or deflt if there is no data at that position or the data is
	* not a string
	*
	* The return value is a pointer into another data object.  It
	* is valid at least until the AC_TABLE object is freed.
	*/

mytime_t ac_table_date	( AC_TABLE tbl, int row, int col, mytime_t deflt) ;
	/*
	* fetch a date out of the table
	*/

AC_OBJ 	ac_table_obj	( AC_TABLE tbl, int row, int col, AC_HANDLE handle) ;
	/*
	* fetch an object out of the table
	*/

char * ac_table_printable( AC_TABLE, int row, int col, char *dflt) ;
	/*
	* return a printable string of whatever is in the table
	* at that position.  The return value may be a static value
	* or may be a pointer into data in the table.
	*/

/*
* Additional interfaces to make tables more convenient to use and display
*/

int ac_table_insert_text (AC_TABLE tbl, int row, int col, char *value) ;

	/*
	* creates or overwrites a cell in a table with a particular
	* value.  You would use this to mess with a table before
	* printing it.  Only char * types can be inserted into the
	* table; the resulting field type is TEXT.
	*
	* free is true if the string should be freed when the table is
	* freed, of false if the user will free the string when necessary.
	*
	* returns 0 on success, 1 on memory allocation failure.
	*/

int ac_table_insert_type (AC_TABLE tbl, int row, int col, void *dat, 
	ac_type type) ;
	/*
	* inserts an arbitrary type into a cell of the table.
	*/


/*
* object related functions 
*/

AC_OBJ ac_get_obj (AC_DB db, char *class, char *name, AC_HANDLE handle) ;
	/*
	* get an object by name
	* Returns the object or NULL if the object
	* does not exist.
	*/

AC_OBJ ac_copy_obj (AC_OBJ obj, AC_HANDLE handle) ;
	/*
	* gets another "instance" (really just a reference) of the object - 
	* both can (and must) be freed 
	*/

/*
* data related functions
*/

char *	ac_name	(AC_OBJ obj) ;
char *	ac_class (AC_OBJ obj) ;
	/*
	* return value is a pointer into the object - the string goes
	* away when the object goes away
	*/

/*
* Note that the functions that use tags can ONLY see tags that are
* in the object model.  These functions CANNOT see tags that are
* inside a #xxx construct.
*/

BOOL ac_has_tag	( AC_OBJ obj, char * tag) ;
	/*
	* returns true or false
	*/

ac_type ac_tag_type 	( AC_OBJ obj, char *tag) ;
	/*
	* returns the type of the data after the tag, or ac_none
	* if the tag is not there or there is no data item there
	*/

int	ac_tag_int 	( AC_OBJ obj, char *tag, int    deflt) ;
	/*
	* returns integer after the tag, or deflt if the tag
	* is not there or the data is not an integer
	* (Should it convert float?)
	*/

float	ac_tag_float	( AC_OBJ obj, char *tag, float  deflt) ;
	/*
	* returns float after the tag, or deflt if the tag
	* is not there or the data is not a float
	* (Should it convert int?)
	*/

char *	ac_tag_text	( AC_OBJ obj, char *tag, char * deflt) ;
	/*
	* returns text after the tag, or deflt if the tag
	* is not there or the data is not a KEY or a text
	*
	* return value is pointer into the object - the string goes away
	* when the object goes away
	*/

mytime_t ac_tag_date 	( AC_OBJ obj, char *tag, mytime_t deflt) ;
	/*
	* return the date after the tag, or default if the tag
	* is not there or the data is not a date
	*/

AC_OBJ  ac_tag_obj 	( AC_OBJ obj, char *tag, AC_HANDLE handle) ;
	/*
	* return the object after the tag, or NULL if the tag
	* is not there or the data is not an object
	*/


char * ac_tag_printable( AC_OBJ obj, char *tag, char *dflt) ;
	/*
	* return a printable string of whatever is in the table
	* at that position.  The return value may be a static value
	* or may be a pointer into data in the object.
	*/

/*
* type A object
*  In addition to B classes, with tags and values declared in the schema
*  acedb handles 'byte buffers' called A type objects
*  They are declared in whooks/quovadis.c togetehr with dedicated data 
*  handling functions. 
*  The most important A classes are DNA, Peptide and LongText
*  We provide here an AceC interface for these 3 types.
*  
*/

char *ac_obj_dna (AC_OBJ obj, AC_HANDLE h) ;
char *ac_zone_dna (AC_OBJ aobj, int x1, int x2, AC_HANDLE h) ;
/*
 * DNA
 *      returns a straing of ATGC
 *      ac_zone_dna returns a subpart, in biologist coord
 *        ask x1 = 1 to obtain the first base of the sequence
 *        if x1 > x2, the sequence is reverse complemented before transfer
 *
 *      if (x1 || x2) < 1 OR > max(dna), the function returns NULL
 *      it would be much better to extrapolate automatically in the parent object
 *
 *      The object must belong to an acedb class supporting the DNA interface
 *      like DNA, Sequence, mRNA, i.e. any class containg the DNA tag, or contain
 *      the SMAP sequence-hierachy tag. 
 */


char *ac_obj_peptide (AC_OBJ obj, AC_HANDLE h) ;
/*
 * Peptide
 * the object must be a protein or a CDS
 *
 */

char *ac_longtext (AC_OBJ aobj, AC_HANDLE h) ;
/*
 * LongText
 * in particulat paper abstracts
 * return the whole abstarct as a single char *
 * either the return valued, or the handle should be passed to ac_free
 */

/*  
  unsigned char * ac_a_data (AC_OBJ obj, int *len, AC_HANDLE h) ;

   * INCOMPLETE unless you add code in dump.c !!
   * nothing will get exported.

 * other type A object
 *
 * If the object is of type A :
 *  - The returned value is the an array of bytes, allocated on handle
 *  - If len is not NULL, it is filled in with the number of bytes.
 *  - There is a \0 after the returned string (not included in the length).
 * Otherwise:
 *  - returns NULL and *len is undefined.
 *
 * The content of of the returned array is not specified, in
 * practice it is "whatever show -C does" and usually contains
 * formating characters that should be cleansed in the client.
 *	
 * Properly speaking, this should not be considered as a public function
 * but as a startup point if you want to handle a new type of A data.
 * A data types are declared on the server side in whooks/quovadis.c
 * and require registering dedicated code. In the same way one
 * should register here new AceC code to handle the specifics of that
 * class and new dumper code in w4/dump.c around:	case 'C': // C dumping 
 */

/*
* raw command
*/
unsigned char * ac_command ( AC_DB db, char *command, int *result_length,
	AC_HANDLE handle) ;
	/*
	* pass a command to the database and return the output that
	* would be produced by tace.
	*
	* command is the command text.  It is a single line of text.
	* if result_length is not null, it is filled in with the
	* length of the result.  The return value is the text of
	* the server response (i.e. up to where tace would have
	* printed the next prompt).
	*
	* ac_free() the returned string when you are finished with it.
	*/

/*
* enter data into database - this is the only write operation in Ace C
*/

BOOL ac_parse (AC_DB db, 
	       char *text, 
	       char **error_messages,
	       AC_KEYSET *aks,
	       AC_HANDLE h) ;
	/*
	* parse a string containing data in .ace format; return a keyset and
	* the text of the result of the parse command.
	* 
	* if error_count is not NULL, it is filled with the number of errors.
	* The value is unchanged if you did not get write access.
	*
	* if error_messages is not NULL, it is filled with a pointer to the
	* error messages produced by the database.  ac_free() the data when 
	* you are finished with it.
	*
	* both the returned keyset and the error message text are allocated
	* on handle h.
	*
	* each call to ac_parse_string() is independent.  text must contain
	* a complete .ace file -- objects cannot be split across multiple
	* calls.
	*
	*/

#endif /*  AC_H_DEF */
