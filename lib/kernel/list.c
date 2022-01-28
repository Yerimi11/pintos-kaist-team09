#include "list.h"
#include "../debug.h"

/* Our doubly linked lists have two header elements: the "head"
   just before the first element and the "tail" just after the
   last element.  The `prev' link of the front header is null, as
   is the `next' link of the back header.  Their other two links
   point toward each other via the interior elements of the list.
	이중으로 연결된 목록에는 두 가지 헤더 요소가 있다: 
	첫 번째 요소 바로 앞에 있는 "머리"와 마지막 요소 바로 뒤에 있는 "꼬리". 
	프론트 헤더의 'prev' 링크는 null이고, 백 헤더의 'next' 링크도 null이다. 
	그들의 다른 두 링크는 목록의 내부 요소를 통해 서로를 가리킨다. 빈 목록은 다음과 같다.
   An empty list looks like this:

   		+------+     +------+
    <---| head |<--->| tail |--->
  		+------+     +------+

   A list with two elements in it looks like this:

   		+------+     +-------+     +-------+     +------+
    <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
   		+------+     +-------+     +-------+     +------+

   The symmetry of this arrangement eliminates lots of special
   cases in list processing.  For example, take a look at
   list_remove(): it takes only two pointer assignments and no
   conditionals.  That's a lot simpler than the code would be
   without header elements.
	이러한 배열의 대칭은 목록 처리에서 많은 특별한 경우를 제거한다. 
	예를 들어, list_remove pointer: 두 개의 포인터 할당만 필요하며 조건은 없다. 
	헤더 요소가 없는 코드보다 훨씬 간단하다.
	
   (Because only one of the pointers in each header element is used,
   we could in fact combine them into a single header element
   without sacrificing this simplicity.  But using two separate
   elements allows us to do a little bit of checking on some
   operations, which can be valuable.) */
	/* (왜냐면 각 헤더 요소에서 하나의 포인터만 사용되기 때문에, 
	우리는 사실 이러한 단순성을 희생시키지 않고 이들을 단일 헤더 요소로 결합할 수 있다. 
	그러나 두 개의 별도 요소를 사용하면 일부 작업을 조금이나마 확인할 수 있으므로 유용하게 사용할 수 있다.) */

static bool is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) UNUSED;

/* Returns true if ELEM is a head, false otherwise. */
static inline bool
is_head (struct list_elem *elem) {
	return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* Returns true if ELEM is an interior element,
   false otherwise. */
static inline bool
is_interior (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* Returns true if ELEM is a tail, false otherwise. */
static inline bool
is_tail (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

/* Initializes LIST as an empty list. */
void
list_init (struct list *list) { // list의 주소값을 포인터로 선언
	ASSERT (list != NULL); // ASSERT 오류감지. 조건이 True여야 넘어감
	list->head.prev = NULL;
	list->head.next = &list->tail; // 주소로 찾아감 = 값으로 찾아감
	list->tail.prev = &list->head; // list값으로 찾아가서 찾은 head는 list의 주소로 찾아간 tail.prev이다
	list->tail.next = NULL;
}

/* Returns the beginning of LIST. */
// list_begin이라는 함수가 실행될 때 - ASSERT뒤의 조건이 True이면 - list_elem의 구조체 타입으로 return해준다
struct list_elem *list_begin (struct list *list) {
	ASSERT (list != NULL);
	return list->head.next;
}

/* Returns the element after ELEM in its list. If ELEM is the last element in its list, returns the list tail.  
   Results are undefined if ELEM is itself a list tail. */
struct list_elem *
list_next (struct list_elem *elem) {
	ASSERT (is_head (elem) || is_interior (elem)); // 중간일 때
	return elem->next;
}

/* Returns LIST's tail.

   list_end() is often used in iterating through a list from front to back.  
   See the big comment at the top of list.h for an example. */
struct list_elem *
list_end (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* Returns the LIST's reverse beginning, for iterating through
   LIST in reverse order, from back to front. */
struct list_elem *
list_rbegin (struct list *list) {
	ASSERT (list != NULL);
	return list->tail.prev;
}

/* Returns the element before ELEM in its list.  
   If ELEM is the first element in its list, returns the list head.  
    -> 첫번째 elem이면 head 리턴해라 (구현 안되있다고 밑줄 영어에 써있음)
   Results are undefined if ELEM is itself a list head. */
struct list_elem *
list_prev (struct list_elem *elem) {
	ASSERT (is_interior (elem) || is_tail (elem)); // 중간일 때
	return elem->prev; // 앞에껄 리턴해라 (윗윗줄에 list_prev라서)
}

/* Returns LIST's head.

   list_rend() is often used in iterating through a list in
   reverse order, from back to front.  Here's typical usage,
   following the example from the top of list.h:
   list_rends는 종종 뒤에서 앞으로 역순으로 목록을 반복하는 데 사용됩니다. 
   다음은 일반적인 용도이며, list.h의 맨 위에 있는 예를 따릅니다.

   for (e = list_rbegin (&foo_list); e != list_rend (&foo_list);
   e = list_prev (e))
   {
   struct foo *f = list_entry (e, struct foo, elem);
   ...do something with f...
   }
   */
struct list_elem *
list_rend (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* Return's LIST's head.

   list_head() can be used for an alternate style of iterating
   through a list, e.g.:

   e = list_head (&list);
   while ((e = list_next (e)) != list_end (&list))
   {
   ...
   }
   */
struct list_elem *
list_head (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* Return's LIST's tail. */
struct list_elem *
list_tail (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* Inserts ELEM just before BEFORE, which may be either an
   interior element or a tail.  The latter case is equivalent to
   list_push_back(). */
void
list_insert (struct list_elem *before, struct list_elem *elem) {
	ASSERT (is_interior (before) || is_tail (before));
	ASSERT (elem != NULL);

	elem->prev = before->prev;
	elem->next = before;
	before->prev->next = elem;
	before->prev = elem; // prev <-> elem <-> before 순으로 연결 됨.
} /* 2번째 인자에 해당하는 스레드가 1번째 인자에 해당하는 스레드 앞에 위치하게 된다 */

/* Removes elements FIRST though LAST (exclusive) from their
   current list, then inserts them just before BEFORE, which may
   be either an interior element or a tail. */
void
list_splice (struct list_elem *before,
		struct list_elem *first, struct list_elem *last) {
	ASSERT (is_interior (before) || is_tail (before));
	if (first == last)
		return;
	last = list_prev (last);

	ASSERT (is_interior (first));
	ASSERT (is_interior (last));

	/* Cleanly remove FIRST...LAST from its current list. */
	first->prev->next = last->next;
	last->next->prev = first->prev;

	/* Splice FIRST...LAST into new list. */
	first->prev = before->prev;
	last->next = before;
	before->prev->next = first;
	before->prev = last;
}

/* Inserts ELEM at the beginning of LIST, so that it becomes the
   front in LIST. */
void
list_push_front (struct list *list, struct list_elem *elem) {
	list_insert (list_begin (list), elem);
}

/* Inserts ELEM at the end of LIST, so that it becomes the
   back in LIST. */
void
list_push_back (struct list *list, struct list_elem *elem) {
	list_insert (list_end (list), elem);
}

/* Removes ELEM from its list and returns the element that
   followed it.  Undefined behavior if ELEM is not in a list.

   It's not safe to treat ELEM as an element in a list after
   removing it.  In particular, using list_next() or list_prev()
   on ELEM after removal yields undefined behavior.  This means
   that a naive loop to remove the elements in a list will fail:

 ** DON'T DO THIS **
 for (e = list_begin (&list); e != list_end (&list); e = list_next (e))
 {
 ...do something with e...
 list_remove (e);
 }
 ** DON'T DO THIS **

 Here is one correct way to iterate and remove elements from a
list:

for (e = list_begin (&list); e != list_end (&list); e = list_remove (e))
{
...do something with e...
}

If you need to free() elements of the list then you need to be
more conservative.  Here's an alternate strategy that works
even in that case:

while (!list_empty (&list))
{
struct list_elem *e = list_pop_front (&list);
...do something with e...
}
*/
struct list_elem *
list_remove (struct list_elem *elem) { // elem : 지울 애
	ASSERT (is_interior (elem)); 
	elem->prev->next = elem->next; // 지울애의 다음을 = 지울애의 전꺼의 다음으로 한다
	elem->next->prev = elem->prev;
	return elem->next; // 지울애 빼고 앞 뒤 노드를 연결해준다
}

/* Removes the front element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
struct list_elem *
list_pop_front (struct list *list) {
	struct list_elem *front = list_front (list);
	list_remove (front);
	return front;
}

/* Removes the back element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
struct list_elem *
list_pop_back (struct list *list) {
	struct list_elem *back = list_back (list);
	list_remove (back);
	return back;
}

/* Returns the front element in LIST.
   Undefined behavior if LIST is empty. */
struct list_elem *
list_front (struct list *list) {
	ASSERT (!list_empty (list));
	return list->head.next;
}

/* Returns the back element in LIST.
   Undefined behavior if LIST is empty. */
struct list_elem *
list_back (struct list *list) {
	ASSERT (!list_empty (list));
	return list->tail.prev;
}

/* Returns the number of elements in LIST.
   Runs in O(n) in the number of elements. */
size_t
list_size (struct list *list) {
	struct list_elem *e; // list_begin의 리턴타입이 구조체 포인터이므로 그 값을 담기 위해서는 e도 같은 구조체 포인터 타입으로 선언해야해서 *붙였고 이건 주소값을 담음 
	size_t cnt = 0;

	for (e = list_begin (list); e != list_end (list); e = list_next (e))
		cnt++;
	return cnt;
}

/* Returns true if LIST is empty, false otherwise. */
bool
list_empty (struct list *list) {
	return list_begin (list) == list_end (list);
}

/* Swaps the `struct list_elem *'s that A and B point to. */
static void
swap (struct list_elem **a, struct list_elem **b) {
	struct list_elem *t = *a;
	*a = *b;
	*b = t;
}

/* Reverses the order of LIST. */
void
list_reverse (struct list *list) {
	if (!list_empty (list)) {
		struct list_elem *e;

		for (e = list_begin (list); e != list_end (list); e = e->prev)
			swap (&e->prev, &e->next);
		swap (&list->head.next, &list->tail.prev);
		swap (&list->head.next->prev, &list->tail.prev->next);
	}
}

/* Returns true only if the list elements A through B (exclusive)
   are in order according to LESS given auxiliary data AUX. */
static bool
is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	if (a != b)
		while ((a = list_next (a)) != b)
			if (less (a, list_prev (a), aux))
				return false;
	return true;
}

/* Finds a run, starting at A and ending not after B, of list
   elements that are in nondecreasing order according to LESS
   given auxiliary data AUX.  Returns the (exclusive) end of the
   run.
   A through B (exclusive) must form a non-empty range. */
static struct list_elem *
find_end_of_run (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	ASSERT (a != NULL);
	ASSERT (b != NULL);
	ASSERT (less != NULL);
	ASSERT (a != b);

	do {
		a = list_next (a);
	} while (a != b && !less (a, list_prev (a), aux));
	return a;
}

/* Merges A0 through A1B0 (exclusive) with A1B0 through B1
   (exclusive) to form a combined range also ending at B1
   (exclusive).  Both input ranges must be nonempty and sorted in
   nondecreasing order according to LESS given auxiliary data
   AUX.  The output range will be sorted the same way. */
static void
inplace_merge (struct list_elem *a0, struct list_elem *a1b0,
		struct list_elem *b1,
		list_less_func *less, void *aux) {
	ASSERT (a0 != NULL);
	ASSERT (a1b0 != NULL);
	ASSERT (b1 != NULL);
	ASSERT (less != NULL);
	ASSERT (is_sorted (a0, a1b0, less, aux));
	ASSERT (is_sorted (a1b0, b1, less, aux));

	while (a0 != a1b0 && a1b0 != b1)
		if (!less (a1b0, a0, aux))
			a0 = list_next (a0);
		else {
			a1b0 = list_next (a1b0);
			list_splice (a0, list_prev (a1b0), a1b0);
		}
}

/* Sorts LIST according to LESS given auxiliary data AUX, using a
   natural iterative merge sort that runs in O(n lg n) time and
   O(1) space in the number of elements in LIST. */
void
list_sort (struct list *list, list_less_func *less, void *aux) {
	size_t output_run_cnt;        /* Number of runs output in current pass. */

	ASSERT (list != NULL);
	ASSERT (less != NULL);

	/* Pass over the list repeatedly, merging adjacent runs of
	   nondecreasing elements, until only one run is left. */
	do {
		struct list_elem *a0;     /* Start of first run. */
		struct list_elem *a1b0;   /* End of first run, start of second. */
		struct list_elem *b1;     /* End of second run. */

		output_run_cnt = 0;
		for (a0 = list_begin (list); a0 != list_end (list); a0 = b1) {
			/* Each iteration produces one output run. */
			output_run_cnt++;

			/* Locate two adjacent runs of nondecreasing elements
			   A0...A1B0 and A1B0...B1. */
			a1b0 = find_end_of_run (a0, list_end (list), less, aux);
			if (a1b0 == list_end (list))
				break;
			b1 = find_end_of_run (a1b0, list_end (list), less, aux);

			/* Merge the runs. */
			inplace_merge (a0, a1b0, b1, less, aux);
		}
	}
	while (output_run_cnt > 1);

	ASSERT (is_sorted (list_begin (list), list_end (list), less, aux));
}

/* Inserts ELEM in the proper position in LIST, which must be
   sorted according to LESS given auxiliary data AUX.
   Runs in O(n) average case in the number of elements in LIST. */
void			/* 스레드 넣을 리스트, 넣고자하는 스레드, 삽입시 기준이 될 함수 */
list_insert_ordered (struct list *list, struct list_elem *elem,
		list_less_func *less, void *aux) {
	struct list_elem *e;

	ASSERT (list != NULL);
	ASSERT (elem != NULL);
	ASSERT (less != NULL);

	for (e = list_begin (list); e != list_end (list); e = list_next (e))
		if (less (elem, e, aux))
			break;
	// ready_list는 우선순위가 높은 순으로 정렬되어 있어야 한다고 하였다.
	// 따라서 list_insert_ordered( ) 함수 내부의 less( ) 함수는 
	// thread.c의 thread_compare_priority함수처럼 elem > e 일 때 true를 반환하는 함수여야 한다.

	return list_insert (e, elem); // elem <-> e 순서로 된다
} /* 2번째 인자에 해당하는 스레드가 1번째 인자에 해당하는 스레드 앞에 위치하게 된다 */

/* Iterates through LIST and removes all but the first in each
   set of adjacent elements that are equal according to LESS
   given auxiliary data AUX.  If DUPLICATES is non-null, then the
   elements from LIST are appended to DUPLICATES. */
void
list_unique (struct list *list, struct list *duplicates,
		list_less_func *less, void *aux) {
	struct list_elem *elem, *next;

	ASSERT (list != NULL);
	ASSERT (less != NULL);
	if (list_empty (list))
		return;

	elem = list_begin (list);
	while ((next = list_next (elem)) != list_end (list))
		if (!less (elem, next, aux) && !less (next, elem, aux)) {
			list_remove (next);
			if (duplicates != NULL)
				list_push_back (duplicates, next);
		} else
			elem = next;
}

/* Returns the element in LIST with the largest value according
   to LESS given auxiliary data AUX.  If there is more than one
   maximum, returns the one that appears earlier in the list.  If
   the list is empty, returns its tail. */
struct list_elem *
list_max (struct list *list, list_less_func *less, void *aux) {
	struct list_elem *max = list_begin (list);
	if (max != list_end (list)) {
		struct list_elem *e;

		for (e = list_next (max); e != list_end (list); e = list_next (e))
			if (less (max, e, aux))
				max = e;
	}
	return max;
}

/* Returns the element in LIST with the smallest value according
   to LESS given auxiliary data AUX.  If there is more than one
   minimum, returns the one that appears earlier in the list.  If
   the list is empty, returns its tail. */
struct list_elem *
list_min (struct list *list, list_less_func *less, void *aux) {
	struct list_elem *min = list_begin (list);
	if (min != list_end (list)) {
		struct list_elem *e;

		for (e = list_next (min); e != list_end (list); e = list_next (e))
			if (less (e, min, aux))
				min = e;
	}
	return min;
}
