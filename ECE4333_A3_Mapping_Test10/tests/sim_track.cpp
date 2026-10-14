// Host simulation of the Test10 line follower on the figure-8 track drawing.
// Not a substitute for a real run: it checks the decision logic (crossing goes
// straight, corners turn the right way, never reverses, laps complete) over a
// sweep of unknown physical parameters.
//   g++ -O2 -std=c++14 -I../UNO_Mapping sim_track.cpp -o sim && ./sim
//   ./sim --dump run.jsonl   (one representative run, UNO-format N230 lines)
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <vector>
#include <random>
#include <string>
#include "LineFollower.h"

struct P{double x,y;};
static const double DEG=M_PI/180.0;

// Route from the drawing, cm. Start C, heading 0 = toward the STOP sign.
std::vector<P> corners(){
  P C{0,0},O{150,0},D{320,0},E{150+120*cos(-60*DEG),120*sin(-60*DEG)};
  P A{150+100*cos(120*DEG),100*sin(120*DEG)};
  // close the lower loop: A->B heading 150, B->C heading 240
  double a=43.30127,b=125.0; (void)b;
  P B{A.x+a*cos(150*DEG),A.y+a*sin(150*DEG)};
  return {C,O,D,E,O,A,B};
}
// dense closed centreline with optional corner fillets of radius R (O is not a corner)
std::vector<P> dense(double R){
  auto c=corners();int n=c.size();std::vector<P> out;
  auto isCorner=[&](int i){return !(i==1||i==4);};
  std::vector<P> pts; // replace each corner by an arc
  for(int i=0;i<n;i++){
    P prev=c[(i+n-1)%n],cur=c[i],next=c[(i+1)%n];
    if(!isCorner(i)||R<=0){pts.push_back(cur);continue;}
    double ux=prev.x-cur.x,uy=prev.y-cur.y,vx=next.x-cur.x,vy=next.y-cur.y;
    double lu=hypot(ux,uy),lv=hypot(vx,vy);ux/=lu;uy/=lu;vx/=lv;vy/=lv;
    double ang=acos(std::max(-1.0,std::min(1.0,ux*vx+uy*vy))); // interior angle
    double t=R/tan(ang/2);
    P t1{cur.x+ux*t,cur.y+uy*t},t2{cur.x+vx*t,cur.y+vy*t};
    double bx=ux+vx,by=uy+vy,lb=hypot(bx,by);bx/=lb;by/=lb;
    double dc=R/sin(ang/2);P ctr{cur.x+bx*dc,cur.y+by*dc};
    double a1=atan2(t1.y-ctr.y,t1.x-ctr.x),a2=atan2(t2.y-ctr.y,t2.x-ctr.x);
    double da=a2-a1;while(da>M_PI)da-=2*M_PI;while(da<-M_PI)da+=2*M_PI;
    for(int k=0;k<=8;k++){double a=a1+da*k/8;pts.push_back({ctr.x+R*cos(a),ctr.y+R*sin(a)});}
  }
  int m=pts.size();
  for(int i=0;i<m;i++){P p=pts[i],q=pts[(i+1)%m];double L=hypot(q.x-p.x,q.y-p.y);int k=std::max(1,(int)ceil(L/0.5));
    for(int j=0;j<k;j++)out.push_back({p.x+(q.x-p.x)*j/k,p.y+(q.y-p.y)*j/k});}
  return out;
}
struct Track{
  std::vector<P> pts;std::vector<double> s;double L;
  Track(double R){pts=dense(R);s.resize(pts.size());s[0]=0;for(size_t i=1;i<pts.size();i++)s[i]=s[i-1]+hypot(pts[i].x-pts[i-1].x,pts[i].y-pts[i-1].y);
    L=s.back()+hypot(pts[0].x-pts.back().x,pts[0].y-pts.back().y);}
  // rasterised tape (0.2 cm cells) for a given tape width
  static constexpr double X0=-40,Y0=-140,RES=0.2;int GW=0,GH=0;std::vector<unsigned char> grid;
  void raster(double w){GW=(int)(400/RES);GH=(int)(290/RES);grid.assign((size_t)GW*GH,0);double r=w/2;int rc=(int)ceil(r/RES)+1;
    size_t n=pts.size();
    for(size_t i=0;i<n;i++){const P&a=pts[i],&b=pts[(i+1)%n];int k=std::max(1,(int)ceil(hypot(b.x-a.x,b.y-a.y)/0.1));
      for(int j=0;j<k;j++){double px=a.x+(b.x-a.x)*j/k,py=a.y+(b.y-a.y)*j/k;int cx=(int)((px-X0)/RES),cy=(int)((py-Y0)/RES);
        for(int dy=-rc;dy<=rc;dy++)for(int dx=-rc;dx<=rc;dx++){int gx=cx+dx,gy=cy+dy;if(gx<0||gy<0||gx>=GW||gy>=GH)continue;
          double qx=X0+(gx+0.5)*RES-px,qy=Y0+(gy+0.5)*RES-py;if(qx*qx+qy*qy<=r*r)grid[(size_t)gy*GW+gx]=1;}}}}
  bool black(double x,double y)const{int gx=(int)((x-X0)/RES),gy=(int)((y-Y0)/RES);if(gx<0||gy<0||gx>=GW||gy>=GH)return false;return grid[(size_t)gy*GW+gx];}
  // nearest route index within a forward window of the current progress index
  size_t local(double x,double y,size_t idx,double &d)const{
    size_t n=pts.size(),best=idx;double bd=1e18;
    for(int k=-60;k<=120;k++){size_t i=(idx+n+k)%n;double dx=pts[i].x-x,dy=pts[i].y-y,dd=dx*dx+dy*dy;if(dd<bd){bd=dd;best=i;}}
    d=sqrt(bd);return best;}
};

struct Params{double d,w,F,kv,R;bool gyro;double asym,B,DB;int cruise;double noise=0,gscale=1.01;bool inverted=false;};
struct Result{bool pass;int laps;double maxDev;int fault;double t;int crosses,turns,searches;std::string why;};

bool TRACE=false;
Result run(const Params&p,unsigned seed,FILE*dump=nullptr){
  Track tr(p.R);tr.raster(p.w);std::mt19937 rng(seed);std::normal_distribution<double> gn(0,0.3);
  double x=15-p.F,y=0,th=0,vl=0,vr=0; // robot reference point = rotation centre
  LineFollower f;f.cruise=p.cruise;f.pivot=100;f.crossHold=400;f.maxTurn=200;f.wideSweep=170;f.degPerMs=0.12f;f.blockCm=15;
  double gyroYaw=0,bias=((int)(rng()%200)-100)/100.0*0.3; // deg/s residual bias
  double gsign=p.inverted?-1:1;int flips=0;
  uint32_t now=1000;f.start(0,now);
  size_t idx=0;double travelled=0;int laps=0;double maxDev=0;Result r{false,0,0,0,0,0,0,0,""};
  double distModel=0;uint32_t lastSample=0;uint16_t q=0;uint8_t mask=0;
  int L=0,Rr=0;
  for(int it=0;it<200000;it++){
    uint32_t dt= (it%50==0)?14:2; // ~12 ms ultrasonic stall every 100 ms
    now+=dt;double ds=dt*1e-3;
    // sensors
    double c=cos(th),s=sin(th);
    auto blk=[&](double lat){double sx=x+p.F*c-lat*s,sy=y+p.F*s+lat*c;return tr.black(sx,sy);};
    bool bl=blk(p.d),bm=blk(0),br=blk(-p.d);
    if(p.noise>0){std::uniform_real_distribution<double> u(0,1);if(u(rng)<p.noise)bl=!bl;if(u(rng)<p.noise)bm=!bm;if(u(rng)<p.noise)br=!br;}
    mask|=(bl?1:0)|(bm?2:0)|(br?4:0)|((bl&&bm&&br)?8:0);
    f.step(bl,bm,br,now,(float)(gsign*gyroYaw),p.gyro,-1,L,Rr);
    if(f.requestSignFlip){gsign=-gsign;f.negateAngles();f.requestSignFlip=false;flips++;}
    static int prevState=-1;
    if(TRACE&&(int)f.state!=prevState){printf("t=%6.2f %-6s pos=(%6.1f,%6.1f) th=%7.1f gyro=%7.1f ref=%7.1f s=%d%d%d idx=%zu\n",(now-1000)/1000.0,
      (const char*[]){"IDLE","FOLLOW","CROSS","TURN","SEARCH","FAIL"}[f.state],x,y,th/DEG,gyroYaw,f.refYaw,bl,bm,br,idx);prevState=f.state;}
    if(f.state==LineFollower::FAIL){r.fault=f.fault;r.why="FAIL";break;}
    // motors: deadband, asymmetry, 60 ms lag
    auto speed=[&](int pwm,double gain){double a=fabs(pwm);if(a<p.DB)return 0.0;return (pwm>0?1:-1)*p.kv*gain*(a-p.DB*0.6);};
    double tl=speed(L,1.0),trr=speed(Rr,p.asym);double k=ds/0.06;if(k>1)k=1;vl+=(tl-vl)*k;vr+=(trr-vr)*k;
    double v=(vl+vr)/2,w=(vr-vl)/p.B;
    x+=v*cos(th+w*ds/2)*ds;y+=v*sin(th+w*ds/2)*ds;th+=w*ds;
    gyroYaw+=(w/DEG*p.gscale+bias+gn(rng))*ds; // 1% scale error, bias, noise
    distModel+=(L+Rr)*0.5*0.33*ds;
    travelled+=fabs(v)*ds;
    // progress along the route (measured at the sensor row)
    double d;size_t ni=tr.local(x,y,idx,d);
    if(d>maxDev)maxDev=d;
    if(d>12){r.why="left route";break;}
    if(ni<idx&&idx-ni>tr.pts.size()/2){laps++;}
    idx=ni;
    if(dump&&now-lastSample>=100){lastSample=now;
      fprintf(dump,"{\"N\":230,\"d\":[%u,%u,%d,%d,%d,%d,%d,%d,%ld,%ld,%d,%d,%d,%u,%u]}\n",q++,now,bl?900:150,bm?900:150,br?900:150,L,Rr,(int)(gyroYaw*0+w/DEG*10),(long)(gyroYaw*10),(long)(distModel*10),0,0,-1,(unsigned)(0x60|f.state),(unsigned)(mask|f.events));
      mask=0;f.events=0;}
    if(laps>=2){r.pass=true;break;}
  }
  if(!r.pass&&r.why.empty())r.why="timeout";
  r.laps=laps;r.maxDev=maxDev;r.t=(now-1000)/1000.0;return r;
}

int main(int argc,char**argv){
  if(argc>2&&!strcmp(argv[1],"--trace"))TRACE=true;
  if(argc>2&&(!strcmp(argv[1],"--dump")||TRACE)){
    FILE*f=TRACE?nullptr:fopen(argv[2],"w");Params p{1.5,2.5,9,0.4,6,true,0.95,25,30,60};Result r=run(p,7,f);if(f)fclose(f);
    printf("dump run: %s laps=%d t=%.1fs maxDev=%.1fcm %s\n",r.pass?"PASS":"FAIL",r.laps,r.t,r.maxDev,r.why.c_str());return r.pass?0:1;}
  if(argc>=8&&!strcmp(argv[1],"--one")){TRACE=true;Params p{atof(argv[2]),atof(argv[3]),atof(argv[4]),atof(argv[5]),atof(argv[6]),atoi(argv[7])!=0,0.95,25,30,60};
    unsigned seed=(unsigned)(p.d*100+p.w*10+p.F+p.kv*1000+p.R+(p.gyro?1:0));Result r=run(p,seed);
    printf("%s laps=%d t=%.1f fault=%d dev=%.1f %s\n",r.pass?"PASS":"FAIL",r.laps,r.t,r.fault,r.maxDev,r.why.c_str());return 0;}
  if(argc>=2&&!strcmp(argv[1],"--inverted")){
    int ok=0,n=0;for(double d:{1.0,1.5,2.0})for(double w:{1.8,2.5,4.0})for(double F:{9.0,12.0,15.0})for(double kv:{0.25,0.4})for(double R:{0.0,8.0}){
      if(w<d)continue;Params p{d,w,F,kv,R,true,0.95,25,30,60};p.inverted=true;Result r=run(p,(unsigned)(d*100+w*10+F));n++;ok+=r.pass;
      if(!r.pass)printf("FAIL inverted d=%.1f w=%.1f F=%.0f kv=%.2f R=%.0f: %s fault=%d\n",d,w,F,kv,R,r.why.c_str(),r.fault);}
    printf("INVERTED GYRO SIGN (auto-corrected at first pivot): %d/%d passed 2 laps\n",ok,n);return 0;}
  if(argc>=2&&!strcmp(argv[1],"--stress")){
    std::mt19937 g(12345);std::uniform_real_distribution<double> U(0,1);int n=0,ok=0;
    for(int i=0;i<400;i++){
      Params p;p.d=1.0+U(g)*1.0;p.w=std::max(p.d*1.0,1.8)+U(g)*1.5;p.F=9+U(g)*6;p.kv=0.2+U(g)*0.3;p.R=U(g)<0.5?0:U(g)*10;p.gyro=true;
      p.asym=0.85+U(g)*0.15;if(U(g)<0.5)p.asym=1/p.asym;p.B=20+U(g)*15;p.DB=20+U(g)*20;p.cruise=50+(int)(U(g)*40);p.noise=U(g)*0.01;p.gscale=0.95+U(g)*0.1;
      if(argc>=3&&atoi(argv[2])!=i)continue;
      TRACE=argc>=3;Result r=run(p,1000+i);n++;ok+=r.pass;
      if(!r.pass)printf("#%d FAIL d=%.2f w=%.2f F=%.1f kv=%.2f R=%.1f asym=%.2f B=%.0f DB=%.0f cruise=%d noise=%.3f gs=%.2f: %s laps=%d t=%.0f fault=%d\n",i,p.d,p.w,p.F,p.kv,p.R,p.asym,p.B,p.DB,p.cruise,p.noise,p.gscale,r.why.c_str(),r.laps,r.t,r.fault);}
    printf("STRESS (gyro on, tape >= sensor spacing): %d/%d passed 2 laps\n",ok,n);return 0;}
  int total=0,pass=0,passGyro=0,totGyro=0;
  for(double d:{1.0,1.5,2.0})for(double w:{1.8,2.5,4.0})for(double F:{9.0,12.0,15.0})for(double kv:{0.25,0.4})for(double R:{0.0,8.0})for(int g=0;g<2;g++){
    if(w<d*1.0&&false)continue;
    Params p{d,w,F,kv,R,g==1,0.95,25,30,60};
    Result r=run(p,(unsigned)(d*100+w*10+F+kv*1000+R+g));
    total++;pass+=r.pass;if(g){totGyro++;passGyro+=r.pass;}
    if(!r.pass)printf("FAIL d=%.1f w=%.1f F=%.0f kv=%.2f R=%.0f gyro=%d: %s laps=%d t=%.1f fault=%d dev=%.1f\n",d,w,F,kv,R,g,r.why.c_str(),r.laps,r.t,r.fault,r.maxDev);
  }
  printf("SUMMARY gyro ON: %d/%d passed 2 laps; gyro OFF: %d/%d\n",passGyro,totGyro,pass-passGyro,total-totGyro);
  return 0;
}
