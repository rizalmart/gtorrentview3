/**
 * @file bencode.c
 *
 * @brief Functions for decode and encode data in bencode format.
 *
 * Tue Sep 14 21:35:13 2004
 * Copyright (c) 2004  Alejandro Claro
 * aleo@apollyon.no-ip.com
 */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define MAXDIGIT 10 /* max number of digit in a UINT32 */

/* INCLUDES *****************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "bencode.h"

/* PRIVATE FUNCTIONS ********************************************************/

static BencNode* _benc_node_copy_sibling (BencNode* node, BencNode* parent);

static BencNode* _benc_decode_buf_string (char* data, UINT32 length, UINT32 *bytes);
static BencNode* _benc_decode_buf_int (char* data, UINT32 length, UINT32 *bytes);
static BencNode* _benc_decode_buf_list (char* data, UINT32 length, UINT32 *bytes);
static BencNode* _benc_decode_buf_dictionary (char *data, UINT32 length, UINT32 *bytes);

static BencNode* _benc_decode_file_string (FILE *fp);
static BencNode* _benc_decode_file_int (FILE *fp);
static BencNode* _benc_decode_file_list (FILE *fp);
static BencNode* _benc_decode_file_dictionary (FILE *fp);

/* FUNCTIONS ****************************************************************/

/**
 * @brief Decode bencode data from buffer to a Tree (BencNode)
 *
 * numbers are saved as string, no like numbers, so convertion is 
 * necesary when you need the data. The data field of list and
 * dictionary types are the number of childrens saved as string like
 * everything else.
 *
 * @param data: bencode data.
 * @param length: the length of the bencode data
 * @param bytes: return the number of bytes readed from buffer (can be NULL)
 * @return a pointer to a new allocated BencNode tree.
 */
BencNode*
benc_decode_buf (char* data, UINT32 length, UINT32 *bytes)
{
  UINT32 bytes_counter;
  BencNode *tree = NULL;

  if(data == NULL || length == 0) 
    return NULL;

  /* this do all the job */
  switch (data[0])
  {
    case 'd':			/* dictionary */
      tree = _benc_decode_buf_dictionary (data, length, &bytes_counter);
      break;
    case 'l':			/* list */
      tree = _benc_decode_buf_list (data, length, &bytes_counter);
      break;
    case 'i':			/* integer */
      tree = _benc_decode_buf_int (data, length, &bytes_counter);
      break;
    default:      /* string? */
      if(isdigit(data[0]))
        tree = _benc_decode_buf_string (data, length, &bytes_counter);
  }

  if(bytes != NULL)
    *bytes = bytes_counter;
  
  return tree;
}

/**
 * @brief decode a bencoded string from a buffer
 *
 * DON'T USE DIRECTLY. use benc_decode_buf instead.
 *
 * @param data: the bencode string.
 * @param length: the length of the data
 * @param bytes: return the bytes readed from the data buffer.
 * @return a pointer to a new allocated BencNode with the string.
 */
static BencNode*
_benc_decode_buf_string (char* data, UINT32 length, UINT32 *bytes)
{
  BencNode *new;
  char *begin;
  UINT32 l;

  l = strtol(data, &begin, 10); 
  
  if(*begin != ':' || l >= (length - (int)(begin-data)))
  {
    *bytes = 0;
    return NULL;
  }
  
  new = benc_node_new(BENC_TYPE_STRING, l, begin+1);
  *bytes = l + (int)(begin-data) + 1;
  
  return new;  
}

/**
 * @brief decode a bencoded integer from a buffer
 *
 * DON'T USE DIRECTLY. use benc_decode_buf instead.
 *
 * @param data: the bencode integer.
 * @param length: the length of the data
 * @param bytes: return the bytes readed from the data buffer.
 * @return a pointer to a new allocated BencNode with the number as string.
 */
static BencNode*
_benc_decode_buf_int (char* data, UINT32 length, UINT32 *bytes)
{
  BencNode *new;
  char *end;
  UINT32  l;

  end = memchr(data, 'e', length);  
  
  if((l = end-data-1) < 1)
    return NULL;
  
  new = benc_node_new(BENC_TYPE_INTEGER, l, data+1);
  
  *bytes = l + 2;
  return new;
}

/**
 * @brief decode a bencoded list 
 *
 * DON'T USE DIRECTLY. use benc_decode_buf instead.
 *
 * @param data: the bencode list.
 * @param length: the length of the data
 * @param bytes: return the bytes readed from the data buffer.
 * @return a pointer to a new allocated BencNode tree of the list.
 */
static BencNode*
_benc_decode_buf_list (char *data, UINT32 length, UINT32 *bytes)
{
  BencNode *root, *node;
  UINT32 counter, bytes_counter, number;
  char new_data[MAXDIGIT+1];

  root = benc_node_new (BENC_TYPE_LIST, 1, "0");
  
  for(counter = 1, number = 0; data[counter] != 'e' && counter < length;
      counter += bytes_counter, number++)
  {
    node = benc_decode_buf(data+counter, length-counter, &bytes_counter);
        
    if(node == NULL || bytes_counter == 0)
    {
      *bytes = 0;
      benc_node_destroy(root);
      return NULL;
    }
    
    benc_node_append(root, node); 
  }

  *bytes = counter+1;
  sprintf(new_data, "%i", number);
  root = benc_node_change (&root, benc_node_type(root), strlen(new_data), new_data);  
  
  return root;  
}

/**
 * @brief decode a bencoded dictionary
 *
 * DON'T USE DIRECTLY. use benc_decode_buf instead.
 *
 * @param data: the bencode  dictionary.
 * @param length: the length of the data
 * @param bytes: return the bytes readed from the data buffer.
 * @return a pointer to a new allocated BencNode tree of the dictionary.
 */

static BencNode*
_benc_decode_buf_dictionary (char *data, UINT32 length, UINT32 *bytes)
{
  BencNode *root, *key, *node;
  UINT32 counter, bytes_counter, number;
  char new_data[MAXDIGIT+1];

  root = benc_node_new (BENC_TYPE_DICTIONARY, 1, "0");
  
  for(counter = 1, number = 0; data[counter] != 'e' && counter < length;
      counter += bytes_counter, number++)
  {
    key = NULL;

    if(data[counter] == 'i')
    {
      key = _benc_decode_buf_int (data+counter,
                            length-counter, &bytes_counter);
    }
    else if(isdigit(data[counter]))
    {
      key = _benc_decode_buf_string (data+counter,
                            length-counter, &bytes_counter);
    }

    if(key != NULL)
    {        
      key->type = BENC_TYPE_KEY;
      counter += bytes_counter;
        
      node = benc_decode_buf(data+counter, length-counter, &bytes_counter);
    }

    if(key == NULL || node == NULL)
    {
      *bytes = 0;
      benc_node_destroy(root);
      return NULL;
    }
    
    benc_node_append(key, node);
    benc_node_append(root, key); 
  }

  *bytes = counter+1;
  sprintf(new_data, "%i", number);
  root = benc_node_change(&root, benc_node_type(root), strlen(new_data), new_data);  
  
  return root;  
}
  
/**
 * @brief Decode bencode data from file to a Tree (BencNode)
 *
 * numbers are saved as string, no like numbers, so convertion is 
 * necesary when you need the data. The data field of list and
 * dictionary types are the number of childrens saved as string like
 * everything else.
 *
 * @param fp: FILE pointer.
 * @return a pointer to a new allocated BencNode tree.
 */
BencNode*
benc_decode_file (FILE* fp)
{
  BencNode *tree = NULL;
  char c;
  
  if(fp == NULL || feof(fp)) 
    return NULL;

  /* this do all the job */
  c = fgetc(fp);
  switch (c)
  {
    case 'd':			/* dictionary */
      tree = _benc_decode_file_dictionary (fp);
      break;
    case 'l':			/* list */
      tree = _benc_decode_file_list (fp);
      break;
    case 'i':			/* integer */
      tree = _benc_decode_file_int (fp);
      break;
    default:      /* string? */
      ungetc (c, fp);    
      if(isdigit(c))
        tree = _benc_decode_file_string (fp);
  }

  return tree;
}

/**
 * @brief decode a bencoded string from a file
 *
 * DON'T USE DIRECTLY. use benc_decode_file instead.
 *
 * @param fp: the FILE to read.
 * @return a pointer to a new allocated BencNode with the string.
 */
static BencNode*
_benc_decode_file_string (FILE *fp)
{
  BencNode *new;
  char *buff;
  char len[MAXDIGIT+1];  /* 10 is the max number of digit of a UINT32 */
  UINT32 l;
  unsigned short ok;
  
  for(l = 0, ok = 0; l < MAXDIGIT+1 && !feof(fp); l++)
  {
    len[l] = fgetc(fp);
    if(len[l] == ':')
    {
      ok = 1;
      break;
    }
  }
  
  if(!ok)
    return NULL;
  
  len[l] = '\0';
  l = strtol(len, (char **)NULL, 10);

  buff = malloc(l+1);
  fread(buff, l, 1, fp);
  
  new = benc_node_new(BENC_TYPE_STRING, l, buff);

  free(buff);   
  return new;  
}

/**
 * @brief decode a bencoded integer from a file
 *
 * DON'T USE DIRECTLY. use benc_decode_file instead.
 *
 * @param fp: the FILE to read.
 * @return a pointer to a new allocated BencNode with the number as string.
 */
static BencNode*
_benc_decode_file_int (FILE *fp)
{
  BencNode *new;
  char number[MAXDIGIT+1];
  unsigned short l, ok;

  for(l = 0, ok = 0; l < MAXDIGIT+1 && !feof(fp); l++)
  {
    number[l] = fgetc(fp);
    if(number[l] == 'e')
    {
      ok = 1;
      break;
    }
  }
  
  if(!ok)
    return NULL;
  
  new = benc_node_new(BENC_TYPE_INTEGER, l, number);
  
  return new;
}

/**
 * @brief decode a bencoded list from a file
 *
 * DON'T USE DIRECTLY. use benc_decode_file instead.
 *
 * @param fp: the FILE to read.
 * @return a pointer to a new allocated BencNode tree of the list.
 */
static BencNode*
_benc_decode_file_list (FILE *fp)
{
  BencNode *root, *node;
  UINT32 number;
  char new_data[MAXDIGIT+1], c;

  if(feof(fp))
    return NULL;
  
  root = benc_node_new (BENC_TYPE_LIST, 1, "0");
  
  for(c = fgetc(fp), number = 0; c != 'e' && !feof(fp); c = fgetc(fp), number++)
  {
    ungetc(c, fp);
    
    node = benc_decode_file(fp);
        
    if(node == NULL)
    {
      benc_node_destroy(root);
      return NULL;
    }
    
    benc_node_append(root, node); 
  }

  sprintf(new_data, "%i", number);
  root = benc_node_change (&root, benc_node_type(root), strlen(new_data), new_data);  
  
  return root;  
}

/**
 * @brief decode a bencoded dictionary from a file
 *
 * DON'T USE DIRECTLY. use benc_decode_file instead.
 *
 * @param fp: the FILE to read.
 * @return a pointer to a new allocated BencNode tree of the dictionary.
 */
static BencNode*
_benc_decode_file_dictionary (FILE *fp)
{
  BencNode *root, *node, *key;
  UINT32 number;
  char new_data[MAXDIGIT+1], c;

  if(feof(fp))
    return NULL;
  
  root = benc_node_new (BENC_TYPE_DICTIONARY, 1, "0");
  
  for(c = fgetc(fp), number = 0; c != 'e' && !feof(fp); c = fgetc(fp), number++)
  {
    key = NULL;

    if(c == 'i')
      key = _benc_decode_file_int (fp);
    else if(isdigit(c))
    {
      ungetc(c, fp);
      key = _benc_decode_file_string (fp);
    }

    if(key != NULL)
    {        
      key->type = BENC_TYPE_KEY;
        
      node = benc_decode_file(fp);
    }
        
    if(key == NULL || node == NULL)
    {
      benc_node_destroy(root);
      return NULL;
    }
    
    benc_node_append(key, node);
    benc_node_append(root, key); 
  }

  sprintf(new_data, "%i", number);
  root = benc_node_change (&root, benc_node_type(root), strlen(new_data), new_data);  
  
  return root;  
}


/**
 * @brief Encode a tree (BencNode) to a bencode meta file.
 *
 * @param fp: the to write.
 * @param tree: the tree to encode.
 * @return the number of wrote bytes.
 */
UINT32
benc_encode_file (BencNode* tree, FILE* fp)
{
  BencNode *child;
  UINT32 bytes;

  if(tree == NULL)
    return 0;

  switch(benc_node_type(tree))
  {
   case BENC_TYPE_INTEGER:
      bytes = fprintf (fp, "i%se", benc_node_data (tree));
      break;
   case BENC_TYPE_STRING:
      bytes  = fprintf (fp, "%i:", benc_node_length (tree));   
      bytes += fwrite (benc_node_data (tree), sizeof(char), benc_node_length (tree), fp);
      break;
   case BENC_TYPE_KEY:
      bytes  = fprintf (fp, "%i:", benc_node_length (tree));   
      bytes += fwrite(benc_node_data (tree), sizeof(char), benc_node_length (tree), fp);
      bytes += benc_encode_file (benc_node_first_child(tree), fp);
      break;
   case BENC_TYPE_LIST:
   case BENC_TYPE_DICTIONARY:
      bytes = fprintf(fp, "%c", (benc_node_type (tree) == BENC_TYPE_LIST)? 'l' : 'd');
      for(child = benc_node_first_child (tree); child != NULL;
          child = benc_node_next_sibling (child))
      {
        bytes += benc_encode_file (child, fp);
      } 
      bytes += fprintf(fp, "%c", 'e');
      break;
   case BENC_TYPE_ALL:
   default:
      bytes = 0;      
  }    

  return bytes;
}

/**
 * @brief Encode a tree (BencNode) to a bencode buffer.
 *
 * @param tree: the tree to encode.
 * @param bytes: return the length of the buffer.
 * @return a pointer to a new allocated buffer.
 */
char*
benc_encode_buf (BencNode *tree, UINT32 *bytes)
{
  BencNode *child;
  char *string, *tmp, *tmp2;
  UINT32 counter, counter2;

  string = NULL;
  
  if(tree == NULL)
    return NULL;

  switch(benc_node_type(tree))
  {
   case BENC_TYPE_INTEGER:
      string = malloc (benc_node_length(tree)+3);
      *bytes = snprintf (string, benc_node_length(tree)+3, 
                         "i%se", benc_node_data (tree));
      break;
   case BENC_TYPE_STRING:
      string = malloc (MAXDIGIT + 1 + benc_node_length (tree));
      counter = sprintf (string, "%i:", benc_node_length (tree));   
      memmove (string+counter, benc_node_data (tree), benc_node_length (tree));
      *bytes = counter + benc_node_length (tree);
      break;
   case BENC_TYPE_KEY:
      tmp = malloc(MAXDIGIT+1+benc_node_length (tree));
      counter = sprintf (tmp, "%i:", benc_node_length (tree));   
   
      memmove (tmp+counter, benc_node_data (tree), benc_node_length (tree));
      counter += benc_node_length (tree);
    
      tmp2 = benc_encode_buf (benc_node_first_child(tree), &counter2);

      if( tmp2 != NULL)
      {        
        string = malloc(counter + counter2);
        memmove (string, tmp, counter);
        memmove (string+counter, tmp2, counter2);
        free (tmp2);

        *bytes = counter + counter2;
      }
   
      free(tmp);
      break;
   case BENC_TYPE_LIST:
   case BENC_TYPE_DICTIONARY:
      string = malloc(2);
      string[0] = (benc_node_type (tree) == BENC_TYPE_LIST)? 'l' : 'd';
      counter = 1;
    
      for(child = benc_node_first_child (tree); child != NULL;
          child = benc_node_next_sibling (child))
      {
        tmp2 = benc_encode_buf (child, &counter2);
        
        tmp = malloc (counter + counter2 + 1);
        memmove (tmp, string, counter);
        memmove (tmp + counter, tmp2, counter2);
        counter += counter2;
        
        free(tmp2);
        free(string);
        
        string = tmp;
      } 
      string[counter] = 'e';

      *bytes = counter + 1;
      break;
   case BENC_TYPE_ALL:
   default:
       *bytes = 0;      
  }    

  return string;
}

/**
 * @brief Create a new node
 *
 * Create a new node for the tree. Use this for create the root node.
 * The copied data is NULL terminated. You're responsible of call
 * benc_node_destroy to free the memory.
 *
 * @param type: the node's type.
 * @param length: the data's length.
 * @param data: the data to copy inside the node.
 * @return a pointer to the node.
 */
BencNode*
benc_node_new (BencType type, UINT32 length, char* data)
{
  BencNode *root;

  root = (BencNode *)malloc(sizeof(BencNode)+length+1);

  root->type = type;
  root->length = length;
  root->data = (char *)(root+1);  
  
  memmove(root->data, data, length);
  root->data[length] = '\0'; /* append a extra 0x0 to the end */

  root->parent = NULL;  
  root->next = NULL;
  root->children = NULL;
  
  return root;
}

/**
 * @brief Change the type and/or the data of a node
 *
 * Change the type and/or the data of a node. If length is 0
 * and data is NULL then is changed just the type. Node may change 
 * of memory address.
 *
 * @param node: a pointer to the BencNode to change
 * @param type: the new type of the nodePop
 * @param length: the new data's length.
 * @param data: pointer to the new data.
 * @return a pointer to the node. 
 */
BencNode*
benc_node_change (BencNode** node, BencType type, UINT32 length, 
                  char* data)
{
  BencNode *new, *first;

  if(length > (*node)->length)
  {
    new = benc_node_new(type, length, data);
    new->parent = (*node)->parent;
    new->next = (*node)->next;
    new->children = (*node)->children;

    for(first = benc_node_first_sibling(*node);
        first->next != NULL && first->next != new->next; first = first->next)
    {
      if(first->next == *node)
      {
        first->next = new;
        break;
      }
    }

    for(first = benc_node_first_child(*node); first != NULL;
        first = first->next)
    {
        first->parent = new;
    }
    
    free(*node);  
    *node = new;    
  }
  else if(length == 0)
  {
    (*node)->type = type;
  }
  else
  {
    (*node)->type = type;
    (*node)->length = length;
    memmove((*node)->data, data, length);
  }
  
  return *node;  
}

/**
 * @brief Recursively copies a BencNode. 
 *
 * @param node: a BencNode node 
 * @return a pointer to the new tree.
 */
BencNode*
benc_node_copy (BencNode* node)
{
  BencNode *root;

  root = (BencNode *)malloc (sizeof(BencNode)+node->length);

  if(root == NULL) 
    return NULL;
  
  memmove (root, node, sizeof(BencNode)+node->length);

  root->parent = NULL;  
  root->next = NULL;
  
  if(node->children != NULL)
    root->children = _benc_node_copy_sibling (node->children, root);   
  else
    root->children = NULL;

  return root;
}

/**
 * @brief Recursively copies a BencNode and all the sibling next to it.
 *
 * DON'T USE THIS DIRECTLY. note that return node hasn't a root, and 
 * the sibling at the right are not copied.
 * @see benc_node_copy.
 *
 * @param node: the BencNode node to copy
 * @param parent: the new BencNode parent node.
 * @return a pointer to the new tree.
 */
static BencNode*
_benc_node_copy_sibling (BencNode* node, BencNode* parent)
{
  BencNode *first;

  /* i was thinking recall 'first' -> 'this', but it's used by C++ */
  first = (BencNode *)malloc (sizeof(BencNode)+node->length);
  if(first == NULL) 
    return NULL;
  
  memmove (first, node, sizeof(BencNode)+node->length);

  first->parent = parent;  
  
  if(node->children != NULL)
    first->children = _benc_node_copy_sibling (node->children, first);
  else
    first->children = NULL;
  
  if(node->next != NULL)
    first->next = _benc_node_copy_sibling (node->next, parent);
  else
    first->next = NULL;
  
  return first;  
}

/**
 * @brief Insert a BencNode beneath the parent at the given position.
 *
 * Insert a BencNode beneath the parent at the given position. This don't
 * create a new node. @see benc_node_append
 *
 * @param parent: the parent node to place the node under.
 * @param position: with respect to its siblings. If position is -1,
 *                  node is inserted as the last child of parent.
 * @param node: the BencNode to insert.
 * @return a pointer to the inserted node (the same node). NULL if fail
 */
BencNode* 
benc_node_insert (BencNode* parent, int position, BencNode* node)
{
  BencNode *children;

  if(parent == NULL || node == NULL || parent == node) 
    return NULL;
 
  if(parent->children != NULL)
  {
    if(position < 0)
      children = benc_node_last_child (parent);
    else if(position > 0)
    {
      children = benc_node_nth_child (parent, (unsigned)position-1);
      if(children == NULL)
        children = benc_node_last_child (parent);
    }
    else
      children = parent->children;

    node->next = children->next;
    children->next = node;
  }
  else
  {
    node->next = NULL;
    parent->children = node;
  }

  node->parent = parent;
  
  return node;
}

/**
 * @brief Create and Insert a BencNode beneath the parent at the
 *        given position.
 *
 * Insert a BencNode beneath the parent at the given position. 
 * @see benc_node_append_new
 *
 * @param parent: the parent node to place the node under.
 * @param position: with respect to its siblings. If position is -1,
 *                  node is inserted as the last child of parent.
 * @param type: the node's type.
 * @param length: the data's length.
 * @param data: the data to copy inside the node.
 * @return a pointer to the inserted node. NULL if fail
 */
BencNode* 
benc_node_insert_new (BencNode* parent, int position, BencType type, 
                                UINT32 length, char* data)
{
  BencNode *new;

  new = benc_node_new(type, length, data);
  if(benc_node_insert(parent, position, new) == NULL)
  {
    benc_node_destroy(new);
    new = NULL;
  }
  
  return new;
}

/**
 * @brief Find a BencNode in the tree
 *
 * Search for type and/or data. To find any type use BENC_TYPE_ALL
 * as type. To find any data use length = 0 or data = NULL. 
 *
 * @param root: the BencNode root of the tree/sub-tree.
 * @param type: the node's type. (BENC_TYPE_ALL to search just by data)
 * @param length: the data's length. (0 if search just by type)
 * @param data: the data to search. (NULL if just search by type)
 * @return a pointer to the first node that match. NULL if no match.
 */
BencNode* benc_node_find (BencNode* root, BencType type, UINT32 length, char* data)
{
  BencNode *node, *child;
  
  for(node = root; node != NULL; node = benc_node_next_sibling(node))
  {
    if(type == benc_node_type(node) || type == BENC_TYPE_ALL)
    {
      if(data == NULL || length == 0) /* if is just looking by type */
        return node;
        
      if(length == benc_node_length(node))
        if(memcmp(data, benc_node_data(node), length) == 0)
           return node;
    }      
 
    if(!benc_node_is_leaf (node))
    {
      child = benc_node_find (benc_node_first_child(node), type, length, data);
      if(child != NULL) 
         return child;
    }
  }  

  return NULL;  
}

/**
 * @brief Search in the children of a BencNode.
 *
 * Search just in the children of a node for type and/or data.
 * To find any type use BENC_TYPE_ALL  as type. To find any data
 * use length = 0 or data = NULL. 
 *
 * @param parent: the BencNode parent.
 * @param type: the node's type. (BENC_TYPE_ALL to search just by data)
 * @param length: the data's length. (0 if search just by type)
 * @param data: the data to search. (NULL if just search by type)
 * @return a pointer to the first node that match. NULL if no match.
 */
BencNode* benc_node_find_child (BencNode* parent, BencType type, UINT32 length, char* data)
{
  BencNode *node;
  
  if(parent == NULL)
    return NULL;
  
  for(node = benc_node_first_child(parent); node != NULL;
      node = benc_node_next_sibling(node))
  {
    if(type == benc_node_type(node) || type == BENC_TYPE_ALL)
    {
      if(data == NULL || length == 0) /* if is just searching by type */
        return node;
        
      if(length == benc_node_length(node))
        if(memcmp(data, benc_node_data(node), length) == 0)
           return node;
    }      
  }  
 
  return NULL;  
}

/**
 * @brief Find the value node of a KEY in a tree/sub-tree
 *
 * Search for a espesific KEY type node, and returns the child node (the 
 * value node).
 *
 * @param node: the BencNode root.
 * @param key: a NULL terminated KEY string.
 * @return a pointer to the value node of the first match. NULL if no match.
 */
BencNode* benc_node_find_key (BencNode* node, char* key)
{
  BencNode *key_node;
  
  key_node = benc_node_find(node, BENC_TYPE_KEY, strlen(key), key);
  if(key_node == NULL)
    return NULL;

  return (key_node->children);
}

/**
 * @brief Gets the last child of a BencNode.
 *
 * @param node: a BencNode (must not be NULL)
 * @return a pointer to the last child. NULL if it has no
 *         children.
 */ 
BencNode* 
benc_node_last_child (BencNode* node)
{
  BencNode *child;
  
  if(node->children == NULL)
    return NULL;
  
  for(child = node->children; child->next != NULL; child = child->next);
  
  return child;
}

/**
 * @brief Gets the nth child of a BencNode. The first child
 *        is at position 0.
 *
 * @param node: a BencNode (must not be NULL)
 * @param n: the position.
 * @return a pointer to the nth child. if position is too big,
 *         return NULL.
 */ 
BencNode* 
benc_node_nth_child (BencNode* node, unsigned int n)
{
  BencNode *child;
  unsigned int i;

  if(node->children == NULL)
    return NULL;

  child = benc_node_first_child(node);
  for(i=0; i<n && child != NULL; i++, child = child->next);

  return child;
}

/**
 * @brief Gets the first sibling of a BencNode. This could 
 *        possibly be the node itself.
 *
 * @param node: a BencNode.
 * @return a pointer to the first sibling.
 */ 
BencNode* 
benc_node_first_sibling (BencNode* node)
{
  if(node->parent == NULL)
    return node;
  
  return benc_node_first_child(node->parent);
}

/**
 * @brief Gets the last sibling of a BencNode. This could 
 *        possibly be the node itself.
 *
 * @param node: a BencNode.
 * @return a pointer to the last sibling.
 */ 
BencNode*
benc_node_last_sibling (BencNode* node)
{
  BencNode *last;

  for(last = node; last->next != NULL; last = last->next);
    
  return last;
}

/**
 * @brief Gets the previus sibling of a BencNode. 
 *
 * Gets the previus sibling of a BencNode. This could 
 * possibly be the node itself if this is the first or
 * only node.
 *
 * @param node: a BencNode.
 * @return a pointer to the previus sibling.
 */ 
BencNode*
benc_node_prev_sibling (BencNode* node)
{
  BencNode *prev;

  if(node->parent == NULL)
    return node;
  
  for(prev = benc_node_first_child(node->parent); 
      prev->next != node && prev->next != NULL; prev = prev->next);
  
  return prev;
}

/**
 * @brief Gets the root of a tree. 
 *
 * @param node: a BencNode.
 * @return a pointer to the root node.
 */ 
BencNode*
benc_node_get_root (BencNode* node)
{
  BencNode *root;

  for(root = node; root->parent != NULL; root = root->parent);
    
  return root; 
}

/**
 * @brief Unlinks a GNode from a tree, 
 *        resulting in two separate trees. 
 *
 * @param node: the BencNode to unlink, which becomes the root
 *              of a new tree.
 * @return a pointer to node.
 */ 
BencNode*
benc_node_unlink (BencNode* node)
{
  BencNode *prev;

  if(!benc_node_is_root (node))
  {
    if(node != benc_node_first_sibling(node))
    {
      prev = benc_node_prev_sibling (node);
      prev->next = node->next;
    }
    else
      (node->parent)->children = node->next;

    node->next = NULL;
    node->parent = NULL;
  }

  return node;
}

/**
 * @brief Removes the GNode and its children from the tree,
 *        freeing any memory allocated.
 *
 * @param root: the root of the tree or subtree to destroy.
 */ 
void
benc_node_destroy (BencNode* root)
{
  if(root == NULL)
    return;
    
  benc_node_unlink (root);

  while (!benc_node_is_leaf (root))
    benc_node_destroy (benc_node_first_child (root));
  
  free (root);
  return;
}

/* END **********************************************************************/
