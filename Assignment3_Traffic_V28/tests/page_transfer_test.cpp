#include <cassert>
#include <vector>
#include <algorithm>
#include "../ESP32_Assignment2/PageTransfer.h"
struct Client{std::vector<uint8_t> received;int calls=0;bool open=true,stall=false,disconnect=false;
 bool connected(){return open;}void stop(){open=false;}
 size_t write(const uint8_t *b,size_t n){assert(n<=1024);calls++;if(disconnect){open=false;return 0;}if(stall||calls%4==0)return 0;n=std::min(n,size_t(117));received.insert(received.end(),b,b+n);return n;}};
int main(){std::vector<uint8_t> data(27000);for(size_t i=0;i<data.size();i++)data[i]=i%251;
 uint32_t now=0;auto clock=[&]{return now;};auto yield=[&]{now++;};Client c;
 assert(writePageBody(c,data.data(),data.size(),clock,yield)==data.size());assert(c.received==data&&c.open);
 Client dead;dead.stall=true;now=0;assert(writePageBody(dead,data.data(),data.size(),clock,yield)==0);assert(!dead.open&&now==8000);
 Client gone;gone.disconnect=true;now=0;assert(writePageBody(gone,data.data(),data.size(),clock,yield)==0&&!gone.open);
}
