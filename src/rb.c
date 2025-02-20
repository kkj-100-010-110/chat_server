#include "rb.h"

/* BASIS */

/*
 * Return the size of the sub-tree of root.
 * - Take a tree as its parameter
 */
size_t rb_size_t(RB_tree *t)
{
	if (!t || !t->root) return 0;
	return t->root->size;
}

/*
 * Return the size of the sub-tree of a node.
 * - Take a node as its parameter
 */
size_t rb_size_n(RB_node *n)
{
	if (!n) return 0;
	return n->size;
}

/*
 * Create a tree.
 */
RB_tree *rb_create(int (*comparison)(const void*, const void*), void (*key_free)(void *), void (*data_free)(void *))
{
	RB_tree *t = malloc(sizeof(RB_tree));
	if (!t) {
		fprintf(stderr, "Error: malloc() failed in rb_create()\n");
		return NULL;
	}
	t->root = NULL;
	t->compare = comparison;
	t->key_free = key_free;
	t->data_free = data_free;
	return t;
}

/*
 * Create a node with key and data.
 */
RB_node *rb_node_create(void *key, void *data)
{
	RB_node *n = malloc(sizeof(RB_node));
	if (!n) {
		fprintf(stderr, "Error: malloc() failed in rb_node_create()\n");
		return NULL;
	}
	n->key = key;
	n->data = data;
	n->color = RED;
	n->size = 1;
	n->link[LEFT] = NULL;
	n->link[RIGHT] = NULL;
	return n;
}

/*
 * Insert a node into the tree.
 * - Take a tree, key and data as parameters
 */
void rb_insert(RB_tree *t, void *key, void *data)
{
	if (!t) {
		fprintf(stderr, "Error: Tree is null in rb_insert()\n");
		return;
	}
	_rb_insert(&t->root, t->compare, t->data_free, key, data);
	if (t->root) t->root->color = BLACK;
}

/*
 * rb_insert() help
 * - Same as binary search tree
 * - One is different which is balancing trees
 * - If the node that has the same key, replace the data with input data
 */
void _rb_insert(RB_node **n, Compare f, DataFree df, void *key, void *data)
{
	if (!(*n))
	{
		(*n) = rb_node_create(key, data);
		return;
	}
	else if (f((*n)->key, key) == 0) {
		if (df) {
			free((*n)->data);
			(*n)->data = NULL;
		}
		(*n)->data = data;
		return;
	} else if (f((*n)->key, key) < 0) {
		_rb_insert(&(*n)->link[RIGHT], f, df, key, data);
		fix_up(n);
		return;
	} else {
		_rb_insert(&(*n)->link[LEFT], f, df, key, data);
		fix_up((n));
		return;
	}
}

/*
 * Remove the node with the key
 */
void rb_remove(RB_tree *t, void *key)
{
	if (!rb_contain(t, key)) {
		fprintf(stderr, "Error: There is not the node with the key in rb_remove()\n");
		return;
	}
	_rb_remove(&t->root, t->compare, t->key_free, t->data_free, key);
	if (t->root) t->root->color = BLACK;
}

/*
 * rb_remove() help
 * - if the key value is less than the node's, it goes to left sub-tree of the node
 *  - it works as rb_remove_min()
 * - if the key value is greater than the node's, it goes to right sub-tree of the node
 *  - it works as rb_remove_max()
 * - if the key value is the same as the node's,
 *  - check if there is the sub-tree of the target node
 *  - if it is an internal node, swap the minimum node of its right one(swap the successor for the
 *  target).
 */
void _rb_remove(RB_node **n, Compare f, KeyFree kf, DataFree df, void *key)
{
	if (f(key, (*n)->key) < 0)
	{
		if (!rb_is_red((*n)->link[LEFT]) && !rb_is_red((*n)->link[LEFT]->link[LEFT]))
			rb_move_red_left(n);
		_rb_remove(&(*n)->link[LEFT], f, kf, df, key);
	} else {
		if (rb_is_red((*n)->link[LEFT]))
			rb_rotate_right(n);

		if (f(key, (*n)->key) == 0 && !(*n)->link[RIGHT]) {
			free_key_and_data(kf, df, n);
			free((*n));
			*n = NULL;
			return;
		}

		if (!rb_is_red((*n)->link[RIGHT]) && !rb_is_red((*n)->link[RIGHT]->link[LEFT]))
			rb_move_red_right(n);
		
		// remove the node
		if (f(key, (*n)->key) == 0) {
			RB_node *tmp = rb_min_n((*n)->link[RIGHT]);
			free_key_and_data(kf, df, n);
			(*n)->key = tmp->key;
			(*n)->data = tmp->data;
			_rb_remove_min(kf, df, &(*n)->link[RIGHT]);
		} else {
			_rb_remove(&(*n)->link[RIGHT], f, kf, df, key);
		}
	}
	fix_up(n);
}

/*
 * Remove the whole tree
 */
void rb_remove_tree(RB_tree *t)
{
	while (t->root)
		rb_remove(t, t->root->key);
}

/*
 * Remove the node with the minimum key value
 */
void rb_remove_min(RB_tree *t)
{
	if (!t->root) {
		fprintf(stderr, "Error: Tree is empty in rb_remove_min()\n");
		return;
	}
	_rb_remove_min(t->key_free, t->data_free, &t->root);
	if (t->root) t->root->color = BLACK;
}

/*
 * Remove the node with the maximum key value
 */
void rb_remove_max(RB_tree *t)
{
	if (!t->root) {
		fprintf(stderr, "Error: Tree is empty in rb_remove_min()\n");
		return;
	}
	_rb_remove_max(t->key_free, t->data_free, &t->root);
	if (t->root) t->root->color = BLACK;
}

/*
 * rb_remove_min() help
 * - If the left child node of the current node and its left child node are black
 * - Bottom-up with fix_up() to balance the tree
 */
void _rb_remove_min(KeyFree kf, DataFree df, RB_node **n)
{
	if (!(*n)->link[LEFT]) { // found the min
		free((*n));
		*n = NULL;
		return;
	}

	if (!rb_is_red((*n)->link[LEFT]) && !rb_is_red((*n)->link[LEFT]->link[LEFT]))
		rb_move_red_left(n);

	_rb_remove_min(kf, df, &(*n)->link[LEFT]);
	fix_up(n);
}

/*
 * rb_remove_max() help
 * - If the left link of the current node is black, call rb_rotate_right()
 * - If the right link of the current node and its left link are black, call rb_move_red_right()
 */
void _rb_remove_max(KeyFree kf, DataFree df, RB_node **n)
{
	if (rb_is_red((*n)->link[LEFT]))
		rb_rotate_right(n);

	if (!(*n)->link[RIGHT]) { // found the max
		free((*n));
		*n = NULL;
		return;
	}

	if (!rb_is_red((*n)->link[RIGHT]) && !rb_is_red((*n)->link[RIGHT]->link[LEFT]))
		rb_move_red_right(n);

	_rb_remove_max(kf, df, &(*n)->link[RIGHT]);
	fix_up(n);
}

/*
 * Move the red link to the left.
 * - check if the left child node of the right child of the node is red
 */
void rb_move_red_left(RB_node **n)
{
	rb_color_flip(n);

	if (rb_is_red((*n)->link[RIGHT]->link[LEFT])) {
		rb_rotate_right(&(*n)->link[RIGHT]);
		rb_rotate_left((n));
		rb_color_flip(n);
	}
}

/*
 * Move the red link to the right.
 * - check if the left child node of the left child of the node is red
 */
void rb_move_red_right(RB_node **n)
{
	rb_color_flip(n);

	if (rb_is_red((*n)->link[LEFT]->link[LEFT])) {
		rb_rotate_right(n);
		rb_color_flip(n);
	}
}

/*
 * Free key & data.
 * - callback functions
 */
void free_key(void *key)
{
	free(key);
}
void free_data(void *data)
{
	free(data);
}

void free_key_and_data(KeyFree kf, DataFree df, RB_node **n)
{
	if (kf) {
		kf((*n)->key);
		(*n)->key = NULL;
	}
	if (df) {
		df((*n)->data);
		(*n)->data = NULL;
	}
}


/*
 * Balance trees - when it goes back up to the tree, it fixes up imbalance cases.
 * - any imbalance cases
 *  - the right node of n is red
 *  - the left node of n and its left node are red
 * - color check
 * 	- if left and right nodes of n are red, change colors
 */
void fix_up(RB_node **n)
{
	if (!(*n)) {
		fprintf(stderr, "Error: Node is null in fix_up()\n");
		return;
	}
	if (rb_is_red((*n)->link[RIGHT]) && !rb_is_red((*n)->link[LEFT])) rb_rotate_left(n);
	if (rb_is_red((*n)->link[LEFT]) && rb_is_red((*n)->link[LEFT]->link[LEFT])) rb_rotate_right(n);
	if (rb_is_red((*n)->link[LEFT]) && rb_is_red((*n)->link[RIGHT])) rb_color_flip(n);

	(*n)->size = 1 + rb_size_n((*n)->link[LEFT]) + rb_size_n((*n)->link[RIGHT]);
}

/*
 * Find the node.
 * - Take a node, a function pointer and key as parameters
 */
RB_node *rb_find(RB_node *n, Compare f, void *key)
{
	if (!n) return NULL;
	else if (f(n->key, key) == 0) return n;
	else if (f(n->key, key) < 0) return rb_find(n->link[RIGHT], f, key);
	else return rb_find(n->link[LEFT], f, key);
}

/*
 * Check if the color of the node is red.
 */
bool rb_is_red(RB_node *n)
{
	if (!n) return BLACK; // null node is black
	return n->color;
}

/*
 * Check if the tree has the node with the key.
 */
bool rb_contain(RB_tree *t, void *key)
{
	if (!t) {
		fprintf(stderr, "Error: Tree is null in rb_contain()\n");
		return NULL;
	}
	RB_node *tmp = rb_find(t->root, t->compare, key);
	return tmp == NULL ? false : true;
}

/*
 * Return the max value of the tree.
 */
void *rb_max_t(RB_tree *t)
{
	if (!t) {
		fprintf(stderr, "Error: Tree is null in rb_max_t()\n");
		return NULL;
	}
	RB_node *res = rb_max_n(t->root);
	if (!res) return NULL;
	return res->key;
}

/*
 * Return the max value of the sub-tree of the node.
 */
RB_node *rb_max_n(RB_node *n)
{
	if (!n) {
		fprintf(stderr, "Error: Node is null in rb_max_n()\n");
		return NULL;
	}
	while (n->link[RIGHT])
		n = n->link[RIGHT];
	return n;
}

/*
 * Return the min value of the tree.
 */
void *rb_min_t(RB_tree *t)
{
	if (!t) {
		fprintf(stderr, "Error: Tree is null in rb_min_t()\n");
		return NULL;
	}
	RB_node *res = rb_min_n(t->root);
	if (!res) return NULL;
	return res->key;
}

/*
 * Return the min value of the sub-tree of the node.
 */
RB_node *rb_min_n(RB_node *n)
{
	if (!n) {
		fprintf(stderr, "Error: Node is null in rb_min_n()\n");
		return NULL;
	}
	while (n->link[LEFT])
		n = n->link[LEFT];
	return n;
}

/* ROTATIONS */

/*
 * Rotate the node and its right child node to the left.
 * - When the red link to attach the right of the target node.
 */
void rb_rotate_left(RB_node **n)
{
	RB_node *tmp = (*n)->link[RIGHT];
	(*n)->link[RIGHT] = tmp->link[LEFT];
	tmp->link[LEFT] = (*n);
	tmp->color = (*n)->color;
	(*n)->color = RED;
	tmp->size = (*n)->size;
	(*n)->size = 1 + rb_size_n((*n)->link[LEFT]) + rb_size_n((*n)->link[RIGHT]);
	(*n) = tmp;
}

/*
 * Rotate the node and its right child node to the right.
 * - When the red link is subsequent to the other red link.
 */
void rb_rotate_right(RB_node **n)
{
	RB_node *tmp = (*n)->link[LEFT];
	(*n)->link[LEFT] = tmp->link[RIGHT];
	tmp->link[RIGHT] = (*n);
	tmp->color = (*n)->color;
	(*n)->color = RED;
	tmp->size = (*n)->size;
	(*n)->size = 1 + rb_size_n((*n)->link[LEFT]) + rb_size_n((*n)->link[RIGHT]);
	(*n) = tmp;
}

/* COLOR FLIP */

/*
 * Flip the colors of the node and its children.
 * - When both links are red.
 */
void rb_color_flip(RB_node **n)
{
	(*n)->color = !(*n)->color;
	(*n)->link[LEFT]->color = !(*n)->link[LEFT]->color;
	(*n)->link[RIGHT]->color = !(*n)->link[RIGHT]->color;
	//n->color = rb_color_change(n->color);
	//n->link[LEFT]->color = rb_color_change(n->link[LEFT]->color);
	//n->link[RIGHT]->color = rb_color_change(n->link[RIGHT]->color);
}

/*
 * Color change
 */
Color rb_color_change(Color c) { return c == RED ? BLACK : RED; }

/*
 * Get what color the node has
 */
Color rb_color(RB_node *n) { return n->color; }

/* UTILITY */

/*
 * They are for comparison as function pointers
 * s - compare p1 and p2 in string(char)
 * i - compare p1 and p2 in integer
 * If p1 is greater than p2, return positive int values. If p1 is less than p2, return negative int
 * values. If p1 is the same as p2, return 0.
 */

int comparison_s(const void *p1, const void *p2)
{
	return strcmp((const char*)p1, (const char*)p2);
}
int comparison_i(const void *p1, const void *p2)
{
	return *(int*)p1 > *(int*)p2 ? 1 : *(int*)p1 < *(int*)p2 ? -1 : 0;
}

/* Print */

/*
 * Print a tree in some ways
 * 1 - inorder
 * 2 - preorder
 * 3 - postorder
 * 4 - each level in int
 * 5 - each level in string
 */
void rb_print(RB_tree *t, int order)
{
	rb_print_node(t->root, order);
}

/*
 * Print a node in some ways
 * 1 - inorder
 * 2 - preorder
 * 3 - postorder
 * 4 - each level in int
 * 5 - each level in string
 */
void rb_print_node(RB_node *n, int order)
{
	if (!n) {
		printf("it's empty.\n");
		return;
	}
    switch (order) {
        case 1: _print_inorder(n); break;
        case 2: _print_preorder(n); break;
        case 3: _print_postorder(n); break;
        case 4: _print_level_i(n); break;
        case 5: _print_level_s(n); break;
		case 6: _print_inorder_string(n); break;
        default : printf("Wrong number for order");
    }
    putchar('\n');
}

void _print_inorder_string(RB_node *n)
{
	if (!n) return;

	_print_inorder_string(n->link[LEFT]);
	printf("%s ", (char *)n->key);
	_print_inorder_string(n->link[RIGHT]);
}

/* root->left-> right */
void _print_preorder(RB_node *n)
{
    if (!n) return;

	int check = *(int *)n->key;
	if (isalpha(check))
		printf("%c ", check);
    printf("%d ", *(int *)n->key);

    _print_preorder(n->link[LEFT]);
    _print_preorder(n->link[RIGHT]);
}
/* left->root->right */
void _print_inorder(RB_node *n)
{
    if (!n) return;

    _print_inorder(n->link[LEFT]);

	int check = *(int *)n->key;
	if (isalpha(check))
		printf("%c ", check);
    printf("%d ", *(int *)n->key);

    _print_inorder(n->link[RIGHT]);
}
/* left->right->root */
void _print_postorder(RB_node *n)
{
    if (!n) return;

    _print_postorder(n->link[LEFT]);
    _print_postorder(n->link[RIGHT]);

	int check = *(int *)n->key;
	if (isalpha(check))
		printf("%c ", check);
    printf("%d ", *(int *)n->key);
}
/* each level */
void _print_level_i(RB_node *n)
{
    int depth = 0;
	Queue *q = qcreate();
    qpush(q, n);
    while (qsize(q)) {
        printf("Depth-%d:", depth);
        int size = qsize(q);
        for (int i = 0; i < size; i++) {
            RB_node *tmp = qfront(q);
            qpop(q);
			if (rb_is_red(tmp)) {
				printf(TEXT_RED);
				printf("%d(%d)", *(int*)tmp->key, tmp->size);
				printf(TEXT_RESET);
			} else
				printf("%d(%d)", *(int*)tmp->key, tmp->size);

            if (tmp->link[LEFT]) {
                printf("[L-%d]", *(int*)tmp->link[LEFT]->key);
                qpush(q, tmp->link[LEFT]);
            }
            if (tmp->link[RIGHT]) {
                printf("[R-%d]", *(int*)tmp->link[RIGHT]->key);
                qpush(q, tmp->link[RIGHT]);
            }
			putchar(' ');
        }
		putchar('\n');
        depth++;
    }
    printf("------------------------------------------------------------------------------------");
	free(q);
	q = NULL;
}

void _print_level_s(RB_node *n)
{
    int depth = 0;
	Queue *q = qcreate();
    qpush(q, n);
    while (qsize(q)) {
        printf("Depth-%d:", depth);
        int size = qsize(q);
        for (int i = 0; i < size; i++) {
            RB_node *tmp = qfront(q);
            qpop(q);
			if (rb_is_red(tmp)) {
				printf(TEXT_RED);
				printf("%s(%d)", (char*)tmp->key, tmp->size);
				printf(TEXT_RESET);
			} else
				printf("%s(%d)", (char*)tmp->key, tmp->size);

            if (tmp->link[LEFT]) {
                printf("[L-%s]", (char*)tmp->link[LEFT]->key);
                qpush(q, tmp->link[LEFT]);
            }
            if (tmp->link[RIGHT]) {
                printf("[R-%s]", (char*)tmp->link[RIGHT]->key);
                qpush(q, tmp->link[RIGHT]);
            }
			putchar(' ');
        }
		putchar('\n');
        depth++;
    }
    printf("------------------------------------------------------------------------------------");
	free(q);
	q = NULL;
}
