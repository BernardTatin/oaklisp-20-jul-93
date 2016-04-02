
/* emulator.c */
extern void maybe_dump_world_aux(int);
extern void printref(ref);
extern void old_find_method_type_pair(ref, ref, ref *, ref *);
extern void find_method_type_pair(ref, ref, ref *, ref *);
extern char *oak_c_string(unsigned int, unsigned long *);
extern void crunch_args(int, char **);
extern int main(int, char **);

/* mymalloc.c */
extern char *my_malloc(long);
extern void realloc_space(space *, ref *);
extern void free_space(space *);
extern void alloc_space(space *);

/* worldio.c */
extern ref contig(ref, bool);
extern long flread(char *, unsigned int, long, FILE *);
extern ref read_ref(FILE *);
extern FILE *prompt_file(char *, char *);
extern void dump_binary_world(bool);
extern void dump_ascii_world(bool);
extern void dump_world(bool);
extern void reoffset(ref, ref *, long);
extern void read_world(char *);
extern long string_to_int(char *);

/* gc.c */
extern void gc_printref(ref);
extern unsigned long gc_get_length(ref);
extern ref gc_touch0(ref);
extern ref loc_touch0(ref, bool);
extern void scavenge(void);
extern void loc_scavenge(void);
extern bool gc_check(ref);
extern void GC_CHECK(ref, char *);
extern void GC_CHECK1(ref, char *, long);
extern unsigned short *pc_touch(unsigned short *);
extern void set_external_full_gc(bool);
extern void gc(bool, bool, char *, long);

/* stacks.c */
extern void flush_buffer(stack *, int);
extern void unflush_buffer(stack *, int);
extern void dump_stack_proc(stack *);
extern void init_stk(stack *);

/* weak.c */
extern void init_wp(void);
extern void enter_wp(ref, ref );
extern void rebuild_wp_hashtable(void);
extern ref ref_to_wp(ref);
extern void wp_hashtable_distribution(void);
extern unsigned long post_gc_wp(void);

/* signal.c */
extern void enable_signal_polling(void);
extern void disable_signal_polling(void);
extern void enable_signal_polling(void);
extern void disable_signal_polling(void);
extern void clear_signal(void);
