
/* 1/RECLAIM_FACTOR is the target for how much of new space should be used
   after a gc.  If more than this is used, the next new space allocated will
   be bigger. */

#define RECLAIM_FACTOR 3

#define OLD_PTR(r) (SPACE_PTR(old,(r)) || full_gc && SPACE_PTR(spatic,(r)))

extern space new, spatic, old;
extern bool trace_gc, full_gc;
extern unsigned long post_gc_wp();
