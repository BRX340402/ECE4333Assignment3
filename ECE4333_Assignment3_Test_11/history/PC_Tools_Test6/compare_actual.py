#!/usr/bin/env python3
"""Compare kinematic predictions against independent measured ground truth.
Actual CSV fields: t_s,actual_x_cm,actual_y_cm,actual_heading_deg
Time is measured from robot START, not the time Python was launched.
Example: python compare_actual.py run01_pose.csv actual_points.csv
"""
import argparse,csv,math,statistics
from pathlib import Path

def angle_diff_deg(a,b):return (a-b+180)%360-180

def compare(predfile, actualfile, outfile):
    with open(predfile,encoding='utf-8-sig',newline='') as f:
        predicted=[p for p in csv.DictReader(f) if p.get('run_t_s') not in ('',None) and p.get('pred_x_cm') not in ('',None)]
    if not predicted:raise ValueError('No predicted samples: record an autonomous run and confirm /traffic-status')
    pred=[(float(p['run_t_s']),p) for p in predicted]
    pred.sort(key=lambda q:q[0]); errors=[]
    with open(actualfile,encoding='utf-8-sig',newline='') as f:
        for measured in csv.DictReader(f):
            t=float(measured['t_s']);tn,near=min(pred,key=lambda q:abs(q[0]-t))
            if abs(tn-t)>1.0:
                print(f'WARNING: no prediction within 1 s of actual t={t} s; skipping');continue
            px=float(near['pred_x_cm']);py=float(near['pred_y_cm']);ph=float(near['pred_heading_deg'])
            ax=float(measured['actual_x_cm']);ay=float(measured['actual_y_cm']);ah=float(measured['actual_heading_deg'])
            ep=math.hypot(px-ax,py-ay);eh=angle_diff_deg(ph,ah)
            errors.append(dict(t_s=t,pred_t_s=tn,pred_x_cm=px,pred_y_cm=py,pred_heading_deg=ph,actual_x_cm=ax,actual_y_cm=ay,actual_heading_deg=ah,position_error_cm=round(ep,3),heading_error_deg=round(eh,3)))
    if not errors:raise ValueError('No paired samples: match t_s range to run_t_s in pose log')
    with open(outfile,'w',encoding='utf-8-sig',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(errors[0]));w.writeheader();w.writerows(errors)
    pos=[row['position_error_cm'] for row in errors];heading=[row['heading_error_deg'] for row in errors]
    print('Pairs:',len(errors),'Mean position error (cm):',round(statistics.mean(pos),2),'Position RMSE (cm):',round(math.sqrt(statistics.mean(x*x for x in pos)),2),'Heading MAE (deg):',round(statistics.mean(abs(x) for x in heading),2))
    print('Saved:',outfile)
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print('Optional: pip install matplotlib for trajectory plot')
        return errors
    plt.figure()
    plt.plot([float(p['pred_x_cm']) for _,p in pred],[float(p['pred_y_cm']) for _,p in pred],label='Predicted trajectory')
    plt.plot([x['actual_x_cm'] for x in errors],[x['actual_y_cm'] for x in errors],'o-',label='Actual measured checkpoints')
    plt.axis('equal');plt.xlabel('X (cm)');plt.ylabel('Y (cm)');plt.legend();plt.title('ECE 4333 A3: Differential-drive prediction vs measured track')
    png=str(Path(outfile).with_suffix('.png'));plt.savefig(png,dpi=170,bbox_inches='tight');plt.close()
    print('Plot:',png);return errors

def main():
    ap=argparse.ArgumentParser();ap.add_argument('predicted_csv');ap.add_argument('actual_csv');ap.add_argument('--out',default='comparison.csv');a=ap.parse_args();compare(a.predicted_csv,a.actual_csv,a.out)
if __name__=='__main__':main()
