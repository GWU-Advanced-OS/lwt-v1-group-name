#ifndef LWT_LIST_H
#define LWT_LIST_H

struct ps_list {
		struct ps_list *n, *p;

};

/*
 *  * This is a separate type to 1) provide guidance on how to use the
 *   * API, and 2) to prevent developers from comparing pointers that
 *    * should not be compared.
 *     */
struct ps_list_head {
		struct ps_list l;

};

#define PS_LIST_DEF_NAME list

static inline void
ps_list_ll_init(struct ps_list *l)
{ l->n = l->p = l;  }

static inline void
ps_list_head_init(struct ps_list_head *lh)
{ ps_list_ll_init(&lh->l);  }

static inline int
ps_list_ll_empty(struct ps_list *l)
{ return l->n == l;  }

static inline int
ps_list_head_empty(struct ps_list_head *lh)
{ return ps_list_ll_empty(&lh->l);  }

static inline void
ps_list_ll_add(struct ps_list *l, struct ps_list *new)
{
		new->n    = l->n;
			new->p    = l;
				l->n      = new;
					new->n->p = new;

}

static inline void
ps_list_ll_rem(struct ps_list *l)
{
		l->n->p = l->p;
			l->p->n = l->n;
				l->p = l->n = l;

}
#endif
