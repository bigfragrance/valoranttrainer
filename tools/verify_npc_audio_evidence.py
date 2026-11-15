"""Read-only static original-audio evidence regression.

No native module import, build, game execution, process handle, network or UI.
This validates retained original bytecode/ABI evidence, not acoustic playback.
"""
from pathlib import Path
import hashlib
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'work/original-npc-audio-routing'


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def walk(node):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def verify_spectated_audio(scripts):
    """Original byte profiles and a symbolic stack contract, not a live hook test."""
    folder = OUT/'frame-recovery'
    native = read(folder/'native-body-profiles.json')
    profile = read(folder/'spectated-audio-frame-profiles.json')
    policy = read(folder/'stack-recovery-policy.json')
    image = (ROOT/'work/client-analysis/Shipping-analysis-copy.exe').read_bytes()
    assert hashlib.sha256(image).hexdigest() == native['image_sha256'] == '147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f'
    nt = struct.unpack_from('<I', image, 0x3c)[0]
    assert image[nt:nt+4] == b'PE\0\0'
    optional = nt+24
    sections = [struct.unpack_from('<8sIIII', image,
                    optional+struct.unpack_from('<H', image, nt+20)[0]+i*40)
                for i in range(struct.unpack_from('<H', image, nt+6)[0])]

    def raw(rva, size):
        for _, virtual_size, start, raw_size, raw_at in sections:
            if start <= rva and rva+size <= start+min(virtual_size, raw_size):
                offset = raw_at+rva-start
                return image[offset:offset+size]
        raise AssertionError(('not completely raw-backed', hex(rva), size))

    instruction_bytes = {}
    for body in native['functions']:
        start, end = int(body['start_rva'],16), int(body['end_rva'],16)
        data = raw(start, end-start)
        assert len(data) == body['size'] and data == bytes.fromhex(body['bytes'])
        assert hashlib.sha256(data).hexdigest() == body['sha256']
        cursor = start
        for insn in body['disassembly']:
            assert int(insn['rva'],16) == cursor
            value = bytes.fromhex(insn['bytes'])
            assert len(value) == insn['size'] and raw(cursor,len(value)) == value
            instruction_bytes[cursor] = value.hex()
            cursor += len(value)
        assert cursor == end
        if body['kind'] == 'complete_function':
            cursor = start
            for entry in body['unwind_entries']:
                assert int(entry['begin_rva'],16) == cursor
                assert raw(int(entry['unwind_rva'],16),4).hex() == entry['unwind_header']
                cursor = int(entry['end_rva'],16)
            assert cursor == end
    assert len(native['functions']) == 8
    # Critical register/stack assignments independently bound to original bytes.
    expected_instructions = {
        0x1b9c54f:'57', 0x1b9c550:'4156', 0x1b9c552:'4157', 0x1b9c554:'4883ec30',
        0x1b9c55b:'4d8bf0', 0x1b9c59a:'498dbe90000000', 0x1b9c5ae:'48892f',
        0x1b9c5b4:'498bd6', 0x1b9c5ba:'488b85e0000000', 0x1b9c5c1:'ff1531fc6308',
        0x6c2ceda:'57', 0x6c2cedb:'4883ec20', 0x6c2cee9:'488bda',
        0x6c2cf94:'48897b20', 0x6c2cf98:'488b5c2438', 0x6c2cfaf:'ff1543f25a03',
        0x6c2cfe4:'e867e5c300', 0x786b550:'4057', 0x786b552:'4883ec20',
        0x786b563:'ff158f0c9702', 0x48548e0:'4883ec28', 0x48548fa:'ff15f8789805',
        0x1cdb395:'488bfa', 0x1cdb5b6:'4c8bc7', 0x1cdb5bf:'e87c0fecff',
        0x8e7b4b:'488d056ea23f01', 0x8e7b52:'488905c701e60c', 0x1cdcaf4:'488b4668',
    }
    for address, value in expected_instructions.items():
        assert instruction_bytes[address] == value
    # Original opcode registration, not an uninitialized raw GNative pointer.
    assert 0x8e7b4b+7+struct.unpack('<i',raw(0x8e7b4e,4))[0] == 0x1ce1dc0
    assert 0x8e7b52+7+struct.unpack('<i',raw(0x8e7b55,4))[0] == 0xd747c40+0x1c*8
    assert raw(0x1ce1dc0,20).hex() == '488b42204c8b084883c00848894220e97c95ffff'

    expected_scripts = {
        'Aud_GetHitConfirmLocality': ('Aud_FunctionLibrary',312,496,32,51,0x1c422000),
        'TriggerWorldImpactEffectSound': ('AudioHandlerActor',812,1188,86,127,0x0c420800),
        'GetPlayOnStartEvent': ('Comp_FXC_AudioBasic_PerspectiveEvents_ContextTeam',731,991,111,130,0x1cc20000),
    }
    assert len(profile['profiles']) == 3
    for p in profile['profiles']:
        name = p['full_path'].rsplit('.',1)[-1]
        stem, memory, serialized, vm, resume, flags = expected_scripts[name]
        f, digest = scripts[('/Game/Audio/Core/'+stem,name)]
        queries = [n for n in walk(f['nodes']) if n.get('function') == '/Script/ShooterGame.ShooterBlueprintLibrary.IsLocallyControlledOrSpectated']
        assert len(queries) == 1
        q = queries[0]
        assert q['op'] == p['query_opcode'] == '0x1c'
        assert (q['vm'],q['vm_end'],f['memory_bytes'],f['stored_bytes']) == (vm,resume,memory,serialized)
        assert (p['query_vm'],p['query_resume_vm'],p['script_memory_bytes'],p['serialized_script_bytes']) == (vm,resume,memory,serialized)
        assert int(p['function_flags'],16) == int(f['function_flags'],16) == flags
        assert p['query_parameters'] == q['params'] and digest == p['decoded_json_sha256']
    assert profile['FFrame_layout'] == dict(Node=0x10,Object=0x18,Code=0x20,CurrentNativeFunction=0x90)
    assert profile['loaded_UFunction_layout']['ScriptData'] == 0x68
    assert profile['loaded_UFunction_layout']['ScriptNum'] == 0x70

    # Symbolically execute the actual push/sub/call layout. This deliberately
    # does not consume the proposed policy offsets while constructing the stack.
    def original_stack(helper):
        frame, code, rsp, memory = 0x10008000, 0x2000007f, 0x10004000, {}
        def push(value):
            nonlocal rsp
            rsp -= 8
            memory[rsp] = value
        push(0x1cdb5c4)  # original CallFunction CALL Invoke
        push(frame)      # Invoke PUSH RDI; CallFunction set RDI=Frame
        push('savedR14')
        push('savedR15')
        rsp -= 0x30
        push(0x1b9c5c7)  # Invoke CALL native exec
        push(frame+0x90) # native exec PUSH RDI; Invoke set RDI=Frame+90
        rsp -= 0x20
        if helper:
            push(0x6c2cfe9)
            push(code)  # helper PUSH RDI; exec now holds advanced Code
            rsp -= 0x20
            push(0x786b569)
        else:
            push(0x6c2cfb5)
        rsp -= 0x28     # Pawn prologue
        push(0x4854900)  # Pawn CALL PC.v800
        return {address-rsp:value for address,value in memory.items()}, frame

    cases = 0
    for name in ('direct','helper'):
        p = policy[name]
        slots, frame = original_stack(name == 'helper')
        returns = {int(offset,16):int(rva,16) for offset,rva in p['original_return_slots'].items()}
        for offset, value in returns.items():
            assert slots[offset] == value
        a, b = p['saved_native_exec_rdi_offset'], p['saved_invoke_rdi_offset']
        assert slots[a]-0x90 == slots[b] == frame
        # A classification contract requires all exact returns and both copies.
        def accepts(candidate):
            if not all(candidate.get(offset) == value for offset,value in returns.items()):
                return False
            one, two = candidate.get(a), candidate.get(b)
            return isinstance(one,int) and isinstance(two,int) and one >= 0x90 and one-0x90 == two == frame
        assert accepts(slots)
        cases += 1
        for offset in list(returns)+[a,b]:
            bad = dict(slots)
            bad[offset] = bad[offset]+8
            assert not accepts(bad), (name,hex(offset))
            cases += 1
        bad = dict(slots)
        bad[a] = 0x80
        assert not accepts(bad)
        cases += 1
        if name == 'helper':
            assert slots[0x58] == 0x2000007f and slots[0x58] != frame
            cases += 1
    return dict(spectated_audio_profiles=3, native_frame_byte_profiles=8,
                symbolic_stack_contract_cases=cases,
                runtime_stack_acceptance=False)


def main():
    index = read(OUT/'functions-index.json')
    manifest = read(OUT/'current-toc-and-assets.json')
    assert manifest['toc_count'] == index['toc_count'] == 17
    assert index['complete_functions'] == len(index['functions']) == 130
    function_keys = {(f['package'], f['function']) for f in index['functions']}
    assert len(function_keys) == 130
    packages, scripts = {}, {}
    for pkg in manifest['packages']:
        stem = pkg['package'].rsplit('/', 1)[-1]
        meta = read(OUT/(stem+'-metadata.json'))
        raw = Path(meta['raw']).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == pkg['sha256'] == meta['sha256']
        packages[pkg['package']] = raw
        script_path = OUT/(stem+'-script.json')
        script_hash = hashlib.sha256(script_path.read_bytes()).hexdigest()
        for f in read(script_path)['functions']:
            assert f['complete'] and f['error'] is None
            assert f['memory_bytes'] == f['decoded_memory']
            assert f['stored_bytes'] == f['decoded_stored']
            scripts[(pkg['package'], f['name'])] = (f, script_hash)
    for row in index['functions']:
        f, digest = scripts[(row['package'], row['function'])]
        assert digest == row['decoded_json_sha256']
        start = row['serialized_script_package_offset']
        size = row['serialized_script_bytes']
        raw_script = packages[row['package']][start:start+size]
        assert len(raw_script) == size
        assert hashlib.sha256(raw_script).hexdigest() == row['serialized_script_sha256']
        assert f['memory_bytes'] == row['script_memory_bytes']

    profiles = read(OUT/'audio-frame-profiles.json')
    expected = {
        'Aud_GetPerspectiveActorTeamPerspective': (1264, 1816, 187),
        'Aud_PlayOnObjectWithPerspectiveAndTeamSwitch': (1584, 2356, 384),
        'Aud_PlayOnObjectWithPerspectiveAndTeamSwitch_Callback': (1593, 2373, 384),
        'Aud_PlayOnObjectWithPerspectiveVO': (1716, 2560, 384),
        'Aud_PlayOnObjectWithPerspectiveVO_Callback': (1725, 2577, 384),
    }
    assert len(profiles['primary_profiles']) == 3
    assert len(profiles['additional_vo_profiles']) == 2
    for p in profiles['primary_profiles'] + profiles['additional_vo_profiles']:
        name = p['full_path'].rsplit('.', 1)[-1]
        memory, serialized, query_vm = expected[name]
        assert (p['script_memory_bytes'], p['serialized_script_bytes'], p['query_vm']) == (memory, serialized, query_vm)
        f, _ = scripts[('/Game/Audio/Core/Aud_FunctionLibrary', name)]
        local_queries = [n for n in walk(f['nodes']) if n.get('kind') == 'VirtualCall' and n.get('function') == 'IsLocallyControlled']
        assert len(local_queries) == 1 and local_queries[0]['vm'] == query_vm
        assert int(f['function_flags'], 16) == 0x0c422000
        assert p['frame_layout'] == dict(Node=16, Object=24, Code=32)
        assert p['native_return_chain']['pc_v800_return'] == '0x4854900'
        assert p['native_return_chain']['pawn_v830_caller_return'] == '0x48529be'
        strings = {n.get('value') for n in walk(f['nodes']) if n.get('kind') == 'String'}
        assert {'FirstPerson', 'ThirdPersonEnemy', 'ThirdPersonAlly'} <= strings
    # Bind the production allowlist to independently decoded original scripts.
    header = (ROOT/'native/original_audio_view_consumers.h').read_text(encoding='utf-8')
    declarations = re.findall(r'\{"(Aud_[^"]+)",L"([^"]+)",(\d+),(\d+)\}', header)
    assert len(declarations) == 5
    actual = {name: (path, int(size), int(resume)) for name, path, size, resume in declarations}
    for p in profiles['primary_profiles'] + profiles['additional_vo_profiles']:
        name = p['full_path'].rsplit('.', 1)[-1]
        f, _ = scripts[('/Game/Audio/Core/Aud_FunctionLibrary', name)]
        query = next(n for n in walk(f['nodes']) if n.get('kind') == 'VirtualCall' and n.get('function') == 'IsLocallyControlled')
        assert actual[name] == (p['full_path'], f['memory_bytes'], query['vm_end'])
    registration = read(ROOT/'work/original-audio-listener-audit/registration-local-predicate-profile.json')
    source = (ROOT/'native/original_npc_locality.h').read_text(encoding='utf-8')
    block = re.search(r'ListenerRegistrationBody\[\]\{(.*?)\};', source, re.S).group(1)
    signature = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]+)', block))
    assert signature == bytes.fromhex(registration['bytes'])
    assert registration['return_rva'] == '0x596d2f1'
    values = read(OUT/'perspective-values.json')
    assert values['npc_enemy'] == 'ThirdPersonEnemy'
    assert values['npc_ally'] == 'ThirdPersonAlly'
    assert values['output_switch_group'] == 'Perspective'
    assert [r['value'] for r in values['unrelated_scope_enum']['entries']] == [0, 1, 2, 3]

    # In the existing native entry, the preserved original source direction is
    # queried before its actual spatial/audio dispatch, never supplied by AI.
    play = read(OUT/'function-0x6464e80.json')
    assert any(i['va'] == '0x146465276' and i['instruction'] == 'call 0x14788bb10' for i in play)
    thunk = read(ROOT/'work/duel1-footstep-locality/native/function-0x4852990.json')
    assert any(i['va'] == '0x1448529be' and i['instruction'] == 'mov byte ptr [rbx], al' for i in thunk)
    pawn = read(ROOT/'work/duel1-footstep-locality/native/function-0x48548e0.json')
    assert any(i['instruction'] == 'mov rax, qword ptr [rax + 0x800]' for i in pawn)
    native_layout = read(OUT/'native-call-layouts.json')['PlayEquippableSoundEvent'][0]
    assert native_layout['parameter_size'] == 48
    assert native_layout['parameters'][-1]['offset'] == '0x28'
    spectated = verify_spectated_audio(scripts)
    print(json.dumps(dict(passed=True, original_packages=len(packages), complete_functions=130,
                          pure_audio_profiles=5, runtime_game_executed=False,
                          acoustic_acceptance=False, **spectated), ensure_ascii=False))


if __name__ == '__main__':
    main()
