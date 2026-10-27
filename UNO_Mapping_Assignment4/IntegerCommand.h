#pragma once
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>

// UART protocol: one flat JSON object containing integer-valued fields only.
// No heap, token array or JSON document. Input remains owned by serialInput().
class IntegerCommand {
 struct Key {const char *p;uint8_t len;};
 const char *text=nullptr;
 static void spaces(const char *&p){while(*p==' '||*p=='\r'||*p=='\n'||*p=='\t')++p;}
 static bool member(const char *&p,Key &key,int32_t &value){
  spaces(p);if(*p++!='"')return false;
  key.p=p;key.len=0;
  while(*p&&*p!='"'){
   char c=*p++;if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_')||++key.len>8)return false;
  }
  if(!key.len||*p++!='"')return false;
  spaces(p);if(*p++!=':')return false;spaces(p);
  bool negative=*p=='-';if(negative)++p;
  if(*p<'0'||*p>'9')return false;
  if(*p=='0'&&p[1]>='0'&&p[1]<='9')return false;
  uint32_t magnitude=0,limit=negative?2147483648UL:2147483647UL;
  do{uint8_t digit=*p++-'0';if(magnitude>limit/10||(magnitude==limit/10&&digit>limit%10))return false;magnitude=magnitude*10+digit;}while(*p>='0'&&*p<='9');
  value=negative?(magnitude?-(int32_t)(magnitude-1)-1:0):(int32_t)magnitude;
  spaces(p);return *p==','||*p=='}';
 }
public:
 struct Field {
  int32_t value;bool present;
  bool isInt16()const{return present&&value>=-32768L&&value<=32767L;}
  bool isUInt16()const{return present&&value>=0&&value<=65535L;}
  bool isInt32()const{return present;}
  operator long()const{return present?value:0;}
  long orDefault(long fallback)const{return present?value:fallback;}
 };
 bool begin(const char *input){
  text=nullptr;if(!input)return false;
  const char *p=input;spaces(p);if(*p++!='{')return false;spaces(p);
  const char *first=p;uint8_t count=0;
  if(*p=='}'){++p;spaces(p);if(*p)return false;text=first;return true;}
  for(;;){
   const char *current=p;Key key;int32_t value;
   if(++count>16||!member(p,key,value))return false;
   // Reject duplicate fields so a damaged packet cannot change a field's meaning.
   const char *q=first;
   while(q<current){Key prior;int32_t ignored;if(!member(q,prior,ignored))return false;
    if(prior.len==key.len&&!memcmp(prior.p,key.p,key.len))return false;
    ++q;spaces(q);
   }
   if(*p=='}'){++p;spaces(p);if(*p)return false;text=first;return true;}
   ++p;spaces(p); // comma; next iteration must contain another field
  }
 }
 Field operator[](const char *flashKey)const{
  if(!text)return {0,false};
  const char *p=text;
  while(*p!='}'){
   Key key;int32_t value;if(!member(p,key,value))return {0,false};
   uint8_t i=0;while(i<key.len&&key.p[i]==(char)pgm_read_byte(flashKey+i))++i;
   if(i==key.len&&pgm_read_byte(flashKey+i)==0)return {value,true};
   if(*p=='}')break;++p;spaces(p);
  }
  return {0,false};
 }
 bool containsKey(const char *flashKey)const{return (*this)[flashKey].present;}
};
