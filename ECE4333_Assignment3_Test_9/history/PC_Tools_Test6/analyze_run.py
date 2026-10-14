#!/usr/bin/env python3
"""Read A3 V30 pose CSV; output estimated trajectory graph and run summary.
Predicted distance/speed are model estimates. Do NOT label them actual travel.
Usage: python analyze_run.py run01_pose.csv
Optional for graphs: python -m pip install matplotlib
"""
import csv, math, sys, pathlib, statistics

def analyze(filename):
 p=pathlib.Path(filename)
 with p.open(newline='',encoding='utf-8-sig') as f:
  rows=[r for r in csv.DictReader(f) if r.get('pred_x_cm') not in ('',None) and r.get('pred_y_cm') not in ('',None)]
 if len(rows)<2:raise ValueError('Need at least two valid pose readings. Verify UNO telemetry and Wi-Fi.')
 pts=[]
 for r in rows:
  try:
   t=float(r['run_t_s']); x=float(r['pred_x_cm']);y=float(r['pred_y_cm']);h=float(r['pred_heading_deg'])
   if math.isfinite(t+x+y+h):pts.append((t,x,y,h,r))
  except (ValueError,KeyError,TypeError):pass
 if len(pts)<2:raise ValueError('Need at least two moving-run samples (run_t_s is empty outside run mode).')
 pts.sort(key=lambda z:z[0]); dist=sum(math.hypot(b[1]-a[1],b[2]-a[2]) for a,b in zip(pts,pts[1:]));duration=pts[-1][0]-pts[0][0]
 changes=[];prev=None
 for t,x,y,h,r in pts:
  state=(r.get('traffic_state'),r.get('estop_latched'),r.get('fault_code'))
  if state!=prev:changes.append((t,*state));prev=state
 summary=(f'Robot run analysis (from predicted kinematic poses; NOT measured ground truth)\n'
          f'Samples: {len(pts)}\nLogged time span: {duration:.2f} s\n'
          f'Estimated path length: {dist:.2f} cm\n'
          f'Estimated average speed: {dist/duration if duration>0 else 0:.2f} cm/s\n'
          f'End predicted pose: X={pts[-1][1]:.1f} cm, Y={pts[-1][2]:.1f} cm, heading={pts[-1][3]:.1f} deg\n'
          f'Observed state/safety transitions: {len(changes)}\n'
          f'IMPORTANT: This is NOT verified Figure-8 completion time; confirm with video/real checkpoints.\n')
 dest=p.with_name(p.stem.replace('_pose','')+'_summary.txt');dest.write_text(summary,encoding='utf-8');print(summary);print('Saved',dest)
 try:import matplotlib.pyplot as plt
 except ImportError:print('Install matplotlib if you want PNG plots.');return
 xs=[v[1] for v in pts];ys=[v[2] for v in pts];ts=[v[0] for v in pts];hs=[v[3] for v in pts]
 plt.figure();plt.plot(xs,ys,'-',label='Kinematic prediction');plt.plot(xs[0],ys[0],'o',label='Start');plt.axis('equal');plt.xlabel('Predicted X (cm)');plt.ylabel('Predicted Y (cm)');plt.legend();plt.title('Predicted trajectory (NOT actual track)');plt.grid(True)
 plot=p.with_name(p.stem.replace('_pose','')+'_trajectory.png');plt.savefig(plot,dpi=150,bbox_inches='tight');plt.close();print('Saved',plot)
 plt.figure();plt.plot(ts,hs,label='Kinematic heading');plt.xlabel('Robot time (s)');plt.ylabel('Heading (deg)');plt.grid(True);plt.legend()
 plot=p.with_name(p.stem.replace('_pose','')+'_heading.png');plt.savefig(plot,dpi=150,bbox_inches='tight');plt.close();print('Saved',plot)

if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('Usage: python analyze_run.py run01_pose.csv')
 analyze(sys.argv[1])
