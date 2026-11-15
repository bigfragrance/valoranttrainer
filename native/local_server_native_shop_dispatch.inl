// Included after TickImpl. Only Scenario12 changes the publication boundary.
inline bool TickShopPreludePublished(void* world,void* pc,void* ps,void* pawn,float deltaSeconds,
                                     local_server_native_shop_snapshot::Exit exit){
    // An off-thread stop is only a request. The existing GT Tick consumes it;
    // no engine dispatch or live-state publication is performed by a worker.
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    if(shopPreludeStopRequested.exchange(false,std::memory_order_relaxed))state.stopRequested=true;
    bool result=false,returned=false;
    __try{result=TickImpl(world,pc,ps,pawn,deltaSeconds);returned=true;}
    __finally{PublishShopPrelude(returned?exit:local_server_native_shop_snapshot::Exit::Exception);}
    return result;
}
inline bool Tick(void* world,void* pc,void* ps,void* pawn,float deltaSeconds){
    if(!IsShopPrelude())return TickImpl(world,pc,ps,pawn,deltaSeconds);
    return TickShopPreludePublished(world,pc,ps,pawn,deltaSeconds,local_server_native_shop_snapshot::Exit::Tick);
}
inline bool Stop(){
    if(IsSpectatorInput())return StopSpectatorInput();
    if(IsPeek())return StopPeek();
    if(IsMapHold())return StopMapHold();
    if(!IsShopPrelude()){state.stopRequested=true;return Tick(state.world,state.pc,state.ps,state.pawn,0);}
    shopPreludeStopRequested.store(true,std::memory_order_relaxed);
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    return TickShopPreludePublished(state.world,state.pc,state.ps,state.pawn,0,local_server_native_shop_snapshot::Exit::Stop);
}
