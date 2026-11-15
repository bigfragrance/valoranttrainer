"""WASAPI endpoint loopback. Captures all applications on the selected output."""
from __future__ import annotations
import threading
import time
import numpy as np
from core import RATE, stereo


def devices():
    import pyaudiowpatch as pa
    with pa.PyAudio() as p:
        default = p.get_default_wasapi_loopback()["index"]
        return [dict(index=int(d["index"]), name=d["name"], channels=int(d["maxInputChannels"]),
                     rate=int(d["defaultSampleRate"]), default=d["index"] == default)
                for d in p.get_loopback_device_info_generator()]


class Capture:
    def __init__(self, device, on_audio, on_error):
        self.device, self.on_audio, self.on_error = device, on_audio, on_error
        self.stop_event = threading.Event()
        self.thread = None

    def start(self):
        if self.device["channels"] != 2:
            raise ValueError("请选择两声道输出；多声道输出不能直接作为 HRTF 双耳信号。")
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.thread.start()

    def stop(self):
        self.stop_event.set()

    def run(self):
        import pyaudiowpatch as pa
        try:
            with pa.PyAudio() as p:
                rate = self.device["rate"]
                block = round(rate*.04)
                with p.open(format=pa.paFloat32, channels=2, rate=rate, input=True,
                            input_device_index=self.device["index"], frames_per_buffer=block) as stream:
                    while not self.stop_event.is_set():
                        raw = stream.read(block, exception_on_overflow=True)
                        # Endpoint delivery time; not a guaranteed hardware timestamp.
                        x = np.frombuffer(raw, dtype=np.float32).reshape(-1, 2).copy()
                        self.on_audio(stereo(x, rate), time.time())
        except Exception as exc:
            if not self.stop_event.is_set():
                self.on_error(str(exc))
