import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"PC_Mapping"))
from map_logger import decode_sample


class PoseDecoderTests(unittest.TestCase):
    def test_18_field_packet_includes_wheel_only_prediction(self):
        d=[1,250,500,800,500,45,50,25,1350,370,20,30,18,105,3,1220,19,27]
        x=decode_sample(d)
        self.assertAlmostEqual(x["yaw_deg"],135.0)
        self.assertAlmostEqual(x["dist_cm"],37.0)
        self.assertAlmostEqual(x["kin_heading_deg"],122.0)
        self.assertEqual((x["kin_x_cm"],x["kin_y_cm"]),(19,27))

    def test_old_15_field_packet_stays_readable(self):
        d=[1,250,500,800,500,45,50,25,1350,370,20,30,18,105,3]
        x=decode_sample(d)
        self.assertNotIn("kin_heading_deg",x)


if __name__=="__main__":
    unittest.main()
