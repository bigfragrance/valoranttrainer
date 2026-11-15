from pathlib import Path
import sys
import tempfile
import unittest
import math
import struct
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from training_settings import load, save, DEFAULTS
from training_channel import TrainingChannel, NativeFrame, MAGIC
from training_host import TrainingHost, atomic_json

class SettingsTests(unittest.TestCase):
    def test_saved_settings_survive_restart_and_invalid_numbers_are_bounded(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/'settings.json'
            save({'sensitivity': .72, 'fov': 10000, 'hud_scale': math.nan, 'minimap': False}, path)
            value = load(path)
            self.assertEqual(value['sensitivity'], .72)
            self.assertEqual(value['fov'], 120)
            self.assertEqual(value['hud_scale'], 1)
            self.assertFalse(value['minimap'])
            path.write_text('{corrupt')
            self.assertEqual(load(path), DEFAULTS)

    def test_settings_menu_does_not_quit_game_and_resumes_by_owned_ui_command(self):
        with tempfile.TemporaryDirectory() as folder:
            data = bytearray(4096)
            struct.pack_into('<4I', data, 0, MAGIC, 1, 1234, 1)
            channel = TrainingChannel(data,1234)
            host = TrainingHost(channel,Path(folder)/'training',ui_enabled=True)
            frame = NativeFrame(2,1,(0,0,174),(0,0,0),8|32)
            self.assertFalse(host.handle_escape(frame))
            self.assertTrue(host.settings_open)
            self.assertFalse(host.stop_requested)
            self.assertFalse((Path(folder)/'stop-request.json').exists())
            self.assertEqual(struct.unpack_from('<I',data,664)[0],1)
            atomic_json(host.run_dir/'ui-command.json',{'id':1,'action':'resume'})
            host._last_settings_poll=-math.inf
            host.poll_settings()
            self.assertFalse(host.settings_open)
            self.assertEqual(struct.unpack_from('<I',data,664)[0],0)

if __name__=='__main__': unittest.main()
