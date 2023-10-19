// a block buffer uniquely identified by <dev, blockno>
struct buf {
  int valid;   // only invalid buf can be reclaimed
  int dirty;   // has data been updated?
  int disk;    // does disk "own" buf?
  uint dev;    // device type enum
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev_lru;     // doubly linked list of lru replacer.
  struct buf *next_lru;

  struct buf *prev_hash; // doubly linked list of hashtable.
  struct buf *next_hash;

  struct buf *prev_free; // doubly linked list of free list.
  struct buf *next_free;
  uchar data[BSIZE];
};

