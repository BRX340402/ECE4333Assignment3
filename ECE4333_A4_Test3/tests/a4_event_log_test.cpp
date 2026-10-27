#include <cassert>
#include "../UNO_Test3/A4EventLog.h"
int main(){
 A4EventLog log;
 A4EventLog::Event e={};
 assert(!log.peek(e));
 for(int i=0;i<3;i++){e.t=i+1;log.push(e);}
 assert(log.count==3&&log.lost==0);
 e.t=4;log.push(e);
 assert(log.count==3&&log.lost==1); // oldest record overwritten
 A4EventLog::Event out={};
 assert(log.peek(out)&&out.t==2);
 log.remove();assert(log.peek(out)&&out.t==3);
 log.remove();assert(log.peek(out)&&out.t==4);
 log.remove();assert(!log.peek(out));
 return 0;
}
