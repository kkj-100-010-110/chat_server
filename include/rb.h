#ifndef _RED_BLACK_TREE_H_
#define _RED_BLACK_TREE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "queue.h"

#define LEFT 0
#define RIGHT 1

/* For print() */
#define TEXT_RED "\033[0;31m"
#define TEXT_RESET "\033[0m"

/* UTILITY */
int comparison_s(const void *p1, const void *p2);
int comparison_i(const void *p1, const void *p2);
void free_key(void *key);
void free_data(void *data);

/* DATA TYPE */
typedef enum { BLACK=0, RED=1 } Color;
typedef int (*Compare)(const void*, const void*);
typedef void (*KeyFree)(void *);
typedef void (*DataFree)(void *);

typedef struct rb_tree {
	struct rb_node *root;
	Compare compare;
	KeyFree key_free;
	DataFree data_free;
} RB_tree;

typedef struct rb_node {
	void	*key;
	void	*data;
	Color	color;
	int		size; // subtree count
	struct rb_node *link[2];
} RB_node;

/* BASIS */
size_t rb_size_t(RB_tree *t);
size_t rb_size_n(RB_node *n);
RB_tree *rb_create(int (*comparison)(const void*, const void*), void (*key_free)(void *), void (*data_free)(void *));
RB_node *rb_node_create(void *key, void *data);
bool rb_contain(RB_tree *t, void *key);
bool rb_is_red(RB_node *n);
void *rb_max_t(RB_tree *t);
void *rb_min_t(RB_tree *t);
RB_node *rb_max_n(RB_node *n);
RB_node *rb_min_n(RB_node *n);
RB_node *rb_find(RB_node *n, Compare f, void *key);

/* INSERTION */
void rb_insert(RB_tree *t, void *key, void *data);
void _rb_insert(RB_node **n, Compare f, DataFree df, void *key, void *data);

/* DELETION */
void rb_remove(RB_tree *t, void *key);
void _rb_remove(RB_node **n, Compare f, KeyFree kf, DataFree df, void *key);
void rb_remove_tree(RB_tree *t);

void rb_remove_min(RB_tree *t);
void rb_remove_max(RB_tree *t);
void _rb_remove_min(KeyFree kf, DataFree df, RB_node **n);
void _rb_remove_max(KeyFree kf, DataFree df, RB_node **n);

void rb_move_red_left(RB_node **n);
void rb_move_red_right(RB_node **n);

void fix_up(RB_node **n);

void free_key_and_data(KeyFree kf, DataFree df, RB_node **n);

/* ROTATIONS */
void rb_rotate_left(RB_node **n);
void rb_rotate_right(RB_node **n);

/* COLOR FLIP */
void rb_color_flip(RB_node **n);
Color rb_color(RB_node *n);
Color rb_color_change(Color c);

/* PRINT */
void rb_print(RB_tree *t, int order);
void rb_print_node(RB_node *n, int order);
void _print_preorder(RB_node *n);
void _print_inorder(RB_node *n);
void _print_postorder(RB_node *n);
void _print_level_i(RB_node *n);
void _print_level_s(RB_node *n);
void _print_inorder_string(RB_node *n);

#endif//_RED_BLACK_TREE_H_
