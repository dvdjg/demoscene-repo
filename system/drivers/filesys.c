/*
 * Minimal read-only filesystem for demo disk images.
 *
 * Purpose: maps a simple directory of FileEntry records (CRC32, sector start,
 * size) on top of a FloppyOpen device; FileRead seeks sector-wise and verifies
 * checksums. This is not AmigaDOS FFS — it is a compact demo packer format.
 */
#include <debug.h>
#include <common.h>
#include <string.h>
#include <types.h>
#include <crc32.h>
#include <system/errno.h>
#include <system/file.h>
#include <system/filesys.h>
#include <system/floppy.h>
#include <system/memory.h>

#define IOF_EOF 0x0002
#define IOF_ERR 0x8000

/* On disk directory entries are always aligned to 2-byte boundary. */
typedef struct FileEntry {
  u_char   reclen;   /* total size of this record in bytes */
  u_char   type;     /* type of file (1: executable, 0: regular) */
  u_short  start;    /* sector where the file begins (0..1759) */
  u_int    size;     /* file size in bytes (up to 1MiB) */
  u_int    cksum;    /* crc32 checksum on file data */
  char     name[0];  /* name of the file (NUL terminated) */
} FileEntryT;

static FileEntryT *NextFileEntry(FileEntryT *fe) {
  return (void *)fe + fe->reclen;
}

/* Mounted block device and parsed root directory table. */
static FileT *FileSysDev;
/* Finished by NUL character (reclen = 0). */
static FileEntryT *FileSysRootDir;

struct File {
  FileOpsT *ops;
  /* Current byte position inside this file. */
  int pos;
  u_short flags;

  /* Byte range in underlying device where this file payload lives. */
  u_int start;
  u_int size;
};

static int FsRead(FileT *f, void *buf, u_int nbyte);
static int FsSeek(FileT *f, int offset, int whence);
static void FsClose(FileT *f);

static FileOpsT FsOps = {
  .read = FsRead,
  .write = NoWrite,
  .seek = FsSeek,
  .close = FsClose
};

/* LookupFile — linear scan in custom directory format by filename. */
static FileEntryT *LookupFile(const char *path) {
  FileEntryT *fe = FileSysRootDir;

  if (fe == NULL)
    return NULL;

  do {
    if (strcmp(path, fe->name) == 0)
      return fe;
    fe = NextFileEntry(fe);
  } while (fe->reclen);

  return NULL;
}

FileT *OpenFile(const char *path) {
  FileEntryT *entry;
  FileT *f;

  if (!(entry = LookupFile(path)))
    return NULL;

  f = MemAlloc(sizeof(FileT), MEMF_PUBLIC|MEMF_CLEAR);
  f->ops = &FsOps;
  /* +2 sectors skips disk header + root directory area in this FS layout. */
  f->start = (entry->start + 2) * SECTOR_SIZE;
  f->size = entry->size;

  Debug("%s: %d+%d", path, f->start, f->size);

  return f;
}

static void FsClose(FileT *f) {
  MemFree(f);
}

static int FsRead(FileT *f, void *buf, u_int nbyte) {
  u_int left = nbyte;
  int res;

  Assume(f != NULL);

  if (f->flags & IOF_ERR)
    return EIO;

  Debug("$%p $%p %d+%d", f, buf, f->pos, nbyte);

  /* Clamp read request to remaining file bytes. */
  left = min(left, f->size - f->pos);

  /* Underlying device is random-access; seek absolute payload position first. */
  (void)FileSeek(FileSysDev, f->pos + f->start, SEEK_SET);

  if ((res = FileRead(FileSysDev, buf, left)) < 0)
    return res;

  f->pos += res;

  return res;
}

static int FsSeek(FileT *f, int offset, int whence) {
  if (f->flags & IOF_ERR)
    return EIO;

  Debug("$%p %d %d", f, offset, whence);

  f->flags &= ~IOF_EOF;

  if (whence == SEEK_CUR) {
    offset += f->pos;
    whence = SEEK_SET;
  }

  if (whence == SEEK_END) {
    offset = f->size - offset;
    whence = SEEK_SET;
  }

  if (whence == SEEK_SET) {
    /* New position is not within file. */
    if ((offset < 0) || (offset > (int)f->size))
      return EINVAL;

    f->pos = offset;
    return offset;
  }

  return EINVAL;
}

#define ONSTACK(x) (&(x)), sizeof((x))

/* InitFileSys — mount simple directory table from already opened floppy/file device. */
void InitFileSys(FileT *dev) {
  u_short rootDirLen;

  Assume(dev != NULL);

  FileSysDev = dev;

  /* read directory size */
  FileSeek(dev, SECTOR_SIZE * 2, SEEK_SET);
  FileRead(dev, ONSTACK(rootDirLen));

  Log("[FileSys] Reading directory of %d bytes.\n", rootDirLen);

  /* read directory entries */
  FileSysRootDir = MemAlloc(rootDirLen + 1, MEMF_PUBLIC|MEMF_CLEAR);
  FileRead(dev, FileSysRootDir, rootDirLen);

  /* associate names with file entries */
  {
    FileEntryT *fe = FileSysRootDir;
    do {
      Log("[FileSys] Sector %d: %s file '%s' of %d bytes.\n",
          fe->start, fe->type ? "executable" : "regular", fe->name, fe->size);
      fe = NextFileEntry(fe);
    } while (fe->reclen);
  }
}

void KillFileSys(void) {
  if (FileSysRootDir) {
    /* Mounted device is owned by filesystem layer after InitFileSys(). */
    FileClose(FileSysDev);
    MemFree(FileSysRootDir);
    FileSysDev = NULL;
    FileSysRootDir = NULL;
  }
}

/* ChecksumFile — full-file CRC32 verification against directory metadata. */
static void ChecksumFile(const char *path, u_int size, u_int cksum) {
  FileT *f = OpenFile(path);
  u_char *data = MemAlloc(size, MEMF_PUBLIC);
  u_int actual_cksum;

  FileRead(f, data, size);
  FileClose(f);
  actual_cksum = crc32(data, size);
  MemFree(data);

  if (actual_cksum != cksum) {
    Panic("[FileSys] File '%s' is corrupted - crc32: %08x!", path, actual_cksum);
  }
}

void CheckFileSys(void) {
  FileEntryT *fe = FileSysRootDir;

  Log("[FileSys] Checking data consistency.\n");

  do {
    Log("[FileSys] Checking file '%s' of %d bytes (crc32: %08x)\n",
        fe->name, fe->size, fe->cksum);
    ChecksumFile(fe->name, fe->size, fe->cksum);
    fe = NextFileEntry(fe);
  } while (fe->reclen);
}
