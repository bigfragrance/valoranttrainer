#pragma once
#include "engine_api.h"
#include "original_players.h"
#include "training_control.h"
#include <vector>
#include <map>
#include <set>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

// Read-only original navigation queries, admitted only by the explicit stage9
// caller. No movement, teleport, actor state, capsule or navmesh mutation.
namespace sound_locator_nav_export {
using Vec=engine::Vec;
struct Request {uint32_t operation=3,budget=64;double spacing=200;};
static_assert(sizeof(Request)==16);
struct State {
 bool started=false,complete=false;unsigned phase=0;double spacing=200;
 int nx=0,ny=0;size_t projectionAt=0,pairAt=0,projectionCalls=0,pathCalls=0,pathRejected=0;
 void* world=nullptr;void* pawn=nullptr;
 std::vector<Vec> nodes;std::vector<std::pair<size_t,size_t>> pairs;
 std::map<std::pair<int,int>,std::vector<size_t>> cells;
 std::vector<std::vector<Vec>> paths;
};
inline State state;
inline constexpr double MinX=-6118,MaxX=8218,MinY=-11668,MaxY=2668;
inline constexpr std::array<double,7> Heights{-100,100,300,500,700,1000,1400};
inline double Distance(const Vec&a,const Vec&b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params){
 auto fn=engine::Find(path);return object&&fn&&original_players::Value<uint16_t>(fn,0xbe)==bytes&&engine::Event(object,path,params);
}
inline bool Project(const Vec& point,Vec& projected){
 struct Query {void* world;Vec point;Vec projected{};void* nav=nullptr;void* filter=nullptr;Vec extent{};uint8_t result=0,pad[7]{};} q{state.world,point};
 static_assert(sizeof(Query)==104&&offsetof(Query,result)==96);
 q.extent={state.spacing*.48,state.spacing*.48,160};
 if(!Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.K2_ProjectPointToNavigation",97,&q)
    ||!q.result||!original_players::Finite(q.projected))return false;
 if(std::abs(q.projected.x-point.x)>q.extent.x+1||std::abs(q.projected.y-point.y)>q.extent.y+1||std::abs(q.projected.z-point.z)>161)return false;
 projected=q.projected;return true;
}
inline bool Path(const Vec&from,const Vec&to,std::vector<Vec>&out){
 struct Query {void* world;Vec from,to;void* pawn;void* filter=nullptr;void* result=nullptr;} q{state.world,from,to,state.pawn};
 static_assert(sizeof(Query)==80&&offsetof(Query,result)==72);
 if(!Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.FindPathToLocationSynchronously",80,&q)
    ||!original_players::IsA(q.result,0x4f80dd0))return false;
 uint8_t valid=0,partial=1;auto points=original_players::Value<Vec*>(q.result,0x40);
 auto count=original_players::Value<int32_t>(q.result,0x48),capacity=original_players::Value<int32_t>(q.result,0x4c);
 if(!Event(q.result,L"/Script/NavigationSystem.NavigationPath.IsValid",1,&valid)
    ||!Event(q.result,L"/Script/NavigationSystem.NavigationPath.IsPartial",1,&partial)||!valid||partial
    ||count<2||count>64||capacity<count||!original_players::Readable(points,size_t(count)*sizeof(Vec)))return false;
 double length=0;
 for(int i=0;i<count;++i){if(!original_players::Finite(points[i]))return false;if(i)length+=Distance(points[i-1],points[i]);}
 if(Distance(points[0],from)>30||Distance(points[count-1],to)>30)return false;
 // Do not replace a long detour with a direct neighbor edge. The complete path
 // chain is retained; this cap only bounds local sampling, not reachability.
 if(length>Distance(from,to)*3+100)return false;
 out.assign(points,points+count);return true;
}
inline bool Write(){
 HMODULE module=nullptr;wchar_t filename[32768]{},temporary[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&Write),&module)
    ||!GetModuleFileNameW(module,filename,32768))return false;
 auto slash=wcsrchr(filename,L'\\');if(!slash)return false;
 wcscpy_s(slash+1,32768-size_t(slash+1-filename),L"ascent-nav-export.json");
 swprintf_s(temporary,L"%s.tmp",filename);FILE*f=nullptr;if(_wfopen_s(&f,temporary,L"wb")||!f)return false;
 fprintf(f,"{\"schema_version\":1,\"map_id\":\"ascent\",\"pid\":%lu,\"complete\":%s,\"phase\":%u,\"spacing_cm\":%.3f,\"projection_calls\":%zu,\"path_calls\":%zu,\"path_rejected\":%zu,\"sampling_heights_cm\":[-100,100,300,500,700,1000,1400],\"projection_extent_cm\":[%.3f,%.3f,160],\"nodes_world_cm\":[",GetCurrentProcessId(),state.complete?"true":"false",state.phase,state.spacing,state.projectionCalls,state.pathCalls,state.pathRejected,state.spacing*.48,state.spacing*.48);
 for(size_t i=0;i<state.nodes.size();++i){auto&p=state.nodes[i];fprintf(f,"%s[%.6f,%.6f,%.6f]",i?",":"",p.x,p.y,p.z);}
 fputs("],\"path_chains_cm\":[",f);
 for(size_t i=0;i<state.paths.size();++i){if(i)fputc(',',f);fputc('[',f);for(size_t j=0;j<state.paths[i].size();++j){auto&p=state.paths[i][j];fprintf(f,"%s[%.6f,%.6f,%.6f]",j?",":"",p.x,p.y,p.z);}fputc(']',f);}
 fputs("],\"scope\":\"Original ProjectPointToNavigation and complete valid nonpartial FindPath queries for current pawn; sampled coverage, no full collision-mesh claim\"}",f);
 if(fclose(f)!=0)return false;return MoveFileExW(temporary,filename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}
inline training_control::Result DispatchImpl(const uint8_t*data,uint32_t bytes,char*message,size_t capacity,void*world,void*pawn,const char*mapId){
 using Result=training_control::Result;
 auto fail=[&](const char*s){snprintf(message,capacity,"%s",s);return Result::Failed;};
 Request r;if(bytes!=sizeof(r))return fail("nav_export_request_size");std::memcpy(&r,data,sizeof(r));
 if(!original_players::Thread()||!world||!pawn||!mapId||std::strcmp(mapId,"ascent")||r.operation>3||r.budget<1||r.budget>256)
  return fail("nav_export_requires_ascent_game_thread");
 if(r.operation==0){
  if(!std::isfinite(r.spacing)||r.spacing<100||r.spacing>400)return fail("nav_export_spacing_100_to_400_cm");
  state=State{};state.started=true;state.world=world;state.pawn=pawn;state.spacing=r.spacing;
  state.nx=int(std::floor((MaxX-MinX)/r.spacing))+1;state.ny=int(std::floor((MaxY-MinY)/r.spacing))+1;
 }
 if(!state.started||state.world!=world||state.pawn!=pawn)return fail("nav_export_begin_or_identity_required");
 if(r.operation==1&&!state.complete){
  auto started=GetTickCount64();
  for(unsigned work=0;work<r.budget&&GetTickCount64()-started<8;++work){
   if(state.phase==0){
    const size_t total=size_t(state.nx)*state.ny*Heights.size();
    if(state.projectionAt>=total){
     for(const auto& cell:state.cells)for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy){
      auto adjacentCell=state.cells.find({cell.first.first+dx,cell.first.second+dy});if(adjacentCell==state.cells.end())continue;
      for(auto a:cell.second)for(auto b:adjacentCell->second)if(a<b&&std::abs(state.nodes[a].z-state.nodes[b].z)<=300)state.pairs.push_back({a,b});
     }
     std::sort(state.pairs.begin(),state.pairs.end());state.pairs.erase(std::unique(state.pairs.begin(),state.pairs.end()),state.pairs.end());state.phase=1;continue;
    }
    size_t index=state.projectionAt++;int h=int(index%Heights.size());index/=Heights.size();int ix=int(index/state.ny),iy=int(index%state.ny);
    Vec from{MinX+ix*state.spacing,MinY+iy*state.spacing,Heights[h]},projected{};++state.projectionCalls;
    if(!Project(from,projected))continue;
    auto& cell=state.cells[{ix,iy}];bool duplicate=false;
    for(auto n:cell)if(Distance(state.nodes[n],projected)<5){duplicate=true;break;}
    if(!duplicate){cell.push_back(state.nodes.size());state.nodes.push_back(projected);}
   }else{
    if(state.pairAt>=state.pairs.size()){state.complete=true;state.phase=2;if(!Write())return fail("nav_export_write_failed");break;}
    const auto pair=state.pairs[state.pairAt++];std::vector<Vec> path;++state.pathCalls;
    if(Path(state.nodes[pair.first],state.nodes[pair.second],path))state.paths.push_back(std::move(path));else ++state.pathRejected;
   }
  }
 }
 if(r.operation==2&&!Write())return fail("nav_export_write_failed");
 snprintf(message,capacity,"nav_export_phase_%u_done_%u_proj_%zu_nodes_%zu_pairs_%zu_of_%zu_paths_%zu",state.phase,state.complete?1:0,state.projectionCalls,state.nodes.size(),state.pairAt,state.pairs.size(),state.paths.size());
 return Result::Applied;
}
inline training_control::Result Dispatch(const uint8_t*data,uint32_t bytes,char*message,size_t capacity,void*world,void*pawn,const char*mapId){
 __try{return DispatchImpl(data,bytes,message,capacity,world,pawn,mapId);}__except(EXCEPTION_EXECUTE_HANDLER){snprintf(message,capacity,"nav_export_exception_stop_required");return training_control::Result::Failed;}
}
}
