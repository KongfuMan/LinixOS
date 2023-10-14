// a block buffer uniquely identified by <dev, blockno>
struct buf {
  int valid;   // only invalid buf can be reclaimed
  int dirty;   // has data been updated?
  int disk;    // does disk "own" buf?
  uint dev;    // device type enum
  uint blockno;
  // struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

