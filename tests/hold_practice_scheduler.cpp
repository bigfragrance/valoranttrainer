#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "training_bot_controller.h"
#include <cassert>
#include <cstdio>
using namespace training_bot_controller;
int main(){
  training_control::RouteSet legacy{};const auto legacyPlan=training_control::ReadPracticePlan(legacy);
  assert(legacyPlan.magic==0&&legacyPlan.count==0);
  routeCount=1;routes[0].id=1;routes[0].enabled=true;routes[0].weight=1;
  routes[0].start={0,0,0};routes[0].expose={100,0,0};routes[0].retreat={0,0,0};
  config.arrivalCm=200; // Exercise phase transitions without native path calls.
  for(size_t i=1;i<=2;++i){actors[i]={};actors[i].bound=true;actors[i].bot=true;actors[i].observed.ready=true;actors[i].observed.alive=true;actors[i].rng=static_cast<uint32_t>(i);}
  practicePlan={};practicePlan.magic=0x504c414e;practicePlan.version=1;practicePlan.interval=2;practicePlan.count=1;
  practicePlan.entries[0]={0,0,1,7,0,0,0};nextExposure=0;
  actors[1].phase=actors[2].phase=Phase::Waiting;actors[1].route=actors[2].route=0;
  HoldStrategy(1,10);HoldStrategy(2,10);
  assert(actors[1].phase==Phase::Holding);assert(actors[1].output.crouch);assert(actors[2].phase==Phase::Waiting);
  HoldStrategy(2,11.99);assert(actors[2].phase==Phase::Waiting);
  HoldStrategy(2,12);assert(actors[2].phase==Phase::Holding);
  practicePlan.mode=1;practicePlan.count=3;practicePlan.entries[0]={0,0,1,1,0,1,1};
  practicePlan.entries[1]={0,1,1,8,1,2,2};practicePlan.entries[2]={1,0,1,1,0,0,0};
  practiceWave=0;practiceWaveReady=false;practiceFinished=false;ScheduleWave(20);
  assert(practiceAssignment[1]==0&&practiceAssignment[2]==1);
  assert(practiceDue[1]==21&&practiceDue[2]==22);
  HoldStrategy(1,20);assert(actors[1].phase==Phase::Waiting);
  HoldStrategy(1,21);assert(practiceExposed[1]);
  actors[1].observed.alive=false;ScheduleWave(21.1);assert(practiceDone[1]&&!practiceDone[2]);
  practiceDone[2]=true;ScheduleWave(22);assert(practiceWave==1&&!practiceWaveReady&&!practiceFinished);
  actors[1].observed.alive=true;ScheduleWave(23);assert(practiceAssignment[1]==2&&practiceAssignment[2]==-1);
  practiceDone[1]=true;ScheduleWave(24);assert(practiceFinished);
  practiceWave=0;practiceWaveReady=false;practiceFinished=false;ScheduleWave(30);ScheduleWave(63);assert(practiceWave==1);
  std::puts("native hold scheduler checks passed");
}
