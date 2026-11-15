#pragma once
#include "original_hud.h"
#include "original_players.h"

// Read-only pinned PCM/modifier/shake caches. Never updates or disables a
// modifier. ActiveCameraShakeInfo is24 bytes; LastModification isFTransform96.
namespace training_camera_modifiers_probe {
template<class T> inline T Read(const void* p,size_t at){return original_players::Value<T>(p,at);}
inline void Name(FILE* f,void* object){char name[128]{};original_hud::OriginalObjectName(object,name);original_players::Text(f,name);}
inline void Object(FILE* f,void* object){
    std::fprintf(f,"{\"object\":\"%p\",\"name\":",object);Name(f,object);
    std::fputs(",\"class\":",f);auto type=Read<void*>(object,0x10);Name(f,type);
    std::fputs(",\"package\":",f);Name(f,Read<void*>(type,0x28));std::fputc('}',f);
}
inline void Write(FILE* f){
    auto pc=original_players::activeController,pawn=original_players::activePawn;
    auto pcm=Read<void*>(pc,0x528),follow=Read<void*>(pawn,0x12a8);
    auto data=Read<void**>(pcm,0x2990);const auto count=Read<int32_t>(pcm,0x2998),capacity=Read<int32_t>(pcm,0x299c);
    const bool valid=pcm&&count>=0&&count<=64&&capacity>=count&&(!count||original_players::Readable(data,static_cast<size_t>(count)*8));
    std::fprintf(f,"{\"milliseconds\":%llu,\"pcm\":\"%p\",\"valid\":%s,\"count\":%d,\"cached_shake_modifier\":\"%p\",\"pcm_position\":",static_cast<unsigned long long>(GetTickCount64()),pcm,valid?"true":"false",count,Read<void*>(pcm,0x2a38));
    original_players::Vector(f,Read<engine::Vec>(pcm,0x17c0));std::fputs(",\"follow_position\":",f);original_players::Vector(f,Read<engine::Vec>(follow,0x320));
    std::fputs(",\"modifiers\":[",f);
    if(valid)for(int32_t i=0;i<count;++i){auto modifier=Read<void*>(data,static_cast<size_t>(i)*8);if(i)std::fputc(',',f);
        std::fputs("{\"identity\":",f);Object(f,modifier);
        const float alpha=Read<float>(modifier,0x48);
        std::fprintf(f,",\"owner\":\"%p\",\"flags\":%u,\"priority\":%u,\"alpha\":%.9g",Read<void*>(modifier,0x38),Read<uint32_t>(modifier,0x30),Read<uint8_t>(modifier,0x34),std::isfinite(alpha)?alpha:0);
        if(original_players::IsA(modifier,0x40d8640)){
            auto shakes=Read<void*>(modifier,0x50);const auto n=Read<int32_t>(modifier,0x58),cap=Read<int32_t>(modifier,0x5c);
            const bool readable=n>=0&&n<=128&&cap>=n&&(!n||original_players::Readable(shakes,static_cast<size_t>(n)*24));
            std::fprintf(f,",\"active_shakes_valid\":%s,\"active_shake_count\":%d,\"last_modification_translation\":",readable?"true":"false",n);
            original_players::Vector(f,Read<engine::Vec>(modifier,0xe0));std::fputs(",\"shakes\":[",f);
            if(readable)for(int32_t j=0;j<n;++j){auto shake=Read<void*>(shakes,static_cast<size_t>(j)*24);if(j)std::fputc(',',f);
                std::fputs("{\"identity\":",f);Object(f,shake);
                const float scale=Read<float>(shake,0x34);
                std::fprintf(f,",\"manager\":\"%p\",\"scale\":%.9g,\"source\":",Read<void*>(shake,0x40),std::isfinite(scale)?scale:0);
                Object(f,Read<void*>(shakes,static_cast<size_t>(j)*24+8));std::fputs(",\"pattern\":",f);Object(f,Read<void*>(shake,0x38));std::fputc('}',f);
            }
            std::fputc(']',f);
        }
        std::fputc('}',f);
    }
    std::fputs("]}",f);
}
}
