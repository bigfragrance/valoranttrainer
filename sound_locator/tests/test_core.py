"""Signal and geometry checks only. Synthetic results do not validate VALORANT."""
import json
import sys
from pathlib import Path
import numpy as np
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from core import Locator, Pose, Profile, RATE, read_pose, stereo, angular_distance


def synthetic(angle, seed, seconds=.4, mask=None):
    rng=np.random.default_rng(seed)
    n=round(RATE*seconds)
    f=np.fft.rfftfreq(n,1/RATE)
    a=np.deg2rad(angle)
    z=np.fft.rfft(rng.normal(0,.08,n))
    z[(f<250)|(f>12000)]=0
    if mask is not None:z*=mask(f)
    ild=8*np.sin(a)+7*np.cos(a)*np.sin(f/1800)
    phase=2*np.pi*f*.0005*np.sin(a)+.7*np.cos(a)*np.cos(f/1100)
    left=z*10**(-ild/40)*np.exp(-.5j*phase)
    right=z*10**(ild/40)*np.exp(.5j*phase)
    return np.stack([np.fft.irfft(left,n),np.fft.irfft(right,n)],axis=1)


@pytest.fixture(scope="module")
def profile():
    return Profile.train([(a,synthetic(a,a+1,1)) for a in range(0,360,30)],{"kind":"synthetic_test_only"})


@pytest.mark.parametrize("heading,relative,vector",[(0,0,[0,-1]),(0,90,[1,0]),(90,90,[0,1]),(270,90,[0,-1])])
def test_map_conventions(heading,relative,vector):
    assert np.allclose(Pose(.2,.7,heading).ray(relative)["unit_vector"],vector,atol=1e-8)


def test_pose_validation(tmp_path):
    p=tmp_path/"pose.json"
    p.write_text(json.dumps(dict(x=.2,y=.6,heading_deg=350,timestamp=100)))
    assert read_pose(p,now=100.1).x==.2
    with pytest.raises(ValueError):read_pose(p,now=102)
    with pytest.raises(ValueError):Pose(3,0,0)


def test_no_false_precision_without_profile():
    r=Locator().analyze(synthetic(90,3))
    assert r["side_hint"]=="右侧" and not r["candidates"] and r["status"]=="uncalibrated"


def test_silence_and_mono(profile):
    assert Locator(profile).analyze(np.zeros((8000,2)))["status"]=="quiet"
    mono=np.random.default_rng(3).normal(0,.1,8000)
    assert Locator(profile).analyze(np.stack([mono,mono],axis=1))["status"]=="center_or_mono"
    with pytest.raises(ValueError):stereo(mono)
    with pytest.raises(ValueError):stereo(np.zeros((8000,8)))


def test_roundtrip(profile,tmp_path):
    path=tmp_path/"profile.npz";profile.save(path)
    p=Profile.load(path)
    assert p.full_circle and np.allclose(p.ild,profile.ild)


@pytest.mark.parametrize("angle",range(0,360,30))
def test_held_out_synthetic_direction(profile,angle):
    r=Locator(profile).analyze(synthetic(angle,angle+900),pose=Pose(.5,.5,90))
    assert r["candidates"],r
    c=r["candidates"][0]
    assert angular_distance(c["relative_deg"],angle)<=30,r
    assert c["ray"]["bearing_deg"]==(90+c["relative_deg"])%360


def test_disjoint_frequency_overlap(profile):
    left=synthetic(270,399,mask=lambda f: f<5500)
    right=synthetic(90,400,mask=lambda f: f>=5500)
    r=Locator(profile).analyze(left+right)
    angles=[c["relative_deg"] for c in r["candidates"]]
    assert 90 in angles and 270 in angles,r
    assert r["source_count"] is None


def test_random_independent_stereo_rejected(profile):
    r=Locator(profile).analyze(np.random.default_rng(99).normal(0,.08,(7680,2)))
    assert not r["candidates"],r


def test_partial_calibration_not_full_circle():
    p=Profile.train([(0,synthetic(0,40))])
    assert not p.full_circle
    assert Locator(p).analyze(synthetic(90,50))["status"]=="incomplete_profile"


def test_silent_calibration_rejected():
    with pytest.raises(ValueError):Profile.train([(0,np.zeros((48000,2)))])


def test_integer_and_resampling():
    x=np.tile(np.array([[32767,-32768]],dtype=np.int16),(12000,1))
    y=stereo(x,24000)
    assert y.shape==(24000,2)
    assert np.allclose(y[100:-100].mean(axis=0),[1,-1],atol=.002)


def test_indistinguishable_templates_are_reported(profile):
    ild,phase,support=profile.ild.copy(),profile.phase.copy(),profile.support.copy()
    ild[5]=ild[1];phase[5]=phase[1];support[5]=support[1]
    p=Profile(profile.angles,ild,phase,support)
    assert 150 in p.aliases[1] and 30 in p.aliases[5]
    r=Locator(p).analyze(synthetic(30,800))
    assert r["candidates"][0]["ambiguous_with_deg"]


def test_background_profile_roundtrip_and_rejection(tmp_path):
    bg=np.random.default_rng(7).normal(0,.01,(48000,2))
    clips=[(a,synthetic(a,a+1,1)+bg) for a in range(0,360,30)]
    p=Profile.train(clips,background=bg)
    path=tmp_path/"p.npz";p.save(path);p=Profile.load(path)
    assert p.noise_floor is not None
    assert not Locator(p).analyze(bg[:7680])["candidates"]
    assert Locator(p).analyze(synthetic(90,980)[:7680])["candidates"]
