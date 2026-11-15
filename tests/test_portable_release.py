"""Successful shooting cannot hide missing-resource warnings in a shareable build."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_portable_release import assess_game_log


class PortableReleaseTests(unittest.TestCase):
    def test_missing_optional_catalog_packages_still_block_release(self):
        result = assess_game_log("LogIoDispatcher: Warning: Signature error detected in container 'pakchunk10-WindowsClient'\n"
                                 'LogStreaming: Warning: StartBundleIoRequests: FailedRead: /Game/Example\n')
        self.assertFalse(result['passed'])
        self.assertEqual(result['signature_errors'], 1)

    def test_known_source_constructor_warning_is_not_a_corruption_signal(self):
        self.assertTrue(assess_game_log('CDO Constructor (TextRenderComponent): Failed to find /Engine/DefaultTextMaterialOpaque')['passed'])
        self.assertFalse(assess_game_log('Detected corrupted Pak files. Loading game in repair mode.')['passed'])


if __name__ == '__main__':
    unittest.main()
