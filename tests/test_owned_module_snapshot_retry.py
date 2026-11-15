from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import Mock,patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import offline_gi_instance as subject


class OwnedModuleSnapshotTests(unittest.TestCase):
    def owned(self,results):
        owned=subject._Owned.__new__(subject._Owned)
        owned.pid=123
        owned.handle=456
        owned.process=SimpleNamespace(poll=Mock(return_value=None))
        owned.note=Mock()
        def first(_,pointer):
            item=pointer._obj
            item.name='own.dll'
            item.path='F:\\package\\own.dll'
            item.base=0x100000
            item.bytes=4096
            return True
        owned.api=SimpleNamespace(CreateToolhelp32Snapshot=Mock(side_effect=results),
                                  GetProcessId=Mock(return_value=123),Module32FirstW=Mock(side_effect=first),
                                  Module32NextW=Mock(return_value=False),CloseHandle=Mock())
        return owned

    @patch.object(subject.time,'sleep')
    @patch.object(subject.C,'get_last_error',return_value=24)
    def test_transient_loader_change_recovers(self,error,sleep):
        owned=self.owned([subject.W.HANDLE(-1).value,subject.W.HANDLE(-1).value,77])
        modules=owned.modules()
        self.assertEqual(modules['own.dll'][:2],(0x100000,4096))
        self.assertEqual(owned.api.CreateToolhelp32Snapshot.call_count,3)
        self.assertEqual(sleep.call_count,2)
        owned.api.CloseHandle.assert_called_once_with(77)

    @patch.object(subject.time,'sleep')
    @patch.object(subject.C,'get_last_error',return_value=24)
    def test_retry_is_bounded(self,error,sleep):
        owned=self.owned([subject.W.HANDLE(-1).value]*16)
        with self.assertRaisesRegex(RuntimeError,'Win32 24'):
            owned.modules()
        self.assertEqual(owned.api.CreateToolhelp32Snapshot.call_count,16)
        self.assertEqual(sleep.call_count,15)
        owned.api.CloseHandle.assert_not_called()

    @patch.object(subject.C,'get_last_error',return_value=5)
    def test_other_errors_fail_without_retry(self,error):
        owned=self.owned([subject.W.HANDLE(-1).value])
        with self.assertRaisesRegex(RuntimeError,'Win32 5'):
            owned.modules()
        self.assertEqual(owned.api.CreateToolhelp32Snapshot.call_count,1)

    def test_exited_or_wrong_handle_owner_never_enumerates(self):
        for exited,pid in ((0,123),(None,124)):
            owned=self.owned([77])
            owned.process.poll.return_value=exited
            owned.api.GetProcessId.return_value=pid
            with self.assertRaisesRegex(RuntimeError,'child exited'):
                owned.modules()
            owned.api.CreateToolhelp32Snapshot.assert_not_called()


if __name__=='__main__':
    unittest.main()
