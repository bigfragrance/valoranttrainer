#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>

// Diagnostic-only POD. This never decides native blindness or permits a query.
namespace humanlike_blind_metadata_diagnostics {
enum class Stage:uint8_t {None,FunctionMissing,FunctionWeakCapture,OwnerWeakCapture,NameLookup,ReturnLookup,FinalGate,CaptureFault};
enum Bit:uint64_t {
    FunctionUnavailable=1ULL<<0,OwnerUnavailable=1ULL<<1,NameUnavailable=1ULL<<2,FieldUnavailable=1ULL<<3,FieldClassUnavailable=1ULL<<4,
    FunctionWeakInvalid=1ULL<<5,OwnerWeakInvalid=1ULL<<6,OuterNotCachedOwner=1ULL<<7,OwnerNotNativeClass=1ULL<<8,
    ParameterBytes=1ULL<<9,FunctionFlags=1ULL<<10,Exec=1ULL<<11,PropertiesBytes=1ULL<<12,
    ReturnNotFirst=1ULL<<13,ReturnHasNext=1ULL<<14,ReturnCastFlags=1ULL<<15,ReturnOffset=1ULL<<16,
    ReturnSize=1ULL<<17,ReturnDimension=1ULL<<18,ReturnFlags=1ULL<<19,BoolSize=1ULL<<20,
    BoolOffset=1ULL<<21,BoolByteMask=1ULL<<22,BoolFieldMask=1ULL<<23,ReturnName=1ULL<<24,
    ReturnLookupMissing=1ULL<<25,DiagnosticCaptureFault=1ULL<<26
};
struct Snapshot {
    uint64_t serial=0,mismatches=0,unavailable=0;Stage stage=Stage::None;
    bool functionKnown=false,ownerComparisonKnown=false,nameKnown=false,fieldKnown=false,fieldClassKnown=false;
    bool functionLive=false,ownerLive=false,firstChildKnown=false,fieldFromFirstChild=false;
    uintptr_t image=0,function=0,outer=0,cachedOwner=0,expectedOwner=0,exec=0,firstChild=0,cachedField=0,inspectedField=0,fieldClass=0,next=0;
    uint32_t functionFlags=0;uint16_t parameterBytes=0;int32_t propertiesBytes=0;
    uint64_t fieldCastFlags=0,fieldFlags=0;int32_t offset=-1,size=-1,dimensions=-1;
    uint8_t boolSize=0,boolOffset=0,byteMask=0,fieldMask=0;
    std::array<uint32_t,3> expectedName{},actualName{};
};
static_assert(std::is_trivially_copyable_v<Snapshot>);
inline constexpr uint32_t ExpectedFunctionFlags=0x54020401U;
// Original descriptor -> FProperty ctor -> native-byte FBoolProperty ctor/link.
inline constexpr uint64_t DeclaredReturnFlags=0x0010000000000580ULL;
inline constexpr uint64_t ExpectedReturnFlags=DeclaredReturnFlags|0x0008000000000000ULL|0x0000001040000200ULL,ExpectedCastFlags=0x28001ULL;
static_assert(ExpectedReturnFlags==0x0018001040000780ULL);
inline constexpr uintptr_t ExpectedExecRva=0x65b54e0;
inline const char* StageName(Stage stage){switch(stage){
    case Stage::None:return "none";case Stage::FunctionMissing:return "function_missing";
    case Stage::FunctionWeakCapture:return "function_weak_capture_failed";case Stage::OwnerWeakCapture:return "owner_weak_capture_failed";
    case Stage::NameLookup:return "existing_return_name_missing";case Stage::ReturnLookup:return "return_field_lookup_or_name_failed";
    case Stage::FinalGate:return "unchanged_exact_metadata_gate_rejected";case Stage::CaptureFault:return "diagnostic_capture_fault";
    }return "unknown";}
inline void Assess(Snapshot&s){
    s.mismatches=s.unavailable=0;
    if(s.stage==Stage::CaptureFault){
        s.functionKnown=s.ownerComparisonKnown=s.fieldKnown=s.fieldClassKnown=s.firstChildKnown=false;
    }
    if(!s.functionKnown)s.unavailable|=FunctionUnavailable;
    else{
        if(!s.functionLive)s.mismatches|=FunctionWeakInvalid;
        if(s.outer!=s.cachedOwner)s.mismatches|=OuterNotCachedOwner;
        if(s.parameterBytes!=1)s.mismatches|=ParameterBytes;
        if(s.functionFlags!=ExpectedFunctionFlags)s.mismatches|=FunctionFlags;
        if(s.exec!=s.image+ExpectedExecRva)s.mismatches|=Exec;
        if(s.propertiesBytes<1)s.mismatches|=PropertiesBytes;
    }
    if(!s.ownerComparisonKnown)s.unavailable|=OwnerUnavailable;
    else{if(!s.ownerLive)s.mismatches|=OwnerWeakInvalid;if(s.cachedOwner!=s.expectedOwner)s.mismatches|=OwnerNotNativeClass;}
    if(!s.nameKnown)s.unavailable|=NameUnavailable;
    if(!s.cachedField)s.mismatches|=ReturnLookupMissing;
    if(s.firstChildKnown&&s.firstChild!=s.cachedField)s.mismatches|=ReturnNotFirst;
    if(!s.fieldKnown)s.unavailable|=FieldUnavailable;
    else{
        if(s.next)s.mismatches|=ReturnHasNext;
        if(s.offset!=0)s.mismatches|=ReturnOffset;if(s.size!=1)s.mismatches|=ReturnSize;if(s.dimensions!=1)s.mismatches|=ReturnDimension;
        if(s.fieldFlags!=ExpectedReturnFlags)s.mismatches|=ReturnFlags;
        if(s.boolSize!=1)s.mismatches|=BoolSize;if(s.boolOffset!=0)s.mismatches|=BoolOffset;
        if(!s.byteMask)s.mismatches|=BoolByteMask;if(!s.fieldMask)s.mismatches|=BoolFieldMask;
        // Runtime's existing name check compares the original low 64 bits.
        if(s.nameKnown&&(s.expectedName[0]!=s.actualName[0]||s.expectedName[1]!=s.actualName[1]))s.mismatches|=ReturnName;
    }
    if(!s.fieldClassKnown)s.unavailable|=FieldClassUnavailable;
    else if(s.fieldCastFlags!=ExpectedCastFlags)s.mismatches|=ReturnCastFlags;
    if(s.stage==Stage::CaptureFault)s.unavailable|=DiagnosticCaptureFault;
}
inline std::string Json(const Snapshot&s,uint64_t captures,uint64_t cumulativeMask){
    char b[4096]{};const auto yes=[](bool v){return v?"true":"false";};
    const int count=std::snprintf(b,sizeof(b),
        "{\"captures\":%llu,\"serial\":%llu,\"stage\":\"%s\",\"mismatch_mask\":\"%016llx\",\"unavailable_mask\":\"%016llx\",\"cumulative_mismatch_mask\":\"%016llx\","
        "\"function_known\":%s,\"owner_comparison_known\":%s,\"name_known\":%s,\"field_known\":%s,\"field_class_known\":%s,\"first_child_known\":%s,\"field_from_first_child_diagnostic_only\":%s,"
        "\"function\":{\"object\":\"%llx\",\"live\":%s,\"flags\":\"%08x\",\"expected_flags\":\"%08x\",\"bytes\":%u,\"expected_bytes\":1,\"properties_bytes\":%d,\"expected_minimum_properties_bytes\":1,"
        "\"exec\":\"%llx\",\"exec_rva\":\"%llx\",\"expected_exec_rva\":\"65b54e0\",\"outer\":\"%llx\",\"cached_owner\":\"%llx\",\"expected_owner\":\"%llx\",\"owner_live\":%s},"
        "\"return_field\":{\"cached\":\"%llx\",\"first_child\":\"%llx\",\"inspected\":\"%llx\",\"class\":\"%llx\",\"next\":\"%llx\","
        "\"cast_flags\":\"%016llx\",\"expected_cast_flags\":\"0000000000028001\",\"flags\":\"%016llx\",\"expected_flags\":\"0018001040000780\","
        "\"offset\":%d,\"expected_offset\":0,\"bytes\":%d,\"expected_bytes\":1,\"dimensions\":%d,\"expected_dimensions\":1,"
        "\"bool_size\":%u,\"expected_bool_size\":1,\"bool_offset\":%u,\"expected_bool_offset\":0,\"byte_mask\":%u,\"field_mask\":%u,\"expected_masks_nonzero\":true,"
        "\"actual_name_words\":[%u,%u,%u],\"expected_name_words\":[%u,%u,%u]},\"diagnostic_only\":true,\"metadata_gate_relaxed\":false,\"native_query_invoked_by_diagnostics\":false}",
        static_cast<unsigned long long>(captures),static_cast<unsigned long long>(s.serial),StageName(s.stage),
        static_cast<unsigned long long>(s.mismatches),static_cast<unsigned long long>(s.unavailable),static_cast<unsigned long long>(cumulativeMask),
        yes(s.functionKnown),yes(s.ownerComparisonKnown),yes(s.nameKnown),yes(s.fieldKnown),yes(s.fieldClassKnown),yes(s.firstChildKnown),yes(s.fieldFromFirstChild),
        static_cast<unsigned long long>(s.function),yes(s.functionLive),s.functionFlags,ExpectedFunctionFlags,unsigned(s.parameterBytes),s.propertiesBytes,
        static_cast<unsigned long long>(s.exec),static_cast<unsigned long long>(s.exec>=s.image&&s.exec-s.image<0xe69c000?s.exec-s.image:0),
        static_cast<unsigned long long>(s.outer),static_cast<unsigned long long>(s.cachedOwner),static_cast<unsigned long long>(s.expectedOwner),yes(s.ownerLive),
        static_cast<unsigned long long>(s.cachedField),static_cast<unsigned long long>(s.firstChild),static_cast<unsigned long long>(s.inspectedField),static_cast<unsigned long long>(s.fieldClass),static_cast<unsigned long long>(s.next),
        static_cast<unsigned long long>(s.fieldCastFlags),static_cast<unsigned long long>(s.fieldFlags),s.offset,s.size,s.dimensions,
        unsigned(s.boolSize),unsigned(s.boolOffset),unsigned(s.byteMask),unsigned(s.fieldMask),
        s.actualName[0],s.actualName[1],s.actualName[2],s.expectedName[0],s.expectedName[1],s.expectedName[2]);
    return count>=0&&static_cast<size_t>(count)<sizeof(b)?std::string(b,static_cast<size_t>(count)):"{\"diagnostic_format_overflow\":true}";
}
} // namespace humanlike_blind_metadata_diagnostics
