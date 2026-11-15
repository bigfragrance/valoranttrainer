import sys
from pathlib import Path
import numpy as np
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from event_audio import detect_events, RATE, WINDOW
from learned_audio import associate_events


def audio_bursts(times,seconds=3.):
    rng=np.random.default_rng(67)
    audio=rng.normal(0,.001,(round(seconds*RATE),2))
    for t in times:
        start=round(t*RATE);n=min(round(.05*RATE),len(audio)-start)
        audio[start:start+n]+=rng.normal(0,.08,(n,2))*np.exp(-np.arange(n)[:,None]/(.02*RATE))
    return audio


def test_audio_only_many_bursts_fixed_windows():
    truth=np.arange(.3,2.5,.3)
    events=detect_events(audio_bursts(truth))
    assert len(events)==len(truth) and len(events)>3
    for e,t in zip(events,truth):
        assert abs(e["onset_sample"]/RATE-t)<.021
        assert e["end_sample"]-e["start_sample"]==WINDOW
        assert e["center_sample"]==(e["start_sample"]+e["end_sample"])//2
        assert e["source_count"] is None


def test_no_incomplete_window_or_noise_bursts():
    assert not detect_events(audio_bursts([1.98],seconds=2.))
    assert not detect_events(np.random.default_rng(3).normal(0,.004,(96000,2)))


def test_match_is_evaluation_only_and_one_to_one():
    audio=audio_bursts([.35,.9])
    events=detect_events(audio)
    match=associate_events(events,[{"host_offset_seconds":.3},{"host_offset_seconds":.85}])
    assert match=={0:0,1:1}
    assert detect_events(audio)==events
    with pytest.raises(ValueError):detect_events(np.zeros(100))
