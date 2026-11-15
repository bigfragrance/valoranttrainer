#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>

// Pure admission/evidence policy. No engine, filesystem, process or clock API.
namespace empty_bootstrap_probe_core {
inline constexpr uint64_t Magic=0x314250454d505459ULL;
inline constexpr uint32_t Version=1,MarkerBytes=2096,MaxPolls=600,ExpectedEpoch=1;
inline constexpr uint32_t ObservationIntervalMs=25;
// Diagnostic wall-monotonic cadence only. It cannot change game clocks,
// admission/readiness, native input/Apply frequency or funded work.
struct ObservationCadence {
    bool sampled=false;uint64_t lastMs=0;
    bool Due(uint64_t now){
        if(sampled&&(now<lastMs||now-lastMs<ObservationIntervalMs))return false;
        sampled=true;lastMs=now;return true;
    }
};
#pragma pack(push,1)
struct Marker {
    uint64_t magic=0;
    uint32_t version=0,bytes=0,role=0,pid=0,maxPolls=0,expectedEpoch=0;
    uint8_t nonce[16]{};
    char16_t image[512]{},pair[512]{};
};
#pragma pack(pop)
static_assert(sizeof(Marker)==MarkerBytes&&offsetof(Marker,nonce)==32&&offsetof(Marker,image)==48);
inline char16_t Lower(char16_t c){return c>=u'A'&&c<=u'Z'?char16_t(c+(u'a'-u'A')):c;}
inline bool EqualPath(const std::u16string& a,const std::u16string& b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(Lower(a[i])!=Lower(b[i]))return false;
    return true;
}
inline bool PaddedString(const char16_t* s,size_t cap,std::u16string& out){
    size_t end=0;while(end<cap&&s[end])++end;
    if(end==0||end==cap)return false;
    for(size_t i=end+1;i<cap;++i)if(s[i])return false;
    out.assign(s,end);return true;
}
inline bool PairPath(const std::u16string& pair){
    const std::u16string prefix=u"F:\\valoranttrainer\\.local\\local-server\\pair";
    if(pair.size()<=prefix.size()||!EqualPath(pair.substr(0,prefix.size()),prefix))return false;
    for(size_t i=prefix.size();i<pair.size();++i){const auto c=pair[i];
        if(!((c>=u'a'&&c<=u'z')||(c>=u'A'&&c<=u'Z')||(c>=u'0'&&c<=u'9')||c==u'-'||c==u'_'||c==u'.'))return false;
    }
    return pair.back()!=u'.'&&pair.find(u"..",prefix.size())==std::u16string::npos;
}
// Only the configuration role-directory argument has a producer-added tail.
// Do not normalize marker paths, embedded pair/image values or Admit inputs.
inline bool NormalizeRoleDirectory(const std::u16string& full,uint32_t role,std::u16string& out){
    out.clear();if(role!=1&&role!=2)return false;
    size_t end=full.size();while(end>3&&full[end-1]==u'\\')--end;
    const auto value=full.substr(0,end);const auto slash=value.find_last_of(u'\\');
    if(slash==std::u16string::npos||!PairPath(value.substr(0,slash))
       ||!EqualPath(value.substr(slash+1),role==1?u"host":u"client"))return false;
    out=value;return true;
}
inline int Hex(char16_t c){if(c>=u'0'&&c<=u'9')return c-u'0';c=Lower(c);return c>=u'a'&&c<=u'f'?c-u'a'+10:-1;}
inline bool ParseNonce(const std::u16string& text,uint8_t (&out)[16]){
    if(text.size()!=32)return false;
    bool nonzero=false;for(size_t i=0;i<16;++i){const int a=Hex(text[i*2]),b=Hex(text[i*2+1]);
        if(a<0||b<0)return false;out[i]=uint8_t((a<<4)|b);nonzero|=out[i]!=0;}
    return nonzero;
}
enum class Admission {Off,Armed,InvalidSize,InvalidHeader,InvalidRole,InvalidPid,InvalidNonce,InvalidPath,InvalidImage,InvalidPair,InvalidDirectory,InvalidMarkerPath};
inline const char* Name(Admission a){switch(a){
    case Admission::Off:return "disabled_no_explicit_marker";case Admission::Armed:return "explicit_owned_marker_armed";
    case Admission::InvalidSize:return "marker_exact_size_rejected";case Admission::InvalidHeader:return "marker_header_rejected";
    case Admission::InvalidRole:return "marker_role_rejected";case Admission::InvalidPid:return "marker_current_game_pid_rejected";
    case Admission::InvalidNonce:return "marker_nonce_rejected";case Admission::InvalidPath:return "marker_string_padding_rejected";
    case Admission::InvalidImage:return "marker_current_image_rejected";case Admission::InvalidPair:return "marker_pair_path_rejected";
    case Admission::InvalidDirectory:return "marker_current_role_directory_rejected";case Admission::InvalidMarkerPath:return "marker_exact_path_rejected";
    }return "marker_unknown_rejection";}
inline Admission Admit(const Marker& m,size_t bytes,uint32_t role,uint32_t pid,const uint8_t (&nonce)[16],
    const std::u16string& image,const std::u16string& directory,const std::u16string& markerPath){
    if(bytes!=MarkerBytes)return Admission::InvalidSize;
    if(m.magic!=Magic||m.version!=Version||m.bytes!=MarkerBytes||m.maxPolls!=MaxPolls||m.expectedEpoch!=ExpectedEpoch)return Admission::InvalidHeader;
    if((role!=1&&role!=2)||m.role!=role)return Admission::InvalidRole;
    if(!pid||m.pid!=pid)return Admission::InvalidPid;
    if(std::memcmp(m.nonce,nonce,16)!=0)return Admission::InvalidNonce;
    std::u16string mi,mp;if(!PaddedString(m.image,512,mi)||!PaddedString(m.pair,512,mp))return Admission::InvalidPath;
    const std::u16string copy=u"F:\\valoranttrainer\\client\\ShooterGame\\Binaries\\Win64\\VALORANT-Win64-Shipping.exe";
    if(!EqualPath(mi,image)||!EqualPath(image,copy))return Admission::InvalidImage;
    if(!PairPath(mp))return Admission::InvalidPair;
    const auto expected=mp+(role==1?u"\\host":u"\\client");
    if(!EqualPath(directory,expected))return Admission::InvalidDirectory;
    if(!EqualPath(markerPath,expected+u"\\empty-equipment-bootstrap.bin"))return Admission::InvalidMarkerPath;
    return Admission::Armed;
}
struct Scope {
    bool armed=false,enabled=false,host=false,freeForAll=false,human=false,linked=false,weakVerified=false,baselineRegistered=false;
    uint32_t role=0,index=0,epoch=0;
    uintptr_t world=0,pc=0,ps=0,pawn=0,connection=0,inventory=0;
};
inline bool Hold(const Scope& s){
    return s.armed&&s.role==1&&s.enabled&&s.host&&s.freeForAll&&s.human&&s.index==0&&s.epoch==ExpectedEpoch
        &&s.linked&&s.weakVerified&&s.baselineRegistered&&s.world&&s.pc&&s.ps&&s.pawn&&s.connection&&s.inventory;
}
struct HostEvidence {
    bool installed=false,owned=false,firstBatch=false,empty=false,currentPrediction=false;
    uint32_t epoch=0,error=0,clockIndex=0,consumed=0,clockStep=0;
    uint64_t received=0,accepted=0,applyCalls=0,applyReturns=0,physicsCompleted=0;
};
inline bool HostWitness(const HostEvidence& e){
    return e.installed&&e.owned&&e.firstBatch&&e.empty&&e.currentPrediction&&e.epoch==ExpectedEpoch&&!e.error
        &&e.received>0&&e.accepted>0&&e.applyCalls>0&&e.applyReturns>0&&e.applyReturns<=e.applyCalls
        &&e.physicsCompleted>0&&e.clockIndex>0&&e.clockStep==1&&e.clockIndex==e.consumed;
}
struct ClientEvidence {
    bool ready=false,empty=false,currentPrediction=false,localOwner=false,observed=false,roundtrip=false;
    uint64_t sourceMask=0;
};
inline bool ClientWitness(const ClientEvidence& e){return e.ready&&e.sourceMask==1023&&e.empty&&e.currentPrediction&&e.localOwner&&e.observed&&e.roundtrip;}
} // namespace empty_bootstrap_probe_core
