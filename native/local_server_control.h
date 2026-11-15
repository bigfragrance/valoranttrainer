#pragma once
#include "training_control.h"
#include "bot_spectator_contract.h"
#include "armor_poll_performance.h"
#include "human_bot_initial.h"
#include "local_server_bot_roster_initial.h"
#include "local_server_match.h"
#include "local_server_owned_paths.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

// V2 transport/lifecycle shim only. The parent supplies all engine adaptation.
// Opening the PID mapping or accepting an Initial is never gameplay readiness.
namespace local_server_control {
using Command=training_control::Command;
using Result=training_control::Result;
struct Hooks {
    training_control::Dispatch dispatch=nullptr;
    std::string (*stateJson)()=nullptr;
    void (*requestStop)()=nullptr;
};
struct State {
    bool loaded=false,initialPresent=false,enabled=false,initialized=false,host=false;
    bool ready=false,parentStateValid=false,parentError=false,stopRequested=false,closed=false;
    unsigned error=0;DWORD gameThread=0;
    uint32_t appliedRevision=0,flags=0,lastCommand=0,lastResult=static_cast<uint32_t>(Result::Unavailable);
    uint64_t polls=0,commands=0,publications=0,lastPublish=0,publishedCommands=0;
    const char* stage="local_server_control_not_loaded";
};
inline State state;
inline Hooks hooks;
inline training_control::Initial initial{};
inline std::wstring outputDirectory,initialPath;
static_assert(sizeof(training_control::Initial)==7844,"V2 Initial wire layout changed");
inline const State& GetState(){return state;}
inline const training_control::Initial& Initial(){return initial;}
inline bool Enabled(){return state.loaded&&state.enabled&&!state.closed;}
inline bool Thread(){return state.initialized&&!state.closed&&state.gameThread==GetCurrentThreadId();}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;state.ready=false;state.flags=0;return false;}

inline bool InitialLocation(const std::wstring& output,bool host,std::wstring& canonical,std::wstring& sidecar){
    // Resolve only this loaded package's own pair/role on any local drive.
    if(!local_server_owned_paths::CanonicalAbsolute(output,canonical)
       ||!local_server_owned_paths::RoleDirectory(canonical,host?1U:2U))return false;
    const auto roleAt=canonical.find_last_of(L"\\/");
    if(roleAt==std::wstring::npos||canonical.substr(roleAt+1)!=(host?L"host":L"client"))return false;
    const auto parent=canonical.substr(0,roleAt);
    const auto pairAt=parent.find_last_of(L"\\/");
    const auto pair=parent.substr(pairAt==std::wstring::npos?0:pairAt+1);
    if(pair.size()<=5||pair.compare(0,5,L"pair-")!=0)return false;
    sidecar=parent+L"\\session-initial.bin";return true;
}

inline bool LoadInitial(const std::wstring& output,bool host){
    std::wstring canonical,path;
    if(!InitialLocation(output,host,canonical,path))return Fail(4700,"local_server_control_initial_path_invalid");
    bool rosterRequired=false;
    if(host&&!local_server_bot_roster_wire::RequiredArgument(GetCommandLineW(),rosterRequired))
        return Fail(9029,"fixed_bot_roster_required_cli_invalid");
    if(state.loaded)return state.host==host&&outputDirectory==canonical;
    if(state.initialized||state.error)return false;
    outputDirectory=canonical;initialPath=path;state.host=host;
    const auto file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){
        if(GetLastError()!=ERROR_FILE_NOT_FOUND)return Fail(4701,"local_server_control_initial_open_failed");
        if(host&&rosterRequired)return Fail(9034,"fixed_bot_roster_required_initial_missing");
        state.loaded=true;state.enabled=false;state.stage="local_server_control_basic_lifecycle_probe";return true;
    }
    state.initialPresent=true;
    training_control::Initial candidate{};LARGE_INTEGER bytes{};DWORD received=0;
    const bool exact=GetFileSizeEx(file,&bytes)&&bytes.QuadPart==static_cast<LONGLONG>(sizeof(candidate))
        &&ReadFile(file,&candidate,static_cast<DWORD>(sizeof(candidate)),&received,nullptr)&&received==sizeof(candidate);
    CloseHandle(file);
    if(!exact||candidate.magic!=training_control::Magic||candidate.version!=training_control::Version
       ||candidate.bytes!=sizeof(candidate)||!local_server_match::ValidInitial(candidate))
        return Fail(4702,"local_server_control_initial_configuration_invalid");
    if(host&&!local_server_bot_roster_initial::Load(path,&candidate,sizeof(candidate),candidate.config.totalPlayers,candidate.config.revision,candidate.config.seed,rosterRequired))
        return Fail(local_server_bot_roster_initial::GetState().error,local_server_bot_roster_initial::GetState().stage);
    if(host&&!human_bot_initial::Load(path,candidate.config.totalPlayers,candidate.config.revision,candidate.config.seed))
        return Fail(4710,human_bot_initial::result.stage);
    initial=candidate;state.loaded=true;state.enabled=true;
    state.stage="local_server_control_waiting_parent_world";return true;
}

struct Publication {bool ready=false,error=false;uint32_t revision=0;};
namespace detail {
// Parse the parent JSON structurally: a nested weapon.ready cannot promote the
// session. Unknown fields are validated/skipped, not interpreted as features.
struct Token {size_t begin=0,end=0;char kind=0;};
class JsonReader {
    const std::string& text;size_t at=0;
    void Space(){while(at<text.size()&&(text[at]==' '||text[at]=='\t'||text[at]=='\r'||text[at]=='\n'))++at;}
    bool Take(char c){Space();if(at>=text.size()||text[at]!=c)return false;++at;return true;}
    static int Hex(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
    bool String(std::string* value=nullptr){
        if(at>=text.size()||text[at++]!='"')return false;
        while(at<text.size()){
            unsigned char c=static_cast<unsigned char>(text[at++]);
            if(c=='"')return true;if(c<32)return false;
            if(c=='\\'){
                if(at>=text.size())return false;const char escaped=text[at++];
                if(escaped=='u'){
                    unsigned code=0;for(unsigned n=0;n<4;++n){if(at>=text.size())return false;const int digit=Hex(text[at++]);if(digit<0)return false;code=(code<<4)|static_cast<unsigned>(digit);}
                    if(value)value->push_back(code<128?static_cast<char>(code):'?');continue;
                }
                switch(escaped){case '"':case '\\':case '/':c=static_cast<unsigned char>(escaped);break;
                    case 'b':c='\b';break;case 'f':c='\f';break;case 'n':c='\n';break;case 'r':c='\r';break;case 't':c='\t';break;default:return false;}
            }
            if(value)value->push_back(static_cast<char>(c));
        }return false;
    }
    bool Number(){
        if(at<text.size()&&text[at]=='-')++at;if(at>=text.size())return false;
        if(text[at]=='0')++at;
        else {if(text[at]<'1'||text[at]>'9')return false;while(at<text.size()&&text[at]>='0'&&text[at]<='9')++at;}
        if(at<text.size()&&text[at]=='.'){++at;const auto start=at;while(at<text.size()&&text[at]>='0'&&text[at]<='9')++at;if(at==start)return false;}
        if(at<text.size()&&(text[at]=='e'||text[at]=='E')){++at;if(at<text.size()&&(text[at]=='+'||text[at]=='-'))++at;const auto start=at;while(at<text.size()&&text[at]>='0'&&text[at]<='9')++at;if(at==start)return false;}
        return true;
    }
    bool Value(unsigned depth,Token& token){
        Space();if(at>=text.size()||depth>32)return false;token.begin=at;token.kind=text[at];
        if(text[at]=='{'){if(!Object(depth,nullptr))return false;}
        else if(text[at]=='['){++at;Space();if(at<text.size()&&text[at]==']')++at;
            else {for(;;){Token ignored;if(!Value(depth+1,ignored))return false;Space();if(at<text.size()&&text[at]==']'){++at;break;}if(!Take(','))return false;}}}
        else if(text[at]=='"'){if(!String())return false;}
        else if(text.compare(at,4,"true")==0)at+=4;
        else if(text.compare(at,5,"false")==0)at+=5;
        else if(text.compare(at,4,"null")==0)at+=4;
        else if(!Number())return false;
        token.end=at;return true;
    }
    bool Unsigned(const Token& token,uint32_t& value)const{
        if(token.begin==token.end)return false;uint32_t parsed=0;
        for(size_t n=token.begin;n<token.end;++n){const char c=text[n];if(c<'0'||c>'9')return false;
            const uint32_t digit=static_cast<uint32_t>(c-'0');if(parsed>(UINT32_MAX-digit)/10U)return false;parsed=parsed*10U+digit;}
        value=parsed;return true;
    }
    bool Object(unsigned depth,Publication* output){
        if(!Take('{'))return false;Space();unsigned seen=0;
        if(at<text.size()&&text[at]=='}'){++at;return !output;}
        for(;;){
            Space();std::string key;if(!String(&key)||!Take(':'))return false;
            Token token;if(!Value(depth+1,token))return false;
            if(output){
                if(key=="ready"){
                    if(seen&1U)return false;seen|=1U;
                    const auto length=token.end-token.begin;
                    if(length==4&&text.compare(token.begin,length,"true")==0)output->ready=true;
                    else if(length==5&&text.compare(token.begin,length,"false")==0)output->ready=false;else return false;
                }else if(key=="applied_revision"){
                    if(seen&2U||!Unsigned(token,output->revision))return false;seen|=2U;
                }else if(key=="error"){
                    if(seen&4U)return false;seen|=4U;
                    if(token.kind=='"')output->error=token.end-token.begin>2;
                    else if(text.compare(token.begin,token.end-token.begin,"null")!=0)return false;
                }
            }
            Space();if(at<text.size()&&text[at]=='}'){++at;break;}if(!Take(','))return false;
        }return !output||(seen==7U&&(!output->ready||!output->error));
    }
public:
    explicit JsonReader(const std::string& source):text(source){}
    bool Read(Publication& output){if(text.empty()||text.size()>=training_control::StateCapacity)return false;
        if(!Object(0,&output))return false;Space();return at==text.size();}
};
} // namespace detail

inline std::string WaitingJson(const char* stage,bool failure=false){
    // All stage strings are internal constants; no command/file text is echoed.
    std::string json="{\"ready\":false,\"stage\":\"";json+=stage;
    json+="\",\"error\":";if(failure){json+='"';json+=stage;json+='"';}else json+="null";
    json+=",\"applied_revision\":0,\"map_id\":\"\",\"players\":[],\"routes\":[],\"effects\":[],\"weapon\":{\"ready\":false},\"playable_verified\":false}";
    return json;
}

inline bool PublishWaiting(const char* stage,bool failure=false){
    state.ready=false;state.flags=0;state.appliedRevision=0;state.parentStateValid=false;
    const auto json=WaitingJson(stage,failure);
    if(!training_control::Publish(json.data(),json.size(),0,0))return Fail(4703,"local_server_control_state_publish_failed");
    state.lastPublish=GetTickCount64();++state.publications;return true;
}

inline bool Initialize(const Hooks& callbacks={}){
    if(state.initialized)return Thread()&&hooks.dispatch==callbacks.dispatch&&hooks.stateJson==callbacks.stateJson&&hooks.requestStop==callbacks.requestStop;
    if(!state.loaded||state.closed||state.error)return false;
    if(state.enabled&&(!callbacks.dispatch||!callbacks.stateJson||!callbacks.requestStop))
        return Fail(4704,"local_server_control_parent_hooks_required");
    if(training_control::memory)return Fail(4705,"local_server_control_mapping_already_owned");
    if(!training_control::Initialize())return Fail(4706,"local_server_control_channel_unavailable");
    training_control::lastCommand=0;hooks=callbacks;state.gameThread=GetCurrentThreadId();state.initialized=true;
    if(state.host&&state.enabled&&!human_bot_store::BindGameThread(state.gameThread))
        return Fail(4711,"local_server_control_human_bot_game_thread_bind_failed");
    return PublishWaiting(state.enabled?"local_server_control_waiting_parent_world":"local_server_control_basic_lifecycle_probe");
}

inline bool ClientUI(Command command){
    return command==Command::ToggleBuyMenu||command==Command::ToggleOptions||command==Command::CloseActiveUI;
}
inline bool ValidPayload(Command command,const uint8_t* data,uint32_t count){
    if(count>training_control::CommandCapacity||(count&&!data))return false;
    switch(command){
        case Command::HumanBots:return human_bot_wire::ValidCommand(data,count);
        case Command::SpectateBot:{bot_spectator_contract::Request request{};return bot_spectator_contract::Decode(data,count,request);}
        case Command::Configure:{training_control::Configuration value{};if(count!=sizeof(value))return false;std::memcpy(&value,data,sizeof(value));return local_server_match::Valid(value);}
        case Command::Routes:{
            if(count<8)return false;uint32_t routes=0;std::memcpy(&routes,data,4);
            if(routes>128||(count!=8+routes*sizeof(training_control::Route)&&count!=sizeof(training_control::RouteSet)))return false;
            training_control::RouteSet value{};std::memcpy(&value,data,count);return local_server_match::ValidRoutes(value);}
        case Command::Weapon:case Command::BuyWeapon:return count==sizeof(training_control::Guid);
        case Command::Skin:case Command::SaveSkinSelection:return count==sizeof(training_control::Skin);
        case Command::Effect:{training_control::Effect value{};if(count!=sizeof(value))return false;std::memcpy(&value,data,sizeof(value));
            return value.key[0]&&value.key[79]==0&&value.mode<=2&&std::isfinite(value.value);}
        case Command::ResolveMapPoint:{training_control::Point value{};if(count!=sizeof(value))return false;std::memcpy(&value,data,sizeof(value));
            return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z)
                &&std::abs(value.x)<=1000000&&std::abs(value.y)<=1000000&&std::abs(value.z)<=1000000;}
        case Command::TestPlayerNearBot:case Command::TestAudioRegistration:return count==sizeof(uint32_t);
        case Command::RestartMatch:case Command::ValidateRoutes:case Command::ValidateSkinCatalog:
        case Command::ToggleBuyMenu:case Command::ToggleOptions:case Command::CloseActiveUI:return count==0;
        default:return false;
    }
}

inline Result Dispatch(Command command,const uint8_t* data,uint32_t count,char* message,size_t capacity){
    const auto reply=[&](Result result,const char* text){if(message&&capacity)strncpy_s(message,capacity,text,_TRUNCATE);state.lastResult=static_cast<uint32_t>(result);return result;};
    if(!Thread())return reply(Result::Unavailable,"local_server_control_game_thread_required");
    ++state.commands;state.lastCommand=static_cast<uint32_t>(command);
    if(!Enabled())return reply(Result::Unavailable,"local_server_control_basic_probe_has_no_match_configuration");
    if(state.stopRequested||state.error)return reply(Result::Unavailable,"local_server_control_stopping_or_failed");
    if(!ValidPayload(command,data,count))return reply(Result::Invalid,"local_server_control_invalid_command_payload");
    if(state.host==ClientUI(command))return reply(Result::Unavailable,"local_server_control_command_requires_other_role");
    if(!hooks.dispatch)return reply(Result::Unavailable,"local_server_control_parent_dispatch_unavailable");
    Result result=Result::Failed;
    try {result=hooks.dispatch(command,data,count,message,capacity);}
    catch(...){return reply(Result::Failed,"local_server_control_parent_dispatch_exception");}
    if(message&&capacity)message[capacity-1]=0;
    if(static_cast<uint32_t>(result)>static_cast<uint32_t>(Result::Failed))return reply(Result::Failed,"local_server_control_parent_result_invalid");
    state.lastResult=static_cast<uint32_t>(result);return result;
}

inline bool PublishParent(){
    if(!Thread())return false;
    // GT product JSON, contract validation and MMF copy; not worker file I/O.
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::ControlPublish);
    if(!Enabled()||state.stopRequested||state.error)
        return PublishWaiting(state.error?state.stage:state.stopRequested?"local_server_control_stop_requested":"local_server_control_basic_lifecycle_probe",state.error!=0);
    if(!hooks.stateJson)return Fail(4704,"local_server_control_parent_hooks_required");
    std::string json;
    try {json=hooks.stateJson();}
    catch(...){state.stage="local_server_control_parent_state_exception";return PublishWaiting(state.stage,true);}
    Publication publication;
    detail::JsonReader reader(json);
    if(!reader.Read(publication)){
        state.stage="local_server_control_parent_state_contract_invalid";
        // A rejected snapshot cannot retain a previous ready flag/revision.
        return PublishWaiting(state.stage,true);
    }
    state.ready=publication.ready;state.parentError=publication.error;state.parentStateValid=true;
    state.appliedRevision=publication.revision;state.flags=state.ready?1U:0U;
    state.stage=state.parentError?"local_server_control_parent_reported_error":state.ready?"local_server_control_parent_ready":"local_server_control_waiting_parent_world";
    if(!training_control::Publish(json.data(),json.size(),state.appliedRevision,state.flags))return Fail(4703,"local_server_control_state_publish_failed");
    state.lastPublish=GetTickCount64();++state.publications;return true;
}

inline bool Poll(){
    if(!Thread())return Fail(4707,"local_server_control_game_thread_mismatch");
    ++state.polls;
    if(state.host&&state.enabled)human_bot_store::Poll();
    // Existing V2 sequence/acknowledgement ownership consumes each command once,
    // including Pending/Unavailable. A queued parent action is never replayed.
    training_control::Poll(Dispatch);
    const auto now=GetTickCount64();
    if(now-state.lastPublish<100&&state.publishedCommands==state.commands)return true;
    const bool published=PublishParent();state.publishedCommands=state.commands;return published;
}

inline bool RequestStop(){
    if(!Thread())return false;if(state.stopRequested)return true;
    state.stopRequested=true;state.stage="local_server_control_stop_requested";
    const bool published=PublishWaiting(state.stage);
    if(hooks.requestStop)hooks.requestStop();return published;
}
inline void Close(){
    if(state.host)human_bot_store::Close();
    if(state.initialized)training_control::Close();state.ready=false;state.flags=0;state.initialized=false;
    state.closed=true;state.stage="local_server_control_closed";
}
} // namespace local_server_control
