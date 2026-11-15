"""Protocol layout and stale/timeout/identity checks without starting a process."""
import math
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from training_channel import (ChannelError, ChannelNotReady, ChannelTimeout,
                              MAGIC, RenderState, TrainingChannel)


def mapping():
    result = bytearray(4096)
    struct.pack_into("<4I", result, 0, MAGIC, 1, 1234, 1)
    struct.pack_into("<I", result, 64, 2)
    struct.pack_into("<7dI", result, 72, 1000., -2050., 0., 300., 0., 90., 0., 9)
    return result


class ChannelTests(unittest.TestCase):
    def test_frame_offsets_and_pid_identity(self):
        channel = TrainingChannel(mapping(), 1234)
        frame = channel.read_frame()
        self.assertEqual(frame.sequence, 2)
        self.assertEqual(frame.time, 1000.)
        self.assertEqual(frame.position, (-2050., 0., 300.))
        self.assertEqual(frame.rotation, (0., 90., 0.))
        self.assertEqual(frame.input_flags, 9)
        with self.assertRaises(ChannelError):
            TrainingChannel(mapping(), 777)

    def test_writing_frame_is_never_consumed(self):
        data = mapping()
        struct.pack_into("<I", data, 64, 3)
        with self.assertRaises(ChannelNotReady):
            TrainingChannel(data, 1234, sleep=lambda _: None).read_frame()

    def test_actual_target_head_extends_frame_without_moving_native_ui_fields(self):
        data = mapping()
        struct.pack_into("<I", data, 172, 9)
        data[176:188] = bytes([0xab])*12  # Reserved bytes are not decoded as target data.
        struct.pack_into("<I3d", data, 188, 42, 1010., -20., 389.5)
        frame = TrainingChannel(data, 1234).read_frame()
        self.assertEqual(frame.native_ui_flags, 9)
        self.assertEqual(frame.target_bot_id, 42)
        self.assertEqual(frame.target_head, (1010., -20., 389.5))
        legacy = TrainingChannel(mapping(), 1234).read_frame()
        self.assertEqual((legacy.target_bot_id, legacy.target_head), (0, (0, 0, 0)))
        struct.pack_into("<d", data, 192, math.nan)
        with self.assertRaises(ValueError):
            TrainingChannel(data, 1234).read_frame()

    def test_jett_payload_exact_offsets_and_default_compatibility(self):
        data = mapping()
        self.assertEqual(TrainingChannel(data, 1234).read_frame().equipment_mode, 0)
        struct.pack_into("<3I3f4I", data, 132, 1, 7, 3, .5, .75, 1., 2, 4, 6, 8)
        frame = TrainingChannel(data, 1234).read_frame()
        self.assertEqual((frame.equipment_mode, frame.skill_flags, frame.smoke_count), (1, 7, 3))
        self.assertEqual((frame.smoke_cooldown, frame.updraft_cooldown, frame.dash_cooldown), (.5, .75, 1))
        self.assertEqual((frame.smoke_casts, frame.updraft_casts, frame.dash_casts, frame.ultimate_casts), (2, 4, 6, 8))

    def test_invalid_jett_mode_and_cooldowns_are_rejected(self):
        for offset, fmt, value in ((132, "I", 2), (136, "I", 8), (140, "I", 9),
                                   (144, "f", math.nan), (148, "f", -1)):
            data = mapping()
            struct.pack_into("<"+fmt, data, offset, value)
            with self.subTest(offset=offset), self.assertRaises(ChannelError):
                TrainingChannel(data, 1234).read_frame()

    def test_render_exact_offsets_and_even_published_sequence(self):
        data = mapping()
        channel = TrainingChannel(data, 1234)
        channel.write_render(RenderState(7, (100, 200, 300), 45, 1, 2, .5, 10, 15, 3, 4, 7, 110))
        self.assertEqual(struct.unpack_from("<I", data, 512)[0], 2)
        self.assertEqual(struct.unpack_from("<I", data, 516)[0], 7)
        self.assertEqual(struct.unpack_from("<3d", data, 520), (100, 200, 300))
        self.assertEqual(struct.unpack_from("<4d", data, 544), (45, 1, 2, .5))
        self.assertEqual(struct.unpack_from("<5I", data, 576), (10, 15, 3, 4, 7))
        self.assertEqual(struct.unpack_from("<f", data, 596)[0], 110)

    def test_trace_payload_is_published_before_request_sequence(self):
        data = mapping()
        clock = [0.]
        observed = []
        def sleep(dt):
            clock[0] += dt
            sequence = struct.unpack_from("<I", data, 256)[0]
            if sequence:
                observed.append(struct.unpack_from("<I7d", data, 260))
                struct.pack_into("<id3d", data, 388, 1, 100., 0., 0., 200.)
                struct.pack_into("<I", data, 384, sequence)
        channel = TrainingChannel(data, 1234, clock=lambda: clock[0], sleep=sleep)
        result = channel.trace((0, 0, 300), (0, 0, -5), 1000)
        self.assertTrue(result.hit)
        self.assertEqual(result.impact, (0, 0, 200))
        self.assertEqual(observed[0], (1, 0, 0, 300, 0, 0, -1, 1000))

    def test_timeout_does_not_allow_overwriting_inflight_request(self):
        data = mapping()
        clock = [0.]
        channel = TrainingChannel(data, 1234, clock=lambda: clock[0], sleep=lambda dt: clock.__setitem__(0, clock[0]+dt))
        with self.assertRaises(ChannelTimeout):
            channel.trace((0, 0, 300), (0, 0, -1), timeout=.01)
        first = bytes(data[256:320])
        with self.assertRaisesRegex(ChannelError, "in flight"):
            channel.trace((5, 0, 300), (0, 0, -1), timeout=.01)
        self.assertEqual(bytes(data[256:320]), first)

    def test_native_target_opcode_region_identity_and_shared_request_sequence(self):
        data = mapping()
        clock = [0.]
        commands = []
        def sleep(dt):
            clock[0] += dt
            commands.append(struct.unpack_from("<I7d", data, 260))
            struct.pack_into("<id3diI", data, 388, 1, 990., 990., 0., 190., 1, 42)
            struct.pack_into("<I", data, 384, struct.unpack_from("<I", data, 256)[0])
        channel = TrainingChannel(data, 1234, clock=lambda: clock[0], sleep=sleep)
        result = channel.trace_target((0, 0, 190), (7, 0, 0))
        self.assertEqual((result.hit, result.distance_cm, result.impact, result.region, result.bot_id),
                         (True, 990., (990., 0., 190.), 1, 42))
        self.assertEqual(commands[0], (2, 0, 0, 190, 1, 0, 0, 100000))
        channel.trace((0, 0, 190), (1, 0, 0))
        self.assertEqual([c[0] for c in commands], [2, 1])
        self.assertEqual(struct.unpack_from("<I", data, 256)[0], 2)

    def test_target_metadata_rejected_and_timeout_cannot_fall_back_to_scene_query(self):
        for region, bot_id in ((-1, 1), (3, 1), (1, 0)):
            data = mapping()
            clock = [0.]
            def sleep(dt):
                clock[0] += dt
                struct.pack_into("<id3diI", data, 388, 1, 5., 5., 0., 0., region, bot_id)
                struct.pack_into("<I", data, 384, struct.unpack_from("<I", data, 256)[0])
            with self.subTest(region=region, bot_id=bot_id), self.assertRaises(ChannelError):
                TrainingChannel(data, 1234, clock=lambda: clock[0], sleep=sleep).trace_target((0, 0, 0), (1, 0, 0))
        data = mapping()
        clock = [0.]
        channel = TrainingChannel(data, 1234, clock=lambda: clock[0], sleep=lambda dt: clock.__setitem__(0, clock[0]+dt))
        with self.assertRaises(ChannelTimeout):
            channel.trace_target((0, 0, 0), (1, 0, 0), timeout=.01)
        with self.assertRaisesRegex(ChannelError, "in flight"):
            channel.trace((0, 0, 0), (1, 0, 0))

    def test_native_error_miss_and_invalid_distance(self):
        for status, distance, expected in ((-2, 0, ChannelError), (1, -1, ChannelError), (0, 0, ChannelError), (0, math.inf, None)):
            data = mapping()
            clock = [0.]
            def sleep(dt):
                clock[0] += dt
                struct.pack_into("<id3d", data, 388, status, distance, 0, 0, 0)
                struct.pack_into("<I", data, 384, struct.unpack_from("<I", data, 256)[0])
            channel = TrainingChannel(data, 1234, clock=lambda: clock[0], sleep=sleep)
            if expected:
                with self.assertRaises(expected):
                    channel.trace((0, 0, 0), (1, 0, 0), 100)
            else:
                self.assertFalse(channel.trace((0, 0, 0), (1, 0, 0), 100).hit)

    def test_nonfinite_frame_and_render_are_rejected(self):
        data = mapping()
        struct.pack_into("<d", data, 80, math.nan)
        channel = TrainingChannel(data, 1234)
        with self.assertRaises(ValueError):
            channel.read_frame()
        with self.assertRaises(ValueError):
            channel.write_render(RenderState(health=math.nan))


if __name__ == "__main__":
    unittest.main()
