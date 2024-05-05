/**
 * @file bencode.h
 *
 * @brief header file for decode and encode data in bencode format.
 * 
 * Tue Sep 14 21:37:24 2004
 * Copyright (c) 2004  Alejandro Claro
 * aleo@apollyon.no-ip.com
 */

#ifndef _BENCODE_H
#define _BENCODE_H

/* INCLUDES *****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* TYPE DEFS ****************************************************************/

typedef unsigned int UINT32;

/**
 * @brief Enumeration of the posible BencNode types.
 *
 * Enumeration of the posible BencNode Types. It include 
 * BENC_TYPE_ALL that is just for use with search functions.
 */
typedef enum
{
  BENC_TYPE_INTEGER = 0, /**< integer type */
  BENC_TYPE_STRING,      /**< string type */
  BENC_TYPE_LIST,        /**< list type */
  BENC_TYPE_DICTIONARY,  /**< dictionary type */
  BENC_TYPE_KEY,         /**< key type (a string) */
  BENC_TYPE_ALL          /**< all types (for search functions) */
} BencType;

/**
 * @brief Structure that define a node for decoded Bencode.
 *
 * Structure that define the element of a decoded tree of 
 * bencode data.
 * DON'T EDIT THE MEMBERS DIRECTLY. 
 *
 */
typedef struct _BencNode
{
  BencType     type;           /**< The node's type. @see BencType   */
  UINT32       length;         /**< The data's length.               */
  char         *data;          /**< The data (not necesary a string) */

  struct _BencNode *next;     /**< pointer to the next sibling.     */
  struct _BencNode *parent;   /**< pointer to the parent.           */
  struct _BencNode *children; /**< pointer to the first child.      */
} BencNode;

/* MACROS *******************************************************************/

/**
 * @brief determine the Type of a BencNode.
 *
 * @param  node: a BencNode.
 * @return the type (unsigned short).
 */ 
#define benc_node_type(node)    ((node)->type)

/**
 * @brief determine the data's Length inside of a BencNode.
 *
 * @param  node: a BencNode.
 * @return the data's length (UINT32).
 */ 
#define benc_node_length(node)  ((node)->length)

/**
 * @brief the data inside a BencNode.
 *
 * The data inside a BencNode most no be changed directly.
 * Use benc_node_change_data to do that.
 *
 * @param  node: a BencNode.
 * @return a constant pointer to the data (const char *).
 */ 
#define benc_node_data(node)    ((const char*)((node)->data))

/**
 * @brief Return TRUE if the node is the root of the tree
 *
 * @param node a BencNode
 * @return TRUE if it's the root of the tree, FALSE otherwise. 
 */
#define benc_node_is_root(node) ((node)->parent==NULL && (node)->next==NULL)

/**
 * @brief Return TRUE if the node is a leaf of the tree
 *
 * @param node a BencNode
 * @return TRUE if it's a leaf of the tree, FALSE otherwise. 
 */
#define benc_node_is_leaf(node) ((node)->children==NULL)

/**
 * @brief Append a BencNode as the last children of parent
 *
 * Append a BencNode as the last children of parent. This DON'T
 * create a new node.
 *
 * @param  parent: BencNode parent.
 * @param  node: a BencNode to append.
 * @return a pointer to the appended node (the same node param).
 */ 
#define benc_node_append(parent, node) benc_node_insert (parent, -1, node)

/**
 * @brief Create a BencNode as the last children of parent
 *
 * @param  parent: a BencNode parent.
 * @param  type: the BencType type of the new node.
 * @param  length: the length of the data.
 * @param  data: a pointer to the data to copy inside the node.
 * @return a pointer to the new appended node.
 */ 
#define benc_node_append_new(parent, type, length, data) benc_node_insert_new (parent, -1, type, length, data)

/**
 * @brief Determine the first child of a node
 *
 * @param  node: a BencNode parent.
 * @return a pointer to the first child.
 */ 
#define benc_node_first_child(node) ((node)->children)

/**
 * @brief Gets the next sibling of a BencNode. 
 *
 * Gets the next sibling of a BencNode. This could 
 * possibly be NULL if the node is the last one.
 *
 * @param node: a BencNode.
 * @return a pointer to the next sibling or null if node is the last one.
 */ 
#define benc_node_next_sibling(node) ((node)->next)

/* PROTOTYPES ***************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

BencNode* benc_decode_file (FILE* fp);
BencNode* benc_decode_buf (char* data, UINT32 length, UINT32* bytes);
UINT32    benc_encode_file (BencNode* tree, FILE* fp);
char*     benc_encode_buf (BencNode* tree, UINT32* bytes);

BencNode* benc_node_new (BencType type, UINT32 length, char* data);
BencNode* benc_node_copy (BencNode* node);
BencNode* benc_node_change (BencNode** node, BencType type, UINT32 length,
                            char* data);
                            
BencNode* benc_node_insert (BencNode* parent, int position, BencNode* node);
BencNode* benc_node_insert_new (BencNode* parent, int position, BencType type, 
                                UINT32 length, char* data);

BencNode* benc_node_find (BencNode* root, BencType type, UINT32 length,
                          char* data);
BencNode* benc_node_find_child (BencNode* parent, BencType type, UINT32 length, 
                                char* data);
BencNode* benc_node_find_key (BencNode* node, char* key);

BencNode* benc_node_get_root (BencNode* node);
BencNode* benc_node_last_child (BencNode* node);
BencNode* benc_node_nth_child (BencNode* node, unsigned int n);
BencNode* benc_node_first_sibling (BencNode* node);
BencNode* benc_node_last_sibling (BencNode* node);
BencNode* benc_node_prev_sibling (BencNode* node);

BencNode* benc_node_unlink (BencNode* node);
void      benc_node_destroy (BencNode* root);

#ifdef __cplusplus
}
#endif

#endif /* _BENCODE_H */
