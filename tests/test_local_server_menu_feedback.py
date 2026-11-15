"""Compile production menu/context bodies; no game process is launched."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'work/local-server-playable/menu-feedback-20261114/tests'
COMPILER = Path(r'F:\VSBuildTools\2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe')

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 0
    for index in range(opening, len(source)):
        depth += (source[index] == '{') - (source[index] == '}')
        if not depth:
            return source[start:index + 1]
    raise AssertionError(signature)

def compile_run(name, source):
    OUT.mkdir(parents=True, exist_ok=True)
    cpp, exe = OUT / (name + '.cpp'), OUT / (name + '.exe')
    cpp.write_text(source, encoding='utf-8')
    vc = COMPILER.parents[3]
    sdk = Path(r'F:\zzz-rev\tools\sdk-nuget\headers\c')
    env = dict(os.environ)
    env['PATH'] = str(COMPILER.parent) + os.pathsep + env['PATH']
    env['INCLUDE'] = ';'.join(map(str, (vc / 'include', sdk / 'Include/10.0.26100.0/ucrt')))
    env['LIB'] = ';'.join(map(str, (vc / 'lib/x64', sdk / 'Lib/10.0.26100.0/ucrt/x64', sdk / 'Lib/10.0.26100.0/um/x64')))
    command = [str(COMPILER), '/nologo', '/std:c++17', '/W4', '/WX', '/EHsc', '/O2', '/MT',
               '/Fo' + str(OUT / (name + '.obj')), '/Fe' + str(exe), str(cpp), '/link', '/INCREMENTAL:NO']
    built = subprocess.run(command, env=env, cwd=OUT, text=True, capture_output=True, timeout=60)
    (OUT / (name + '-build.log')).write_text(built.stdout + built.stderr)
    if built.returncode:
        raise AssertionError(built.stdout + built.stderr)
    run = subprocess.run([str(exe)], cwd=OUT, text=True, capture_output=True, timeout=20)
    (OUT / (name + '-run.log')).write_text(run.stdout + run.stderr)
    if run.returncode:
        raise AssertionError(run.stdout + run.stderr)
    print(run.stdout.strip())

PRELUDE = r'''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
// Fake globals deliberately use the production argument names.
#pragma warning(disable:4459)
unsigned checks=0;
void Check(bool ok,const char* name){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}
template<class T>T Read(const void* p,size_t off){T v{};if(p)std::memcpy(&v,static_cast<const uint8_t*>(p)+off,sizeof(v));return v;}
template<class T>bool Write(void* p,size_t off,T v){if(!p)return false;std::memcpy(static_cast<uint8_t*>(p)+off,&v,sizeof(v));return true;}
'''

@unittest.skipUnless(os.name == 'nt' and COMPILER.exists(), 'requires MSVC')
class MenuFeedbackTests(unittest.TestCase):
    def test_recorded_feedback_selects_the_actual_null_dependency(self):
        data = json.loads((ROOT / 'tests/fixtures/menu_feedback_20261114.json').read_text())
        self.assertEqual(data['bridge_error'],4555)
        self.assertEqual(data['last_fault_operation'],'ui_input_mode')
        self.assertEqual(data['last_fault_delta'],1)
        self.assertTrue(data['ui_options_open'])
        self.assertEqual(data['window_menu_input']['error'],0)
        self.assertTrue(data['window_selection_probe']['complete'])
        self.assertFalse(data['window_selection_probe']['selecting'])
        self.assertEqual(data['fault']['exception_code'],0xc0000005)
        self.assertFalse(data['fault']['breakpoint_observation'])
        self.assertEqual((data['fault']['rbx'],data['fault']['exception_information_1']),(0,0x28))
        self.assertEqual(data['fault_rva'],'0x79365df')
        import pefile
        pe=pefile.PE(str(ROOT / 'work/client-analysis/Shipping-analysis-copy.exe'),fast_load=True)
        self.assertEqual(pe.get_data(0x79365c5,7),bytes.fromhex('498b9ff8000000'))
        self.assertEqual(pe.get_data(int(data['fault_rva'],16),4),bytes.fromhex('488b4b28'))

    def test_original_context_construction_and_lifetime(self):
        source = (ROOT / 'native/original_input_bindings.h').read_text(encoding='utf-8-sig')
        body = function(source, 'inline bool EnsureSettingsCommitContext(')
        struct = function(source, 'struct SettingsCommitContext ') + ';'
        cpp = PRELUDE + r'''
struct Object{std::array<uint8_t,4096> data{};uintptr_t type=0;int serial=1;bool live=true;};
Object client,cache,manager,foreign,settings,rso;
constexpr uintptr_t ClientClass=0x6429080;
bool thread=true,profile=true,constructFails=false,queueFails=false;unsigned ctorCalls=0;
namespace engine{void* Class(uintptr_t type){return reinterpret_cast<void*>(type);}}
namespace original_players{bool IsA(void* p,uintptr_t type){return p&&static_cast<Object*>(p)->live&&static_cast<Object*>(p)->type==type;}}
namespace original_inventory_slots{
struct WeakIdentity{void* object=nullptr;int serial=0;};
bool CaptureIdentity(void* p,WeakIdentity& id){id={p,static_cast<Object*>(p)->serial};return true;}
void* ResolveIdentity(const WeakIdentity& id){auto p=static_cast<Object*>(id.object);return p&&p->live&&p->serial==id.serial?p:nullptr;}
}
struct{uintptr_t client=0,telemetry=0;unsigned error=0;const char* stage="none";}state;
bool Thread(){return thread;}
bool Fail(unsigned e,const char* s){state.error=e;state.stage=s;return false;}
bool VerifySettingsCommitProfile(){return profile;}
void* Construct(void* type,void* outer){
 ++ctorCalls;if(constructFails)return nullptr;
 manager=Object{};manager.type=reinterpret_cast<uintptr_t>(type);Write(&manager,0x28,outer);
 for(size_t off:{size_t(0xd0),size_t(0xe8),size_t(0xf0),size_t(0xf8)})Write(&manager,off,static_cast<void*>(&manager));
 if(queueFails)Write<void*>(&manager,0xe8,nullptr);
 return &manager;
}
''' + struct + '\nSettingsCommitContext settingsCommit;\n' + body + r'''
void Reset(){
 client=Object{};cache=Object{};manager=Object{};foreign=Object{};state={};settingsCommit={};
 client.type=ClientClass;cache.type=0x6a81250;thread=profile=true;constructFails=queueFails=false;ctorCalls=0;
 state.client=reinterpret_cast<uintptr_t>(&client);state.telemetry=reinterpret_cast<uintptr_t>(&cache);
 Write<void*>(&client,0x5c0,&cache);Write<void*>(&client,0x378,&settings);Write<void*>(&client,0x750,&rso);
 Write<void*>(&cache,0xe0,&settings);Write<void*>(&cache,0xe8,&rso);
}
int main(){
 Reset();Check(!EnsureSettingsCommitContext(&client),"old null f8 is rejected before commit");Check(state.error==1564&&!settingsCommit.ready,"specific missing-context error");
 Check(EnsureSettingsCommitContext(&client,true),"initialize actual local context");
 Check(Read<void*>(&cache,0xf8)==&manager&&Read<void*>(&manager,0x28)==&client,"original typed dependency and actual outer");
 Check(Read<void*>(&client,0x2d0)==nullptr&&Read<void*>(&cache,0xf0)==nullptr,"no global telemetry/sampler publication");
 Check(Read<void*>(&manager,0xd8)==nullptr&&Read<uint16_t>(&manager,0x78)==0,"no online transport or initialized flag");
 Check(EnsureSettingsCommitContext(&client,true)&&ctorCalls==1,"repeat initialization retains context");
 Check(EnsureSettingsCommitContext(&client),"close uses live original context");
 manager.serial++;Check(!EnsureSettingsCommitContext(&client)&&state.error==1564,"recycled UObject rejected");
 Reset();Check(EnsureSettingsCommitContext(&client,true),"fresh lifecycle");manager.live=false;Check(!EnsureSettingsCommitContext(&client),"retired manager rejected");
 Reset();foreign.type=0x6d73e20;Write<void*>(&cache,0xf8,&foreign);Check(!EnsureSettingsCommitContext(&client,true)&&ctorCalls==0,"foreign manager neither replaced nor adopted");
 Reset();profile=false;Check(!EnsureSettingsCommitContext(&client,true)&&state.error==1562&&ctorCalls==0,"wrong binary cannot construct");
 Reset();thread=false;Check(!EnsureSettingsCommitContext(&client,true)&&state.error==1560,"wrong thread rejected");
 Reset();constructFails=true;Check(!EnsureSettingsCommitContext(&client,true)&&state.error==1563,"constructor failure stays failed");
 Reset();queueFails=true;Check(!EnsureSettingsCommitContext(&client,true)&&state.error==1565,"missing native event rejected");
 Reset();Check(EnsureSettingsCommitContext(&client,true),"queue base");Write<int32_t>(&manager,0xe0,-1);Check(!EnsureSettingsCommitContext(&client),"stopped queue rejected");
 Reset();Check(EnsureSettingsCommitContext(&client,true),"local base");Write<void*>(&manager,0xd8,&foreign);Check(!EnsureSettingsCommitContext(&client),"bound network transport rejected");
 Reset();Write<void*>(&cache,0xe0,&foreign);Check(!EnsureSettingsCommitContext(&client,true)&&state.error==1561,"foreign settings rejected");
 std::printf("settings context: %u checks passed\n",checks);
}
'''
        compile_run('settings-context', cpp)

    def test_options_exit_preserves_original_flow_and_popup_gates(self):
        source = (ROOT / 'native/original_ui_input_mode.h').read_text(encoding='utf-8-sig')
        body = function(source, 'inline bool DismissTopEntryImpl(')
        cpp = PRELUDE + r'''
std::array<uint8_t,64> entryObject{},widgetObject{},exitFunction{},stackObject{};
void* entry=entryObject.data();void* widget=widgetObject.data();
bool complete=true,selecting=false,ready=true,dispatchOK=true;
unsigned exits=0,applies=0,commits=0,dismiss=0,updates=0;bool closed=false;
struct{bool optionsKnown=true,optionsOpen=true;void* optionsEntry=entry;void* optionsWidget=widget;void* menuStack=stackObject.data();unsigned error=0,originalOptionsExits=0;const char* stage="none";}state;
struct{void* optionsExit=exitFunction.data();}functions;
struct{unsigned requests=0,staleRequests=0;void* handleDismiss=stackObject.data();}windowDismiss;
namespace original_hud{struct S{void* clientInstance=entry;}s;const S& State(){return s;}bool Derived(void*,void*){return true;}}
namespace original_input_bindings{
struct{unsigned error=1564;const char* stage="original_settings_commit_context_not_local_or_live";}state;
const auto& State(){return state;}bool EnsureSettingsCommitContext(void*){return ready;}
}
bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
bool UpdateImpl(bool){++updates;return true;}
bool ReadTopEntryImpl(void*& a,void*& b){a=entry;b=widget;return true;}
bool TopHasSelectingKeyImpl(void*,void*,bool& a,bool& b){a=complete;b=selecting;return true;}
bool Dispatch(void*,void* fn,void*){
 if(!dispatchOK)return false;
 if(fn==functions.optionsExit){++exits;++applies;++commits;closed=true;}else ++dismiss;
 return true;
}
''' + body + r'''
int main(){
 Check(DismissTopEntryImpl(entry,widget,true),"Esc complete original options exit");
 Check(exits==1&&applies==1&&commits==1&&closed&&windowDismiss.requests==1,"apply and commit and close all preserved");
 ready=false;Check(!DismissTopEntryImpl(entry,widget,true)&&state.error==1564&&exits==1,"null/retired context fails before original graph");
 ready=true;Check(DismissTopEntryImpl(entry,widget,false)&&exits==1,"pre-event permission remains required");
 complete=false;Check(DismissTopEntryImpl(entry,widget,true)&&exits==1,"unknown selector does not close");
 complete=true;selecting=true;Check(DismissTopEntryImpl(entry,widget,true)&&exits==1,"key-selection Escape remains consumed");
 selecting=false;Check(DismissTopEntryImpl(nullptr,widget,true)&&exits==1,"stale menu identity does not close");
 dispatchOK=false;Check(!DismissTopEntryImpl(entry,widget,true)&&state.error==3148,"native dispatch failure preserved");
 dispatchOK=true;state.optionsOpen=false;Check(DismissTopEntryImpl(entry,widget,false)&&dismiss==1,"B or other top entry retains native HandleDismissInput");
 Check(exits==1&&state.originalOptionsExits==1,"failure never increments successful exit count");
 std::printf("original options graph: %u checks passed\n",checks);
}
'''
        compile_run('options-exit', cpp)

    def test_complete_profiles_match_pinned_original_bytes(self):
        import pefile
        path = ROOT / 'work/client-analysis/Shipping-analysis-copy.exe'
        raw = path.read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(), '147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f')
        pe = pefile.PE(data=raw, fast_load=True)
        source = function((ROOT / 'native/original_input_bindings.h').read_text(encoding='utf-8-sig'), 'inline bool VerifySettingsCommitProfile(')
        rows = re.findall(r'\{(0x[0-9a-f]+),(0x[0-9a-f]+),(0x[0-9a-f]+)ULL\}', source)
        self.assertEqual(len(rows), 5)
        for rva, size, expected in rows:
            value = 14695981039346656037
            for byte in pe.get_data(int(rva,16),int(size,16)):
                value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
            self.assertEqual(value,int(expected,16),rva)

if __name__ == '__main__':
    unittest.main()
