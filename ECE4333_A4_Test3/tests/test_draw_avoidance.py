import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1] / "PC_Mapping"))
from draw_avoidance import render


class DrawAvoidanceTests(unittest.TestCase):
    def test_svg_and_obstacle_point(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            with open(root/"samples.csv","w",newline="") as f:
                w=csv.DictWriter(f,fieldnames=["mode","x_cm","y_cm","t_ms"])
                w.writeheader()
                w.writerow(dict(mode=6,x_cm=0,y_cm=0,t_ms=0))
                w.writerow(dict(mode=6,x_cm=10,y_cm=0,t_ms=150))
                w.writerow(dict(mode=6,x_cm=15,y_cm=10,t_ms=250))
            with open(root/"avoidance_events.csv","w",newline="") as f:
                w=csv.DictWriter(f,fieldnames=["x_cm","y_cm","to_name","t_ms","obstacle_x_cm","obstacle_y_cm"])
                w.writeheader()
                w.writerow(dict(x_cm=10,y_cm=0,to_name="TURN_OUT",t_ms=150,
                                obstacle_x_cm=40,obstacle_y_cm=15))
            with open(root/"avoidance_runs.csv","w",newline="") as f:
                w=csv.DictWriter(f,fieldnames=["start_ms","end_ms"])
                w.writeheader();w.writerow(dict(start_ms=100,end_ms=300))
            out=render(str(root))
            result=Path(out).read_text()
            self.assertIn("<svg ",result)
            self.assertIn("Obstacle est.",result)
            self.assertIn("TURN_OUT",result)


if __name__=="__main__":
    unittest.main()
