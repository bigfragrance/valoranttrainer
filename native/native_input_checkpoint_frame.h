#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// Pure comparison of already-native-decoded records. This helper does NOT
// authorize cancellation. The caller must retain every original epoch/owner,
// publication, correction-index, original reader/end/shape and known-prefix
// gate, then use only the original prune and original Apply paths.
namespace native_input_checkpoint_frame {
enum class Reason:uint8_t {
    Matched=0,SpanUnavailable,BufferUnreadable,BarrierMissing,BarrierTimeChanged,
    TypeChanged,UnsupportedInterveningType,EquippableStateChanged,
    ActivationStateChanged,MovementStateChanged,AimChanged,AimMissing,
    Type3StateChanged,Type10StateChanged,SpanLengthChanged
};
struct Event {
    uint8_t type=0xfe,equippableBits=0,movementBit=0;
    uint16_t activationBits=0;
    uint8_t type3Flags=0,type10Value=0,type10Flag=0;
    uint32_t extraBits=0;float extra=0,deadline=0;
    std::array<uint64_t,3> aim{};
};
struct Proof {
    bool matched=false;Reason reason=Reason::SpanUnavailable;
    uint32_t compared=0,aimOrdinal=0,interveningStates=0,failedOrdinal=0;
    uint32_t equippableStates=0,activationStates=0,movementStates=0;
    uint32_t type3States=0,type10States=0;
    uint8_t retainedType=0xfe,wireType=0xfe;
};
inline const char* Name(Reason reason){
    switch(reason){
    case Reason::Matched:return "matched_original_frame_semantics";
    case Reason::SpanUnavailable:return "bounded_native_decoded_span_unavailable";
    case Reason::BufferUnreadable:return "native_decoded_buffer_unreadable";
    case Reason::BarrierMissing:return "replacement_barrier_missing_at_ordinal3";
    case Reason::BarrierTimeChanged:return "replacement_original_client_barrier_time_changed";
    case Reason::TypeChanged:return "retained_and_wire_event_types_changed";
    case Reason::UnsupportedInterveningType:return "intervening_native_event_semantics_unproven";
    case Reason::EquippableStateChanged:return "intervening_equippable_state_changed";
    case Reason::ActivationStateChanged:return "intervening_activation_state_changed";
    case Reason::MovementStateChanged:return "intervening_movement_state_changed";
    case Reason::AimChanged:return "replacement_native_aim_changed";
    case Reason::AimMissing:return "replacement_native_aim_not_in_same_bounded_frame";
    case Reason::Type3StateChanged:return "intervening_native_type3_state_changed";
    case Reason::Type10StateChanged:return "intervening_native_type10_state_changed";
    case Reason::SpanLengthChanged:return "retained_native_suffix_shorter_than_wire_packet";
    }
    return "unknown_frame_proof_reason";
}
template<class ReadEvent> inline Proof Prove(uint32_t retainedCount,uint32_t wireCount,uint32_t maximum,ReadEvent read){
    Proof out{};
    if(retainedCount<5||wireCount<5||wireCount>maximum||uint64_t(retainedCount)>uint64_t(maximum)*2)return out;
    // Compare the complete fresh native packet. The retained suffix can also
    // contain later packets. Native histories can omit an unchanged aim record.
    if(retainedCount<wireCount){out.reason=Reason::SpanLengthChanged;return out;}
    const auto r=read(false,3),w=read(true,3);
    out.failedOrdinal=3;out.retainedType=r.type;out.wireType=w.type;
    if(r.type!=7||w.type!=7){out.reason=Reason::BarrierMissing;return out;}
    ++out.compared;
    if(!std::isfinite(r.deadline)||!std::isfinite(w.deadline)||r.deadline!=w.deadline){out.reason=Reason::BarrierTimeChanged;return out;}
    for(uint32_t ordinal=4;ordinal<wireCount;++ordinal){
        const auto a=read(false,ordinal),b=read(true,ordinal);
        out.failedOrdinal=ordinal;out.retainedType=a.type;out.wireType=b.type;
        if(a.type!=b.type){out.reason=Reason::TypeChanged;return out;}
        ++out.compared;
        if(a.type==5){
            if(a.aim!=b.aim){out.reason=Reason::AimChanged;return out;}
            out.aimOrdinal=ordinal;continue;
        }
        if(a.type==0){
            if(!std::isfinite(a.extra)||!std::isfinite(b.extra)||a.equippableBits!=b.equippableBits||a.extraBits!=b.extraBits){
                out.reason=Reason::EquippableStateChanged;return out;}
            ++out.equippableStates;
        }else if(a.type==1){
            if(a.activationBits!=b.activationBits){out.reason=Reason::ActivationStateChanged;return out;}
            ++out.activationStates;
        }else if(a.type==2){
            if(a.movementBit!=b.movementBit){out.reason=Reason::MovementStateChanged;return out;}
            ++out.movementStates;
        }else if(a.type==3){
            if(a.type3Flags!=b.type3Flags){out.reason=Reason::Type3StateChanged;return out;}
            ++out.type3States;
        }else if(a.type==10){
            if(a.type10Value!=b.type10Value||a.type10Flag!=b.type10Flag){out.reason=Reason::Type10StateChanged;return out;}
            ++out.type10States;
        }else{out.reason=Reason::UnsupportedInterveningType;return out;}
        ++out.interveningStates;
    }
    out.matched=true;out.reason=Reason::Matched;return out;
}
} // namespace native_input_checkpoint_frame
