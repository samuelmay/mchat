#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "skiplist.h"

/***************************************************************************
 *                      L O C A L  D E F I N E S
 ***************************************************************************/

#define SKIPLIST_DEFAULT_BLOCK_SIZE    4096
#define SKIPLIST_NODE_NEAREST_UNIT(n)  ((n) + ((~((n) & 3) + 1) & 3))

#define SKIPLIST_NODE_UNIT_SIZE(ns)    (SKIPLIST_NODE_NEAREST_UNIT((ns)))

#define SKIPLIST_UNIT_MULT    (4)

#define SKIPLIST_NODE_SIZE(l)         (sizeof(SkipListNode_t) + (l) * sizeof(SkipListNode_t *))

#define SKIPLIST_NODE_UNIT_LEVEL(l)    (SKIPLIST_NODE_UNIT_SIZE(SKIPLIST_NODE_SIZE(l)) / SKIPLIST_UNIT_MULT  - 1)

#define SKIPLIST_MAX_NODE_SIZE         (SKIPLIST_NODE_SIZE(SKIPLIST_MAXLEVEL))

#define SKIPLIST_MAX_UNIT_SIZE        (SKIPLIST_NODE_UNIT_SIZE(SKIPLIST_MAX_NODE_SIZE))

#define SKIPLIST_NUM_UNITS            (SKIPLIST_MAX_UNIT_SIZE / SKIPLIST_UNIT_MULT)

/***************************************************************************
 *                      F O R W A R D  D E C L A R A T I O N S
 ***************************************************************************/
static int getNewLevel( int );
static SkipListNode_t *newSkipListNode( SkipList_t *, void *, void *, int );
static void * skipListNodeAlloc( SkipList_t *list, int level );
static void skipListNodeFree( SkipList_t *list, void *node );

/***************************************************************************
 *                            F U N C T I O N S
 ***************************************************************************/

/***************************************************************************
 * Function    : SkipListAlloc()
 * Description : Allocates a new skip list. 
 * Input       : Pointer to the compare function which is used to compare 
 *               the keys and a pointer to the free function which is used
 *               to free the element stored in each skip list node. 
 * Output      : None.
 * Returns     : Pointer to the newly allocated skip list.
 * ***************************************************************************/
SkipList_t *
SkipListAlloc( SkipListCmp_t cmpFn, SkipListFree_t freeFn )
{
    int        i = 0;
    SkipList_t *list = NULL;

    /*
     * We cannot proceed without the user functions. 
     */
    if ( cmpFn == NULL || freeFn == NULL )
        return( NULL);
    /*
     * Allocate memory for the new skip list. 
     */
    i = sizeof(SkipList_t);
    if ( (list = (SkipList_t *)malloc( i )) == NULL )
        return( NULL );
    memset( list, 0, i ); 
    /*
     * Set the initial random level for the list and
     * initialize various list variables. 
     */
    srand( (unsigned long)time(NULL) );
    list->level  = 0;
    list->cmpFn  = cmpFn;
    list->freeFn = freeFn;
    list->numNodes = 0;
    list->maxLevels = SKIPLIST_MAXLEVEL - 1;
    list->header = newSkipListNode( list, (void *)0, (void *)0, list->maxLevels );
    
    return( list );
}

/***************************************************************************
 * Function    : SkipListFree()
 * Description : Frees the given skip list. 
 * Input       : Pointer to the skip list to be freed.
 * Output      : None.
 * Returns     : None. 
 ****************************************************************************/
void
SkipListFree( SkipList_t *list ) 
{
    register SkipListNode_t *p, *q;
    int i;

    /*
     * Go through the list and delete all nodes. 
     */
    p = list->header->forward[0];

    while ( p ) {
 
        q = p->forward[0];

        /*
         * Call the user registered free function in order
         * to free the element stored in this node. 
         */

        list->freeFn( p->value );

        /*
         * Now, free this node. 
         */
        skipListNodeFree( list, p );

        p = q;

    }
   
    /*
     * Note: the header was malloc'ed not user alloced, so do a free
     */
 
    free(list->header);

    /*
     * All the nodes are freed and hence free the list. 
     */
    free( list );
}

/***************************************************************************
 * Function    : SkipListInstert()
 * Description : Inserts the given element in the skip list at
 *               the right place based on the specified key. 
 * Input       : The list in which the element is to be inserted, key,
 *               element to be inserted and a flag which indicates
 *               whether to replace the existing element, if any. 
 * Output      : None.
 * Returns     : 0 -- success, -1 -- failure
 * ***************************************************************************/
int
SkipListInsert( SkipList_t *list, void *key, void *value, int replace ) 
{
    int i = 0;
    int newLevel = 0;
    SkipListNode_t *x= NULL;
    SkipListNode_t *update[SKIPLIST_MAXLEVEL];
    
    /*
     * Scan all levels while (list-key<serach-key) starting
     * with header in its level. 
     */
    for ( x=list->header, i=list->level; i >= 0; i-- ) {
        while ( x->forward[i] != 0 &&
                list->cmpFn(x->forward[i]->key, key) < 0 ) {
            x = x->forward[i];
        }
        /*
         * Save level pointer. 
         */
        update[i] = x;
    }
    x = x->forward[0];
    /*
     * Element already exists. 
     */
    if ( x && list->cmpFn(x->key, key) == 0 ) {
        if ( replace ) {
            list->freeFn( x->value );
            x->key = key;
            x->value = value;
            return( 0 );
        }
        else
            return( -1 );
    }
    /*
     * Put a new element in the list for this key.
     * Get the new level and fix the list level.
     */
    newLevel = getNewLevel( list->maxLevels );
    /*
     * Adjust the header level. 
     */
    if ( newLevel > list->level ) {
        for ( i=list->level+1; i<=newLevel; i++ ) {
            update[i] = list->header;
        }
        list->level = newLevel;
    }
    /*
     * Make the new element and insert it in the list. 
     */
    x = newSkipListNode( list, key, value, newLevel );
    /*
     * Scan all levels.
     */
    for ( i=0; i<=newLevel; i++ ) {
        /*
         * Set the next pointer of the new element. 
         */
        x->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = x;
    }
    list->numNodes++;
    
    return(0);
}

/***************************************************************************
 * Function    : SkipListDelete()
 * Description : Deletes the matching element from the list. 
 * Input       : Pointer to the list and the key whose corresponding
 *               entry is to be deleted from the list. 
 * Output      : None.
 * Returns     : 0 -- success, -1 -- failure
 ****************************************************************************/
int
SkipListDelete( SkipList_t *list, void *key ) 
{
    int i = 0;
    SkipListNode_t *x = NULL;
    SkipListNode_t *y = NULL;
    SkipListNode_t *update[SKIPLIST_MAXLEVEL];
    /*
     * Scan all levels while (list-key<serach-key) starting
     * with header in its level. 
     */
    for ( x=list->header, i=list->level; i >= 0; i-- ) {
        while ( x->forward[i] != 0 &&
                list->cmpFn(x->forward[i]->key, key) < 0 ) {
            x = x->forward[i];
        }
        /*
         * Save level pointer. 
         */
        update[i] = x;
    }
    x = x->forward[0];
    /*
     * If key exists, delete the corresponding element.
     */
    if ( x && list->cmpFn(x->key, key) == 0 ) {
        for ( i=0; i<=list->level; i++ ) {
            if ( (y=update[i])->forward[i] == x ) {
                y->forward[i] = x->forward[i];
            }    
        }
        list->freeFn( x->value );
        skipListNodeFree( list, x );
        /*
         * Set the new header level.
         */
        while ( list->level > 0 &&
            list->header->forward[list->level] == NULL ) {
            list->level--; 
        }
        list->numNodes--;
        return( 0 );
    }
    else {
        return( -1 );
    }    
}

static void
SkipListDummyFree( void *dummy )
{

}


/***************************************************************************
 * Function    : SkipListDeleteNode()
 * Description : Deletes the matching node from the list. 
 * Input       : Pointer to the list and the key whose corresponding
 *               NODE is to be deleted from the list. 
 *               The FREE FUNCTION is NOT called.
 *               This is useful in cases where the structure needs to be preserved
 *               without having to provide a dummy free function
 * Output      : None.
 * Returns     : 0 -- success, -1 -- failure
 ****************************************************************************/
int
SkipListDeleteNode( SkipList_t *list, void *key )
{
    SkipListFree_t saveFree = list->freeFn;
    int ret;

    list->freeFn = SkipListDummyFree;

    ret = SkipListDelete(list,key);

    list->freeFn = saveFree;

    return ret;

}

/***************************************************************************
 * Function    : SkipListGetNode()
 * Description : Finds the matching node for the given key. 
 * Input       : Pointer to the list and the key whose matching
 *               element is to be found. 
 * Output      : None.
 * Returns     : Pointer to the matching node.
 * ***************************************************************************/
SkipListNode_t *
SkipListGetNode( SkipList_t *list, void *key )
{
    int i = 0;
    SkipListNode_t *x= NULL;
    
    /*
     * Scan all levels while (list-key<serach-key) starting
     * with header in its level. 
     */
    for ( x=list->header, i=list->level; i >= 0; i-- ) {
        while ( x->forward[i] != 0 &&
                list->cmpFn(x->forward[i]->key, key) < 0 ) {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    if ( x && list->cmpFn(x->key, key) == 0 ) {
        return( x );
    }
    else {
        return( NULL );
    }    
}


/***************************************************************************
 * Function    : SkipListGetNodeOrNext()
 * Description : Gets the node or if not present, the next node
 *               in the list based on the key.. 
 * Input       : Pointer to the list and the key . 
 * Output      : None.
 * Returns     : Pointer to the node or if not found, the next node.
 * ***************************************************************************/
SkipListNode_t *
SkipListGetNodeOrNext( SkipList_t *list, void *key )
{
    int i = 0;
    SkipListNode_t *x= NULL;
    
    /*
     * Scan all levels while (list-key<serach-key) starting
     * with header in its level. 
     */
    for ( x=list->header, i=list->level; i >= 0; i-- ) {
        while ( x->forward[i] != 0 &&
                list->cmpFn(x->forward[i]->key, key) < 0 ) {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    return (x);  
}



/***************************************************************************
 * Function    : SkipListGetCount()
 * Description : Gets the total number of elements in the list.
 * Input       : Pointer to the list.
 * Output      : None.
 * Returns     : Number of elements in the skip list.
 * ***************************************************************************/
int
SkipListGetCount( SkipList_t *list )
{
    return( SKIPLIST_NUM_NODES(list) );
}

/***************************************************************************
 * Function    : SkipListGetFirst()
 * Description : Gets the first node in the list.
 * Input       : Pointer to the list.
 * Output      : None.
 * Returns     : Pointer to the first node in the list.
 * ***************************************************************************/
SkipListNode_t *
SkipListGetFirst( SkipList_t *list )
{
    return( SKIPLIST_NODE_FIRST(list) );
}

/***************************************************************************
 * Function    : SkipListGetNext()
 * Description : Gets the next node in the list based on the key.
 * Input       : Pointer to the list and the key whose lexicographically
 *               next node is to be found. 
 * Output      : None.
 * Returns     : Pointer to the next node in the list.
 * ***************************************************************************/
SkipListNode_t *
SkipListGetNext( SkipList_t *list, void *key )
{
    int i = 0;
    SkipListNode_t *x= NULL;
    
    for ( x=list->header, i=list->level; i >= 0; i-- ) {
        while ( x->forward[i] != 0 &&
                list->cmpFn(x->forward[i]->key, key) < 0 ) {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    if ( x && list->cmpFn(x->key, key) == 0 ) {
        x = x->forward[0];
    }

    return( x );
}


/***************************************************************************
 * Function    : SkipListFindOrInsert()
 * Description : Searches for the specified data item
 *               If found returns it, otherwise allocates one using
 *                 user specified function, sets key and data and then
 *                 returns it. 
 *               If no user specified function exists, the given key
 *               and data are inserted.
 *              
 *
 * Input       : The list in which the element is to be inserted, key,
 *               element to be inserted, a allocation function if new key and
 *               data are to be allocated, a return value for a flag to indicate
 *               if allocation was called.
 * Output      : sets found if it found an element.
 * Returns     : Node ptr on success, NULL on failiure
 * ***************************************************************************/
SkipListNode_t *
SkipListFindOrInsert( SkipList_t *list, void *key, void *value , SkipListUserAlloc_t allocFn, int *found )
{
    int i = 0;
    int newLevel = 0;
    SkipListNode_t *x= NULL;
    SkipListNode_t *update[SKIPLIST_MAXLEVEL];
    void *newKey;
    void *newValue;
    
    *found = 0;
    
    /*
     * Scan all levels while (list-key<serach-key) starting
     * with header in its level. 
     */
    for ( x=list->header, i=list->level; i >= 0; i-- ) {
        while ( x->forward[i] != 0 &&
                list->cmpFn(x->forward[i]->key, key) < 0 ) {
            x = x->forward[i];
        }
        /*
         * Save level pointer. 
         */
        update[i] = x;
    }
    x = x->forward[0];
    /*
     * Element already exists. 
     */

    if ( x && list->cmpFn(x->key, key) == 0 ) {

        *found = 1;

        return x;

    }


    /*
     * Put a new element in the list for this key.
     * Get the new level and fix the list level.
     */

    newLevel = getNewLevel( list->maxLevels );
    /*
     * Adjust the header level. 
     */
    if ( newLevel > list->level ) {
        for ( i=list->level+1; i<=newLevel; i++ ) {
            update[i] = list->header;
        }
        list->level = newLevel;
    }

    if ( allocFn ) {

        /*
         * User has a callback function to allocate newKey and newValue
         */

        if ( !allocFn(key,value,&newKey,&newValue) )
            return NULL;

    } else {

        /*
         * User intends to insert the supplied key and value
         */

        newKey = key;

        newValue = value;

    }

    /*
     * Make the new element and insert it in the list. 
     */
    x = newSkipListNode( list, newKey, newValue, newLevel );

    if (!x)
        return NULL;

    /*
     * Scan all levels.
     */
    for ( i=0; i<=newLevel; i++ ) {
        /*
         * Set the next pointer of the new element. 
         */
        x->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = x;
    }
    list->numNodes++;
    
    return x;

}


/***************************************************************************
 * Function    : SkipListClear()
 * Description : Frees all data items on the skiplist.
 * Input       : Pointer to the skip list to be cleared.
 * Output      : None.
 * Returns     : None. 
 ****************************************************************************/
void
SkipListClear( SkipList_t *list ) 
{
    register SkipListNode_t *p, *q;
    int i;    

    /*
     * Go through the list and delete all nodes. 
     * except header node
     */
    p = list->header->forward[0];

    while ( p ) {

        q = p->forward[0];

        /*
         * Call the user registered free function in order
         * to free the element stored in this node. 
         */

        list->freeFn( p->value );

        /*
         * Now, free this node. 
         */

        skipListNodeFree( list, p );

        p = q;
    }
    

    /*
     * All the nodes are freed, the list and the header node remain
     */

    for ( i=0; i<=list->maxLevels; i++ ) {

        list->header->forward[i] = NULL;

    }

	list->level = 0;

    list->numNodes = 0;
    
}



/***************************************************************************
 * Function    : SkipListNClear()
 * Description : Frees n data items on the skiplist.
 * Input       : Pointer to the skip list to be cleared.
 * Output      : None.
 * Returns     : None. 
 ****************************************************************************/
void
SkipListNClear( SkipList_t *list , int n) 
{
    register SkipListNode_t *p, *q;
    int i;    
    SkipListNode_t *update[SKIPLIST_MAXLEVEL + 1];
	int maxUpdateLevel = -1;

	if ( !n ||
		!SKIPLIST_NUM_NODES(list) )
		return;


	if ( n > SKIPLIST_NUM_NODES(list) )
		n = SKIPLIST_NUM_NODES(list);

	if ( n == SKIPLIST_NUM_NODES(list) ) {
			
		SkipListClear(list);
		return;

	}

	bzero((char *)update,sizeof(update));
	
    /*
     * Go through the list and delete all nodes. 
     * except header node
     */

    p = list->header->forward[0];

	assert(p != NULL);

	i = n;

    while ( i ) {

        q = p->forward[0];

        /*
         * Call the user registered free function in order
         * to free the element stored in this node. 
         */

		if ( p->level > maxUpdateLevel )
			maxUpdateLevel = p->level;

		memcpy(update,p->forward,(p->level  + 1 ) * sizeof(SkipListNode_t *));

        list->freeFn( p->value );

        /*
         * Now, free this node. 
         */

        skipListNodeFree( list, p );

        p = q;

		i--;
    }

	memcpy(list->header->forward,update,(maxUpdateLevel + 1 ) * sizeof(SkipListNode_t *));

    while ( list->level > 0 &&
        list->header->forward[list->level] == NULL ) {
        list->level--; 
    }

    list->numNodes-=n;
    
}

/***************************************************************************
 *                        S T A T I C   F U N C T I O N S
 ***************************************************************************/
static int
getNewLevel( int maxLevel )

{
    int level;

    for ( level = 0; (rand() < (RAND_MAX / 2)) && level < SKIPLIST_MAXLEVEL; level++ )
        ;

    level = level > maxLevel ? maxLevel : level;

    return( level );

}

static SkipListNode_t *
newSkipListNode( SkipList_t *list, void *key, void *value, int newLevel )
{
    int i;
    SkipListNode_t *node;

    node = skipListNodeAlloc(list,newLevel);
    if ( node == NULL )
        return( NULL );
    for ( i=0; i<=newLevel; i++ ) {
        node->forward[i] = NULL;
    }
    node->key   = key;
    node->value = value;
	node->level = newLevel;
    return( node );
}


static void *
skipListNodeAlloc( SkipList_t *list, int level )
{
    int idx;

    return malloc(SKIPLIST_NODE_SIZE(level));

}

static void
skipListNodeFree( SkipList_t *list, void *node )
{
    int idx;

    free(node);            
}


/***************************************************************************
 *                        M A I N   F U N C T I O N 
 ***************************************************************************/

/*#define SKIPLIST_TEST*/
#ifdef SKIPLIST_TEST

static int myCmp( void *key1, void *key2 )
{
    int key1Val = (int) ((int *)key1);
    int key2Val = (int) ((int *)key2);

    return( key1Val - key2Val );
}

static void myFree( void *value )
{
   printf("Freed %d\n",(int)value);

}

static void displayMenu()
{
    printf( "\n\n\n\n\n" );
    printf( "1. Insert \n" );
    printf( "2. Delete \n" );
    printf( "3. Search \n" );
    printf( "4. GetNext\n" );
    printf( "5. Print\n" );
    printf( "6. Clear\n" );
    printf( "7. Exit\n" );
    printf( "\n\n\n" );
    printf( "Choice : " );
}

static void insertValue( SkipList_t *list )
{
    char buf[256];
    int  value = 0;

    printf( "\n\nEnter the value to be inserted : " );
    gets( buf );
    value = atoi(buf);
    printf( "Inserting %d ...\n", value );
    if ( SkipListInsert( list, (void *)value, (void *)value, 0 ) != 0 ) {
        printf( "Error in inserting the value\n" );
    }
    else {
        printf( "Insertion is successful\n" );
    }    
}

static void deleteValue( SkipList_t *list )
{
    char buf[256];
    int  key = 0;

    printf( "\n\nEnter the value to be deleted : " );
    gets( buf );
    key = atoi(buf);
    printf( "Deleting the matched element for the key %d ...\n", key );
    if ( SkipListDelete(list, (void *)key) != 0 ) {
        printf( "Error in deleting the value\n" );
    }
    else {
        printf( "Deletion is successful\n" );
    }    
}

static void searchValue( SkipList_t *list )
{
    char buf[256];
    int  key = 0;
    SkipListNode_t *node;
    
    printf( "\n\nEnter the value to be searched : " );
    gets( buf );
    key = atoi(buf);
    printf( "Searching the element for the key %d ...\n", key );
    node = SkipListGetNode( list, (void *)key );
    if ( node == NULL ) {
        printf( "Error in searching the value\n" );
    }
    else {
        printf( "Search is successful\n" );
    }    
}

static void getNextValue( SkipList_t *list )
{
    char buf[256];
    int  key = 0;
    SkipListNode_t *node;
    
    printf( "\n\nEnter the NEXT value to be found : " );
    gets( buf );
    key = atoi(buf);
    printf( "Getting the next element for the key %d ...\n", key );
    node = SkipListGetNext( list, (void *)key );
    if ( node == NULL ) {
        printf( "There is no next element\n" );
    }
    else {
        printf( "Search is successful\n" );
        printf( "Next element is %d\n", (int)node->value );
    }    
}

static void printValue( SkipList_t *list )
{
    int i = 0;
    SkipListNode_t *node;
    
    printf( "\n\n" );
    node = SkipListGetFirst( list );
    for ( ; node; node=node->forward[0] ) {
        printf( "Element-%d : %d\n", i++, (int)node->value );
    }    
}

static void clearList( SkipList_t *list )
{

    printf("\n\nClearing list\n");

    SkipListClear(list);

}

main()
{
    SkipList_t *list = NULL;
    char buf[256];
    int  choice = 0;
 
    list = SkipListAlloc( myCmp, myFree );
    if ( list == NULL ) {
        printf( "ERROR: Allocation of skip list failed\n" );
        exit(1);
    }

    while (1) {
        displayMenu();
        choice = atoi( gets(buf) );
        switch( choice ) {
            case 1 :
                insertValue( list );
                break;
            case 2 :
                deleteValue( list );
                break;
            case 3 :
                searchValue( list );
                break;
            case 4 :
                getNextValue( list );
                break;
            case 5 :
                printValue( list );
                break;
            case 6 :
                clearList( list );
                break;
            case 7:
                SkipListFree(list);
                exit(0);
            default :
                break;
        }
    }

    exit(0);
}

#endif /* SKIPLIST_TEST */
