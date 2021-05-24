#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))
#define CHECK_FD assert(fd>=0&&fd<NR_FILES); 

inline size_t size(int fd){
  CHECK_FD;
  return file_table[fd].size;
}

inline off_t disk_offset(int fd){
  CHECK_FD;
  return file_table[fd].disk_offset;
}

inline off_t open_offset(int fd){
  CHECK_FD;
  return file_table[fd].open_offset;
}

void set_open_offset(int fd,off_t n){
  CHECK_FD;
  assert(n>=0);
  if(n>file_table[fd].size)n=file_table[fd].size;
  file_table[fd].open_offset=n;
}

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);

int fs_open(const char* filename, int flags,int mode){
  for(int i=0;i<NR_FILES;i++){
    // Log("file name: %s",file_table[i].name);
    if(strcmp(filename,file_table[i].name)==0)return i;
  }
  panic("this filename does not exist in file table");
  return -1;
}

size_t events_read(void *buf, size_t len);
void dispinfo_read(void *buf, off_t offset, size_t len);
ssize_t fs_read(int fd,void* buf,size_t len){
  CHECK_FD;
  if(fd<3||fd==FD_FB){
    Log("arg invalid:fd<3||fd==FD_FB");
    return 0;
  }
  if(fd==FD_EVENTS)return events_read(buf,len);
  int n=size(fd)-open_offset(fd);
  if(n>len)n=len;

  if(fd==FD_DISPINFO)
    dispinfo_read(buf,open_offset(fd),n);
  else ramdisk_read(buf,disk_offset(fd)+open_offset(fd),n);
  set_open_offset(fd,open_offset(fd)+n);
  return n;
}

int fs_close(int fd){
  CHECK_FD;
  return 0;
}

void fb_write(const void *buf, off_t offset, size_t len);
ssize_t fs_write(int fd,void* buf,size_t len){
  CHECK_FD;
  if(fd<3||fd==FD_DISPINFO){
    Log("arg invalid:fd<3||fd==FD_DISPINFO");
    return 0;
  }
  int n=size(fd)-open_offset(fd);
  if(n>len)n=len;

  if(fd==FD_FB)fb_write(buf,open_offset(fd),n);
  else ramdisk_write(buf,disk_offset(fd)+open_offset(fd),n);
  set_open_offset(fd,open_offset(fd)+n);
  return n;
}

off_t fs_lseek(int fd,off_t offset,int whence){
  switch(whence){
    case SEEK_SET:
      set_open_offset(fd,offset);
      return open_offset(fd);
    case SEEK_CUR:
      set_open_offset(fd,open_offset(fd)+offset);
      return open_offset(fd);
    case SEEK_END:
      set_open_offset(fd,size(fd)+offset);
      return open_offset(fd);
    default:
      panic("unhandled whence ID=%d",whence);
      return -1;
  }
}

void init_fs() {
  // TODO: initialize the size of /dev/fb
  extern void getScreen(int* p_width,int* p_height);
  int width=0,height=0;
  getScreen(&width,&height);
  file_table[FD_FB].size=width*height*sizeof(uint32_t);
  Log("set FD_FB size=%d",file_table[FD_FB].size);
}
