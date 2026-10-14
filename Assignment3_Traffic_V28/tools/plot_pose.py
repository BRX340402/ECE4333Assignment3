#!/usr/bin/env python3
"""Plot a pose log (tools/log_pose.ps1 CSV): predicted vs fused vs reference figure-8.
usage: python plot_pose.py run1.csv [--rotate 150] [--actual actual_points.csv] [--png out.png]
 --rotate 150 : rotate the view so the track looks like the assignment picture
 --actual     : optional hand-measured ground truth, CSV with columns x_cm,y_cm[,label]
"""
import csv, math, sys, argparse
import matplotlib
matplotlib.use("Agg") if "--show" not in sys.argv else None
import matplotlib.pyplot as plt
# same table as UNO_Assignment2/Track.h
TURN10=[0,-1366,-1034,300,900,1200]; MM=[600,1513,2200,433,1250,3200]
def reference(laps=1):
    x=y=h=0.0; pts=[(0.0,0.0)]; ul=None
    for lap in range(laps):
        segs=range(0,6) if lap==0 else range(1,6)
        for i in segs:
            if lap>0 and i==1: h=0.0
            h+=TURN10[i]/10.0
            x+=MM[i]/10.0*math.cos(math.radians(h)); y+=MM[i]/10.0*math.sin(math.radians(h)); pts.append((x,y))
    return pts
def rot(p,deg):
    a=math.radians(deg); return (math.cos(a)*p[0]-math.sin(a)*p[1], math.sin(a)*p[0]+math.cos(a)*p[1])
ap=argparse.ArgumentParser(); ap.add_argument("csv"); ap.add_argument("--rotate",type=float,default=0)
ap.add_argument("--actual"); ap.add_argument("--png"); ap.add_argument("--show",action="store_true")
a=ap.parse_args()
rows=list(csv.DictReader(open(a.csv)))
f=lambda k:[float(r[k]) for r in rows]
t=f("t_s"); fx,fy=f("fused_x_cm"),f("fused_y_cm"); px,py=f("pred_x_cm"),f("pred_y_cm")
fh,ph,gh=f("fused_heading_deg"),f("pred_heading_deg"),f("gyro_heading_wrapped_deg")
ref=reference(2); R=lambda xs,ys:list(zip(*[rot(p,a.rotate) for p in zip(xs,ys)])) if a.rotate else (xs,ys)
fig,ax=plt.subplots(1,2,figsize=(14,6))
rx,ry=R([p[0] for p in ref],[p[1] for p in ref]); ax[0].plot(rx,ry,'k--',lw=1,label="reference track (assignment figure)")
X,Y=R(px,py); ax[0].plot(X,Y,color='tab:red',lw=1.2,label="predicted (kinematic model only)")
X,Y=R(fx,fy); ax[0].plot(X,Y,color='tab:orange',lw=2,label="fused (model + gyro + corner events)")
ax[0].plot(*R([0],[0]),'go',label="start / traffic light")
if a.actual:
    ar=list(csv.DictReader(open(a.actual))); X,Y=R([float(r['x_cm']) for r in ar],[float(r['y_cm']) for r in ar]); ax[0].plot(X,Y,'bs-',label="measured actual")
ax[0].set_aspect('equal'); ax[0].grid(True); ax[0].set_xlabel("X (cm)"); ax[0].set_ylabel("Y (cm)"); ax[0].legend(fontsize=8); ax[0].set_title("Figure-8 position in the inertial frame")
ax[1].plot(t,gh,color='tab:orange',label="gyro heading"); ax[1].plot(t,ph,color='tab:red',label="predicted heading"); ax[1].plot(t,fh,'--',color='tab:green',label="fused heading")
ax[1].set_xlabel("time (s)"); ax[1].set_ylabel("heading (deg)"); ax[1].grid(True); ax[1].legend(); ax[1].set_title("Gyro vs predicted heading")
plt.tight_layout(); out=a.png or a.csv.rsplit('.',1)[0]+".png"; plt.savefig(out,dpi=110); print("saved",out)
# numbers for the report
def pathlen(xs,ys): return sum(math.hypot(xs[i+1]-xs[i],ys[i+1]-ys[i]) for i in range(len(xs)-1))
print("samples %d, duration %.1f s"%(len(t),t[-1]-t[0]))
print("end point  fused (%.1f, %.1f) cm   predicted (%.1f, %.1f) cm   (a perfect run ends back at the start (0,0))"%(fx[-1],fy[-1],px[-1],py[-1]))
print("path length fused %.0f cm, predicted %.0f cm (reference 2 laps %.0f cm)"%(pathlen(fx,fy),pathlen(px,py),pathlen([p[0] for p in ref],[p[1] for p in ref])))
print("final heading error  fused %.1f deg, predicted %.1f deg (gyro %.1f)"%(fh[-1],ph[-1],gh[-1]))
print("learned speed scale (multiply LEFT/RIGHT_CM_S_PER_PWM by this): %.3f"%float(rows[-1]['speed_scale']))
