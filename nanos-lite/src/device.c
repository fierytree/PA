#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  char str[20];
  bool down=false;
  int key=_read_key();
  if(key&0x8000){
    key^=0x8000;
    down=true;
  }
  if(key!=_KEY_NONE)
  sprintf(str,"%s %s\n",down?"kd":"ku",keyname[key]);
  else sprintf(str,"t %d\n",_uptime());

  if(strlen(str)<=len){
    strncpy((char*)buf,str,strlen(str));
    return strlen(str);
  }
  Log("strlen(event)>len,return 0");
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf,dispinfo+offset,len);
}

extern void getScreen(int* p_width,int* p_height);
void fb_write(const void *buf, off_t offset, size_t len) {
  assert(offset%4==0&&len%4==0);

  int width=0,height=0;
  getScreen(&width,&height);
  int x1=offset/4%width;
  int y1=offset/4/width;
  int y2=(offset+len)/4/width;
  assert(y2>=y1);

  if(y1==y2){
    _draw_rect(buf,x1,y1,len/4,1);
    return;
  }
  int w1=width-x1;
  if(y2-y1==1){
    _draw_rect(buf,x1,y1,w1,1);
    _draw_rect(buf+w1*4,0,y2,len/4-w1,1);
    return;
  }
  int midy=y2-y1-1;
  _draw_rect(buf,x1,y1,w1,1);
  _draw_rect(buf+w1*4,0,y1+1,width,midy);
  _draw_rect(buf+w1*4+midy*width*4,0,y2,len/4-w1-midy*width,1);
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  int width=0,height=0;
  getScreen(&width,&height);
  sprintf(dispinfo,"WIDTH:%d\nHEIGHT:%d\n",width,height);
}
