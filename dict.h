/*
  Dictionary based on code by Morten Eriksen <mortene@sim.no>.
*/

struct dict;

extern struct dict *dict_init(unsigned int (*key2hash) (void *),
			      int (*key_cmp) (void *, void *));
extern void dict_clear(struct dict *d);
extern int dict_enter(struct dict *d, void *key, void *value);
extern void *dict_find_entry(struct dict *d, void *key);
extern void dict_apply_to_all(struct dict *d,
			      void (*func) (void *key, void *value, void *data),
			      void *data);

extern unsigned int dict_key2hash_string(void *key);
extern int dict_key_cmp_string(void *key1, void *key2);
extern unsigned int dict_key2hash_int(void *key);
extern int dict_key_cmp_int(void *key1, void *key2);
extern struct dict * dict_clone(struct dict *old, void * (*key_clone)(void*), void * (*value_clone)(void*));
