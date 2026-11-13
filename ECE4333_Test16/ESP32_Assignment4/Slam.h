#pragma once
#include <math.h>
#include <string.h>
// Test16 EKF-SLAM (Assignment 5, task 6). Runs on the ESP32 with the UNO's 10 Hz samples (N230) and the camera.
//
// State      x = [xr, yr, th, l1x, l1y, ..., lnx, lny]   (cm, cm, rad, then up to NL landmarks in the inertial frame)
//            inertial frame: origin = robot position at the start, x axis = start heading, angles + counter-clockwise.
// Controls   u = [d, dth]: travelled distance (wheel model x odometer scale) and heading change from the GYRO.
// Motion     xr += d cos(th + dth/2), yr += d sin(th + dth/2), th += dth
//            F = d f / d xr = [1 0 -d sin(m); 0 1 d cos(m); 0 0 1],  G = d f / d u = [cos(m) -d sin(m)/2; sin(m) d cos(m)/2; 0 1]
//            P_rr = F P_rr F' + G M G',  P_rl = F P_rl,   M = diag((a_d |d| + 0.1)^2, (a_g |dth| + g_n)^2)
// Measure    z = [r, b] of one landmark (camera: range from the blob width, bearing from its image position)
//            h = [sqrt(q), atan2(dy, dx) - th],  dx = lx - xr, dy = ly - yr, q = dx^2 + dy^2
//            H (2 x 5 non-zero) = [-dx/r -dy/r 0 dx/r dy/r ; dy/q -dx/q -1 -dy/q dx/q]
//            R = diag((0.1 r + 3 cm)^2, (3 deg)^2)
//            S = H P H' + R, K = P H' S^-1, x += K (z - h), P = (I - K H) P
// Association: same colour class, smallest Mahalanobis distance; < 9.21 (chi^2, 2 dof, 99 %) = update,
//            > 25 = new landmark (initialised from the pose and z with the full covariance), otherwise ignored.
struct SlamEKF{
 static const int NL=10,N=3+2*NL;
 float x[N],P[N][N];uint8_t cls[NL];int n=0;           // n landmarks in use
 float aD=0.05f,aG=0.03f,gN=0.002f;                   // motion noise: 5 % of distance, 3 % of turn + 0.002 rad per step
 float sR0=3.0f,sRk=0.10f,sB=0.0524f;                 // measurement noise: 3 cm + 10 % range, 3 deg bearing
 int lastLm=-1;bool lastNew=false;uint16_t updates=0,inits=0,ignored=0;
 void reset(){memset(x,0,sizeof(x));memset(P,0,sizeof(P));n=0;lastLm=-1;updates=inits=ignored=0;P[2][2]=1e-4f;}
 static float wrap(float a){while(a>M_PI)a-=2*M_PI;while(a<-M_PI)a+=2*M_PI;return a;}
 int dim()const{return 3+2*n;}
 void predict(float d,float dth){
  float m=x[2]+dth*0.5f,c=cosf(m),s=sinf(m);
  x[0]+=d*c;x[1]+=d*s;x[2]=wrap(x[2]+dth);
  float F[3][3]={{1,0,-d*s},{0,1,d*c},{0,0,1}},G[3][2]={{c,-d*s*0.5f},{s,d*c*0.5f},{0,1}};
  float sd=aD*fabsf(d)+0.1f,sg=aG*fabsf(dth)+gN,M0=sd*sd,M1=sg*sg;
  int D=dim();float T[3][N];
  for(int i=0;i<3;i++)for(int j=0;j<D;j++){float v=0;for(int k=0;k<3;k++)v+=F[i][k]*P[k][j];T[i][j]=v;}   // T = F P(rows 0..2)
  for(int i=0;i<3;i++)for(int j=0;j<3;j++){float v=0;for(int k=0;k<3;k++)v+=T[i][k]*F[j][k];P[i][j]=v+G[i][0]*G[j][0]*M0+G[i][1]*G[j][1]*M1;}
  for(int i=0;i<3;i++)for(int j=3;j<D;j++){P[i][j]=T[i][j];P[j][i]=T[i][j];}
 }
 // expected measurement and Jacobian (columns 0..2 robot, 3..4 landmark) for landmark l
 void expect(int l,float &r,float &b,float H[2][5])const{
  float dx=x[3+2*l]-x[0],dy=x[4+2*l]-x[1],q=dx*dx+dy*dy;if(q<1)q=1;r=sqrtf(q);b=wrap(atan2f(dy,dx)-x[2]);
  float h0[5]={-dx/r,-dy/r,0,dx/r,dy/r},h1[5]={dy/q,-dx/q,-1,-dy/q,dx/q};memcpy(H[0],h0,sizeof(h0));memcpy(H[1],h1,sizeof(h1));
 }
 // S = H P H' + R for landmark l (only the 5 involved state indices)
 void innov(int l,float H[2][5],float r,float S[2][2])const{
  int id[5]={0,1,2,3+2*l,4+2*l};
  for(int a=0;a<2;a++)for(int b=0;b<2;b++){float v=0;for(int i=0;i<5;i++)for(int j=0;j<5;j++)v+=H[a][i]*P[id[i]][id[j]]*H[b][j];S[a][b]=v;}
  float sr=sR0+sRk*r;S[0][0]+=sr*sr;S[1][1]+=sB*sB;
 }
 // one camera observation: range r (cm), bearing b (rad, + left), colour class c. Returns landmark id or -1.
 int observe(float r,float b,uint8_t c){
  lastNew=false;int best=-1;float bestD=1e9,zr=0,zb=0;
  for(int l=0;l<n;l++){if(cls[l]!=c)continue;
   float er,eb,H[2][5],S[2][2];expect(l,er,eb,H);innov(l,H,r,S);
   float v0=r-er,v1=wrap(b-eb),det=S[0][0]*S[1][1]-S[0][1]*S[1][0];if(det<=0)continue;
   float d2=(v0*v0*S[1][1]-2*v0*v1*S[0][1]+v1*v1*S[0][0])/det;
   if(d2<bestD){bestD=d2;best=l;}}
  if(best>=0&&bestD<9.21f){update(best,r,b);lastLm=best;updates++;return best;}
  if((best<0||bestD>25.0f)&&n<NL){add(r,b,c);lastLm=n-1;lastNew=true;inits++;return n-1;}
  ignored++;return -1;
 }
 void update(int l,float r,float b){
  float er,eb,H[2][5],S[2][2];expect(l,er,eb,H);innov(l,H,r,S);
  int D=dim(),id[5]={0,1,2,3+2*l,4+2*l};float det=S[0][0]*S[1][1]-S[0][1]*S[1][0];
  float Si[2][2]={{S[1][1]/det,-S[0][1]/det},{-S[1][0]/det,S[0][0]/det}},v[2]={r-er,wrap(b-eb)};
  static float PHt[N][2],K[N][2];
  for(int i=0;i<D;i++)for(int a=0;a<2;a++){float s=0;for(int j=0;j<5;j++)s+=P[i][id[j]]*H[a][j];PHt[i][a]=s;}
  for(int i=0;i<D;i++)for(int a=0;a<2;a++)K[i][a]=PHt[i][0]*Si[0][a]+PHt[i][1]*Si[1][a];
  for(int i=0;i<D;i++)x[i]+=K[i][0]*v[0]+K[i][1]*v[1];
  x[2]=wrap(x[2]);
  // P = P - K (H P) with (H P)[a][j] = PHt[j][a] (P symmetric)
  for(int i=0;i<D;i++)for(int j=0;j<D;j++)P[i][j]-=K[i][0]*PHt[j][0]+K[i][1]*PHt[j][1];
  for(int i=0;i<D;i++)for(int j=i+1;j<D;j++){float m=0.5f*(P[i][j]+P[j][i]);P[i][j]=P[j][i]=m;} // keep it symmetric
 }
 void add(float r,float b,uint8_t c){
  int l=n,ix=3+2*l;float a=x[2]+b,ca=cosf(a),sa=sinf(a);
  x[ix]=x[0]+r*ca;x[ix+1]=x[1]+r*sa;cls[l]=c;
  // Gr = d l / d robot = [1 0 -r sa; 0 1 r ca], Gz = d l / d z = [ca -r sa; sa r ca]
  float Gr[2][3]={{1,0,-r*sa},{0,1,r*ca}},Gz[2][2]={{ca,-r*sa},{sa,r*ca}},sr=sR0+sRk*r,Rz[2]={sr*sr,sB*sB};
  int D=dim();
  for(int i=0;i<2;i++)for(int j=0;j<D;j++){float v=0;for(int k=0;k<3;k++)v+=Gr[i][k]*P[k][j];P[ix+i][j]=v;P[j][ix+i]=v;} // cross terms
  for(int i=0;i<2;i++)for(int j=0;j<2;j++){float v=0;for(int k=0;k<3;k++)for(int m=0;m<3;m++)v+=Gr[i][k]*P[k][m]*Gr[j][m];
   v+=Gz[i][0]*Gz[j][0]*Rz[0]+Gz[i][1]*Gz[j][1]*Rz[1];P[ix+i][ix+j]=v;}
  n++;
 }
};
