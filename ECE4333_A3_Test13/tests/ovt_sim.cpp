// Host simulation of the Test13 overtaking state machine (Overtake.h) + line follower on the figure-8 track.
// An obstacle (static, or a slower robot driving the same way) sits ON the line; the car must detour on the outside,
// find the line again and complete the lap, without touching it.  g++ -O2 -std=c++14 -I../UNO_Mapping ovt_sim.cpp -o ovt && ./ovt
// (derived from sim_track.cpp: same track, tape, motor and gyro models)
// Original header: Host simulation of the Test10 line follower on the figure-8 track drawing.
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
#include "Overtake.h"

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

struct Params{double d,w,F,kv,R;bool gyro;double asym,B,DB;int cruise;double noise=0,gscale=1.01;bool inverted=false;double odo=1.10;};
struct Result{bool pass;int laps;double maxDev;int fault;double t;int crosses,turns,searches;std::string why;};
struct Obs{double s0=0,vo=0,r=8;bool on=false;int ovtOk=0,ovtFail=0;int par=45;bool hit=false,estop=false;double ovtTime=0;int side=0,outside=0;double minGap=1e9;} OBS;

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
  double kx=0,ky=0,kh=0,fx=0,fy=0; // UNO model: predicted (wheel model only) and fused (gyro heading + wheel speed)
  int L=0,Rr=0;
  Overtake ov;ov.enabled=OBS.on;ov.pd.kp10=15;ov.pd.kd100=10;int rangeCm=-1;double obsS=OBS.s0,ovtT0=-1;OBS.hit=false;OBS.estop=false;OBS.ovtTime=0;ov.parCm=OBS.par;ov.joinAdv=(int)p.F;ov.outCm=OBS.r>10?32:26;OBS.minGap=1e9;OBS.side=0;OBS.outside=0;bool wasActive=false;
  for(int it=0;it<400000;it++){
    uint32_t dt= (it%50==0)?14:2; // ~12 ms ultrasonic stall every 100 ms
    now+=dt;double ds=dt*1e-3;
    // sensors
    double c=cos(th),s=sin(th);
    auto blk=[&](double lat){double sx=x+p.F*c-lat*s,sy=y+p.F*s+lat*c;return tr.black(sx,sy);};
    bool bl=blk(p.d),bm=blk(0),br=blk(-p.d);
    if(p.noise>0){std::uniform_real_distribution<double> u(0,1);if(u(rng)<p.noise)bl=!bl;if(u(rng)<p.noise)bm=!bm;if(u(rng)<p.noise)br=!br;}
    mask|=(bl?1:0)|(bm?2:0)|(br?4:0)|((bl&&bm&&br)?8:0);
    // UNO odometer (PWM x speed factor). The factor is calibrated on the real car (Test10 run 3:
    // 859 vs 859.6 cm per lap); here it is the true distance with a deliberate error (default +10 %).
    // obstacle on the line: centre from arc length (static or moving along the track), ultrasonic cone +-10 deg, 12 ms stall as before
    double ox=1e9,oy=1e9;
    if(OBS.on){obsS+=OBS.vo*ds;double sm=fmod(obsS,tr.L);size_t lo=0,hi=tr.s.size()-1;while(lo<hi){size_t mid=(lo+hi)/2;if(tr.s[mid]<sm)lo=mid+1;else hi=mid;}ox=tr.pts[lo].x;oy=tr.pts[lo].y;
     double gap=hypot(ox-x,oy-y)-OBS.r;if(gap<OBS.minGap)OBS.minGap=gap;if(hypot(ox-x,oy-y)<OBS.r+9)OBS.hit=true;}
    if(it%50==0){rangeCm=-1;if(OBS.on){double ux=x+(p.F+2)*c,uy=y+(p.F+2)*s;double best=1e9;
      for(int k=-1;k<=1;k++){double a=th+k*10*DEG,dx=cos(a),dy=sin(a),fx=ox-ux,fy=oy-uy,tproj=fx*dx+fy*dy;if(tproj<0)continue;double perp=fabs(fx*dy-fy*dx);if(perp<=OBS.r){double d=tproj-sqrt(OBS.r*OBS.r-perp*perp);if(d<best)best=d;}}
      if(best<150)rangeCm=(int)(best<0?0:best);}}
    f.distCm=(float)(travelled*p.odo);
    bool following=f.settled(now);
    float wDeg=(float)(((vr-vl)/p.B)/DEG*p.gscale+gn(rng)*3);
    bool owns=ov.step((float)(gsign*gyroYaw),wDeg*(float)gsign,f.distCm,rangeCm,f.state==LineFollower::FOLLOW,following,f.refYaw,f.outside(),f.freeAhead(),f.toCorner(),bl,bm,br,now,L,Rr);
    if(ov.active()&&!wasActive){ovtT0=(now-1000)/1000.0;OBS.side=ov.side;OBS.outside=f.outside();}
    if(!ov.active()&&wasActive&&ovtT0>=0){OBS.ovtTime+=(now-1000)/1000.0-ovtT0;ovtT0=-1;}
    {static int ps=-1;if(getenv("OVT_TRACE")&&(int)ov.state!=ps){const char*nm[]={"IDLE","WAIT","OUT_TURN","OUT_DRIVE","ALIGN","PASS","RET_TURN","RET_DRIVE","JOIN","FAIL"};printf("t=%6.2f OVT %-9s pos=(%6.1f,%6.1f) th=%7.1f range=%d f=%d s=%d%d%d seg=%d prog=%.0f\n",(now-1000)/1000.0,nm[ov.state],x,y,th/DEG,rangeCm,(int)f.state,bl,bm,br,f.seg,f.segProg());ps=ov.state;}}
    wasActive=ov.active();
    if(!owns)f.step(bl,bm,br,now,(float)(gsign*gyroYaw),p.gyro,-1,L,Rr);else f.lastMs=now;
    if(ov.handover){f.resume((float)(gsign*gyroYaw),ov.H,now);f.segStart+=ov.lostCm;ov.handover=false;}
    {static int nc=0;nc=(rangeCm>=0&&rangeCm<=12&&it%50==0)?nc+1:((rangeCm<0||rangeCm>12)?0:nc);if(L>0&&Rr>0&&nc>=3){OBS.estop=true;r.why="E-STOP";break;}}
    if(ov.state==Overtake::FAIL){r.fault=15;r.why="OVERTAKE FAIL";break;}
    if(OBS.hit){r.why="COLLISION";break;}
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
    {double vlm=L*0.33,vrm=Rr*0.33,vm=(vlm+vrm)/2,wk=(vrm-vlm)/14.0,hf=gsign*gyroYaw*DEG; // same as PoseEstimate.h
     kx+=vm*cos(kh+wk*ds/2)*ds;ky+=vm*sin(kh+wk*ds/2)*ds;kh+=wk*ds;fx+=vm*cos(hf)*ds;fy+=vm*sin(hf)*ds;}
    travelled+=fabs(v)*ds;
    // progress along the route (measured at the sensor row)
    double d;size_t ni=tr.local(x,y,idx,d);
    if(d>maxDev)maxDev=d;
    if(d>12&&!ov.active()&&ov.state==Overtake::IDLE&&OBS.ovtOk==0&&false){r.why="left route";break;}if(it%50==0){double dm=1e9;for(auto&q:tr.pts){double e=hypot(q.x-x,q.y-y);if(e<dm)dm=e;}if(dm>45){char b[110];snprintf(b,110,"left route at (%.0f,%.0f) th=%.0f d=%.0f ovt=%d",x,y,th/DEG,dm,(int)ov.state);r.why=b;break;}}
    if(ni<idx&&idx-ni>tr.pts.size()/2){laps++;}
    idx=ni;
    if(dump&&now-lastSample>=100){lastSample=now;
      double khw=fmod(kh/DEG+180.0,360.0);if(khw<0)khw+=360.0;khw-=180.0;
      fprintf(dump,"{\"N\":230,\"d\":[%u,%u,%d,%d,%d,%d,%d,%d,%ld,%ld,%d,%d,%d,%u,%u,%d,%d,%d]}\n",q++,now,bl?900:150,bm?900:150,br?900:150,L,Rr,(int)(gyroYaw*0+w/DEG*10),(long)(gyroYaw*10),(long)(distModel*10),(int)fx,(int)fy,-1,(unsigned)(0x68|f.state),(unsigned)(mask|f.events),(int)(khw*10),(int)kx,(int)ky);
      mask=0;f.events=0;}
    if(laps>=(OBS.on?2:2)){r.pass=true;break;}
  }
  if(!r.pass&&r.why.empty())r.why="timeout";
  OBS.ovtOk=ov.ok;OBS.ovtFail=ov.fails;
  r.laps=laps;r.maxDev=maxDev;r.t=(now-1000)/1000.0;return r;
}

int main(int argc,char**argv){
  // obstacle positions (arc length from C along the route, cm): spread over every straight; r = obstacle radius
  struct Case{const char*name;double s;double vo;double r;bool room;int par;};
  // room = the straight leaves >= need() (about 125 cm) between the detection point and the next corner / intersection window.
  // Cases without room must end SAFELY (wait behind it or E-stop), never in contact. Obstacles standing within ~35 cm after a corner
  // or inside the intersection are outside the supported range (see README) and are not simulated.
  std::vector<Case> cases={{"static, C-O (65 cm)",65,0,8,true,45},{"static, O-D (222 cm)",222,0,8,true,45},{"static, O-D (232 cm)",232,0,8,true,45},
    {"static big r=12, O-D (225 cm)",225,0,12,true,52},{"slow robot 5 cm/s, from 215 cm",215,5,9,true,100},
    {"no room: static, C-O (110 cm)",110,0,8,false,45},{"no room: static, O-D (280 cm)",280,0,8,false,45},{"no room: static, D-E (400 cm)",400,0,8,false,45},{"no room: static, B-C (790 cm)",790,0,8,false,45}};
  int total=0,ok=0,okFixed=0,fixedN=0,roomN=0,roomOk=0,nrN=0,nrOk=0,touch=0;bool verbose=argc>1&&!strcmp(argv[1],"-v");
  if(argc>3&&!strcmp(argv[1],"--one")){OBS=Obs();OBS.on=true;OBS.s0=atof(argv[2]);OBS.vo=atof(argv[3]);OBS.r=argc>4?atof(argv[4]):8;OBS.par=argc>5?atoi(argv[5]):55;std::vector<Params> pp={Params{1.5,2.5,9,0.4,6,true,0.95,25,30,60},Params{1.5,2.5,12,0.33,0,true,1.0,25,30,60},Params{1.0,1.8,15,0.3,8,true,0.9,30,35,60},Params{2.0,2.5,9,0.4,0,true,1.05,25,25,70}};Params p=pp[getenv("PIDX")?atoi(getenv("PIDX")):0];setenv("OVT_TRACE","1",1);Result r=run(p,77);printf("%s %s ovtOk=%d fail=%d hit=%d\n",r.pass?"PASS":"FAIL",r.why.c_str(),OBS.ovtOk,OBS.ovtFail,(int)OBS.hit);return 0;}
  std::vector<Params> ps;
  ps.push_back(Params{1.5,2.5,9,0.4,6,true,0.95,25,30,60});
  ps.push_back(Params{1.5,2.5,12,0.33,0,true,1.0,25,30,60});
  ps.push_back(Params{1.0,1.8,15,0.3,8,true,0.9,30,35,60});
  ps.push_back(Params{2.0,2.5,9,0.4,0,true,1.05,25,25,70});
  for(size_t pi=0;pi<ps.size();pi++)for(auto&cs:cases){
    OBS=Obs();OBS.on=true;OBS.s0=cs.s;OBS.vo=cs.vo;OBS.r=cs.r;OBS.par=cs.par;
    for(int seed=0;seed<4;seed++){ Result r=run(ps[pi],(unsigned)(77+pi*13+cs.s+seed*1013));total++;
    bool room=cs.room;bool good=room?(r.pass&&OBS.ovtOk>=1&&!OBS.hit&&!OBS.estop&&OBS.side==OBS.outside&&OBS.outside!=0):(!OBS.hit&&OBS.ovtFail==0); // no room: must wait behind it, never touch it
    if(cs.vo==0&&cs.r<=8&&cs.room)fixedN++,okFixed+=good;ok+=good;if(cs.room){roomN++;roomOk+=good;}else{nrN++;nrOk+=good;}if(OBS.hit)touch++;
    if(!good||verbose)printf("%s params#%zu %-34s : %s ovtOk=%d fail=%d side=%d outside=%d time=%.1fs minGap=%.1fcm laps=%d %s\n",good?"PASS":"FAIL",pi,cs.name,r.pass?"lap done":"no lap",OBS.ovtOk,OBS.ovtFail,OBS.side,OBS.outside,OBS.ovtTime,OBS.minGap,r.laps,r.why.c_str());
  }}
  printf("OVERTAKING SIMULATION: with room: %d/%d overtaken on the outside and lap completed (static r<=8 cm: %d/%d); without room: %d/%d waited / stopped safely; contacts with the obstacle: %d of %d runs\n",roomOk,roomN,okFixed,fixedN,nrOk,nrN,touch,total);
  // regression: no obstacle -> the same behaviour as the plain follower
  OBS=Obs();Result r0=run(ps[0],5);printf("no obstacle: %s laps=%d\n",r0.pass?"PASS":"FAIL",r0.laps);
  return 0;
}
