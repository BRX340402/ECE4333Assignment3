import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "PC_Mapping"))
from map_logger import decode_a4_event, summarize_avoidance


def packet(t, run, before, after, obstacle=(-32768, -32768)):
    return [t, run, before, after, 23, 41, 123, 560, 18, 90, 1, 1,
            140, 300, obstacle[0], obstacle[1], 0]


class EventDecoderTests(unittest.TestCase):
    def test_trigger_and_obstacle_point(self):
        v = decode_a4_event(packet(100, 3, 0, 2, (40, 52)))
        self.assertEqual(v["from_name"], "TRACK")
        self.assertEqual(v["to_name"], "TURN_OUT")
        self.assertEqual(v["obstacle_x_cm"], 40)
        self.assertEqual(v["yaw_deg"], 12.3)

    def test_unknown_obstacle_position(self):
        v = decode_a4_event(packet(150, 3, 2, 3))
        self.assertEqual(v["obstacle_x_cm"], "")
        self.assertEqual(v["obstacle_y_cm"], "")

    def test_success_duration(self):
        records = [decode_a4_event(packet(100, 7, 0, 2, (40, 52))),
                   decode_a4_event(packet(3000, 7, 14, 0))]
        runs = summarize_avoidance(records)
        self.assertEqual(len(runs), 1)
        self.assertEqual(runs[0]["outcome"], "success")
        self.assertAlmostEqual(runs[0]["duration_s"], 2.9)

    def test_fault_not_success(self):
        records = [decode_a4_event(packet(100, 7, 0, 2)),
                   decode_a4_event(packet(200, 7, 3, 15))]
        self.assertEqual(summarize_avoidance(records)[0]["outcome"], "fault")

    def test_wait_clears_without_detour(self):
        records = [decode_a4_event(packet(100, 7, 0, 1)),
                   decode_a4_event(packet(250, 7, 1, 0))]
        self.assertEqual(summarize_avoidance(records)[0]["outcome"],
                         "cleared_without_detour")

    def test_malformed(self):
        with self.assertRaises(ValueError):
            decode_a4_event([1, 2, 3])


if __name__ == "__main__":
    unittest.main()
