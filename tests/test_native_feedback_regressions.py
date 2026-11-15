"""Compile production comparison/adoption bodies against retained data and fake UObjects."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
COMPILER = Path(r'F:\VSBuildTools\2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe')


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == '{':
            depth += 1
        elif source[index] == '}':
            depth -= 1
            if not depth:
                return source[start:index + 1]
    raise AssertionError(signature)


def compile_and_run(source):
    with tempfile.TemporaryDirectory(prefix='native-feedback-') as temporary:
        directory = Path(temporary)
        cpp, executable = directory / 'fixture.cpp', directory / 'fixture.exe'
        cpp.write_text(source, encoding='utf-8')
        vc = COMPILER.parents[3]
        sdk = Path(r'F:\zzz-rev\tools\sdk-nuget\headers\c')
        environment = dict(os.environ)
        environment['PATH'] = str(COMPILER.parent) + os.pathsep + environment['PATH']
        environment['INCLUDE'] = ';'.join(str(path) for path in (
            vc / 'include', sdk / 'Include/10.0.26100.0/ucrt'))
        environment['LIB'] = ';'.join(str(path) for path in (
            vc / 'lib/x64', sdk / 'Lib/10.0.26100.0/ucrt/x64', sdk / 'Lib/10.0.26100.0/um/x64'))
        command = [str(COMPILER), '/nologo', '/std:c++17', '/W4', '/WX', '/EHsc', '/O2', '/MT',
                   '/I' + str(ROOT / 'native'), '/Fo' + str(directory / 'fixture.obj'),
                   '/Fe' + str(executable), str(cpp), '/link', '/INCREMENTAL:NO']
        result = subprocess.run(command, cwd=directory, env=environment, capture_output=True,
                                text=True, timeout=60)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        result = subprocess.run([str(executable)], cwd=directory, capture_output=True,
                                text=True, timeout=20)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        print(result.stdout.strip())
        return result.stdout


PRELUDE = r'''
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include "native_input_checkpoint_frame.h"
unsigned checks=0;
void Check(bool value,const char* label){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",label);std::exit(1);}}
template<class T>T Read(const void* object,size_t offset=0){T value{};if(object)std::memcpy(&value,static_cast<const uint8_t*>(object)+offset,sizeof(value));return value;}
template<class T>void Put(void* object,size_t offset,T value){std::memcpy(static_cast<uint8_t*>(object)+offset,&value,sizeof(value));}
'''


@unittest.skipUnless(os.name == 'nt' and COMPILER.exists(), 'requires local MSVC')
class NativeFeedbackRegressions(unittest.TestCase):
    def test_exact_38_record_checkpoint_and_active_field_negative_cases(self):
        data = json.loads((ROOT / 'tests/fixtures/native_feedback_checkpoint_20261114.json').read_text())
        self.assertEqual((data['count_before'], data['count_after'], data['correction_passed_mask']),
                         (74, 112, 0x3fff))
        arrays = []
        for name in ('retained', 'wire'):
            rows = ['{' + ','.join(str(byte) for byte in bytes.fromhex(row)) + '}' for row in data[name]]
            arrays.append('std::array<std::array<uint8_t,40>,38> ' + name + '{{' + ','.join(rows) + '}};')
        source = (ROOT / 'native/local_server_input.h').read_text(encoding='utf-8-sig')
        body = function(source, 'inline bool SameReplacementFrame(')
        cpp = PRELUDE + r'''
namespace original_players{bool Readable(const void* value,size_t){return value!=nullptr;}}
struct Barrier{bool present=false;int32_t eventId=0;float clientSeconds=0;};
struct Config{uint32_t maxInputEvents=100;} Configuration;
struct State{struct{native_input_checkpoint_frame::Proof replacementFrame;}correctionCancellation;std::array<Barrier,8>barrierTimes{};}state;
''' + body + '\n' + '\n'.join(arrays) + r'''
int main(){
    const auto first=140;
    auto prove=[&](){return SameReplacementFrame(retained[0].data(),wire[0].data(),first,38,38);};
    Check(prove(),"complete actual checkpoint without aim passes");
    const auto actual=state.correctionCancellation.replacementFrame;
    Check(actual.compared==35&&actual.type3States==1&&actual.type10States==1&&actual.aimOrdinal==0,"all actual suffix states compared");
    const auto read=[&](bool fromWire,uint32_t ordinal){native_input_checkpoint_frame::Event event{};
        const auto& bytes=(fromWire?wire:retained)[ordinal];event.type=bytes[0];
        if(event.type==7)event.deadline=Read<float>(bytes.data(),8);
        else if(event.type==0){event.equippableBits=bytes[16]&31;event.extra=Read<float>(bytes.data(),20);event.extraBits=Read<uint32_t>(bytes.data(),20);}
        else if(event.type==1)event.activationBits=Read<uint16_t>(bytes.data(),20)&0x3f3f;
        else if(event.type==3)event.type3Flags=bytes[8]&3;
        else if(event.type==10){event.type10Value=bytes[8];event.type10Flag=bytes[9]?1:0;}
        return event;};
    Check(native_input_checkpoint_frame::Prove(39,38,100,read).matched,"later retained packet does not truncate fresh packet proof");
    for(size_t n=4;n<38;++n){
        auto original=wire[n];const auto kind=wire[n][0];
        const size_t offset=kind==0?16:kind==1?20:8;
        wire[n][offset]^=1;Check(!prove(),"changed actual active field rejected");wire[n]=original;
        wire[n][39]^=0x55;Check(prove(),"native inactive union bytes ignored");wire[n]=original;
    }
    wire[36][8]^=2;Check(!prove(),"type3 second serialized flag rejected");wire=retained;
    wire[37][9]=1;Check(!prove(),"type10 serialized boolean rejected");wire=retained;
    wire[4][20]^=1;Check(!prove(),"type0 serialized float bits rejected");wire=retained;
    Put<float>(wire[4].data(),20,(std::numeric_limits<float>::quiet_NaN)());Check(!prove(),"nonfinite native state rejected");wire=retained;
    Put<float>(wire[3].data(),8,1);Check(!prove(),"client checkpoint time changed");wire=retained;
    state.barrierTimes[0]={true,first+3,Read<float>(wire[3].data(),8)};
    Put<float>(retained[3].data(),8,2);Check(prove(),"already mapped retained deadline uses original client ledger");
    state.barrierTimes={};retained=wire;
    for(uint8_t type:std::array<uint8_t,7>{4,6,7,8,9,11,12}){wire=retained;wire[35][0]=retained[35][0]=type;Check(!prove(),"unsupported/new checkpoint rejected");retained[35][0]=1;}
    wire=retained;retained[4][0]=wire[4][0]=5;
    Check(prove(),"aim may precede remaining actual states");wire[37][8]^=1;Check(!prove(),"changed state after aim rejected");
    Check(!SameReplacementFrame(retained[0].data(),wire[0].data(),first,37,38),"truncated suffix rejected");
    Check(!SameReplacementFrame(nullptr,wire[0].data(),first,38,38),"unreadable suffix rejected");
    std::printf("%u actual checkpoint production checks passed\n",checks);
}
'''
        self.assertIn('production checks passed', compile_and_run(cpp))

    def test_native_replica_retirement_preserves_identity_and_class_guards(self):
        source = (ROOT / 'native/local_server_abilities.h').read_text(encoding='utf-8-sig')
        bodies = '\n'.join(function(source, signature) for signature in (
            'inline bool RetireClientReplicaGraph(', 'inline bool AdoptClient('))
        cpp = PRELUDE + r'''
using Weak=int;
constexpr size_t SkillCount=4;
struct Skill{void*type=nullptr;void*actor=nullptr;void*slot=nullptr;uint8_t slotType=255;Weak identity=0;bool ready=false,adopted=false;};
struct FireState{void*object=nullptr;Weak identity=0;};
struct Player{bool authority=false,grantComplete=false,ready=false,startingStatesReady=false,primingReady=false,daggerBound=false;
    bool readyAtDiagnostic=false,ultimateQuery=false,ultimateReady=false;
    void*inventory=nullptr;std::array<Skill,4>skills{};std::array<FireState,2>fireStates{};std::array<int,4>commits{};size_t commitCursor=0;int priming=0,daggerObserved=0,glideObserved=0;const char*waiting="none";};
struct{unsigned error=0;const char*stage="none";uint64_t replicaGraphRetirements=0,replicaCaptureFailures=0,replicaResolveDeferrals=0;
    void*lastReplicaIdentityActor=nullptr;Weak lastReplicaIdentity=0;uint32_t lastReplicaIdentitySkill=0;bool lastReplicaCaptureSucceeded=false;}state;
void* expired=nullptr;void*uncapturable=nullptr;unsigned restores=0;bool restoreAllowed=true;
bool Live(void*p,Weak){return p&&p!=expired;}
namespace original_inventory_slots{bool CaptureIdentity(void*p,Weak&id){id=1;return p&&p!=uncapturable;}}
bool RestoreObserver(void*,Weak){++restores;return restoreAllowed;}
bool Fail(unsigned code,const char*why){state.error=code;state.stage=why;return false;}
namespace original_players{bool Derived(void*actual,void*expected){return actual==expected;}}
namespace original_jett{const wchar_t*SkillPaths[4]{};void*LoadEquippableClass(const wchar_t*){return nullptr;}}
namespace engine{void*DefaultObject(void*p){return p;}}
''' + bodies + r'''
int main(){
    std::array<uint8_t,0x300>inventory{};std::array<std::array<uint8_t,0x80>,4>slots{},actors{};int type=1;
    Player p;p.inventory=inventory.data();
    for(size_t n=0;n<4;++n){p.skills[n].type=&type;p.skills[n].slotType=static_cast<uint8_t>(n);
        Put<void*>(inventory.data(),0x1b8+n*8,slots[n].data());Put<void*>(slots[n].data(),0x38,actors[n].data());Put<void*>(actors[n].data(),0x10,&type);}
    Check(AdoptClient(p)&&p.grantComplete,"live native graph adopted");
    p.ready=p.daggerBound=p.primingReady=true;p.fireStates[0]={actors[0].data(),1};p.fireStates[1]={actors[1].data(),1};
    expired=actors[3].data();Check(AdoptClient(p)&&!p.grantComplete&&!p.ready,"pending kill slot waits");
    Check(restores==3&&!p.daggerBound&&!p.primingReady&&!p.skills[3].actor,"old graph observers retired before stale slot sampling");
    Check(!state.error,"normal native retirement does not become fatal");
    Check(state.replicaGraphRetirements==1&&state.replicaResolveDeferrals==1&&state.lastReplicaCaptureSucceeded,"unresolved weak exported separately");
    expired=nullptr;Check(AdoptClient(p)&&p.grantComplete,"live replica graph can be readopted");
    std::array<uint8_t,0x80>replacement{};Put<void*>(replacement.data(),0x10,&type);Put<void*>(slots[0].data(),0x38,replacement.data());
    Check(!AdoptClient(p)&&state.error==4457,"live replica replacement still requires retirement");
    state.error=0;Put<void*>(slots[0].data(),0x38,actors[0].data());Put<void*>(actors[0].data(),0x10,nullptr);
    Check(!AdoptClient(p)&&state.error==4456,"wrong native actor class stays fatal");
    Put<void*>(actors[0].data(),0x10,&type);state.error=0;uncapturable=actors[0].data();
    Check(!AdoptClient(p)&&state.error==4458,"native identity capture failure stays fatal");
    Check(state.replicaCaptureFailures==1&&!state.lastReplicaCaptureSucceeded,"capture failure exported separately");
    uncapturable=nullptr;state.error=0;expired=actors[3].data();restoreAllowed=false;
    Check(!AdoptClient(p),"observer restore failure propagates");
    std::printf("%u production replica adoption checks passed\n",checks);
}
'''
        self.assertIn('production replica adoption checks passed', compile_and_run(cpp))


if __name__ == '__main__':
    unittest.main()
