import sys
from pathlib import Path
import numpy as np
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from learned_audio import LearnedAudioModel, ANGLES, BINS, circular_target, angular_features, quality_status


def dummy_model():
    n=int(BINS.sum())*4
    a={"mean":np.zeros(n),"scale":np.ones(n),"w1":np.zeros((64,n)),"b1":np.zeros(64),
       "w2":np.zeros((64,64)),"b2":np.zeros(64),"w3":np.zeros((72,64)),"b3":np.zeros(72)}
    a["b3"][18]=6
    return LearnedAudioModel(a,{"test_fixture":True})


def coherent_signal():
    x=np.random.default_rng(14).normal(0,.1,10000)
    return np.stack([x,np.roll(x,-12)*1.5],axis=1)


def test_circular_target_wrap_and_normalization():
    p=circular_target([359.,-1.,719.,0.])
    assert np.allclose(p.sum(axis=1),1)
    assert np.allclose(p[0],p[1]) and np.allclose(p[0],p[2])
    assert p[3,1]==pytest.approx(p[3,-1])


def test_full_score_field_known_heading_and_roundtrip(tmp_path):
    m=dummy_model();path=tmp_path/"model.npz";m.save(path);m=LearnedAudioModel.load(path)
    r=m.predict(coherent_signal(),350)
    assert r["status"]=="angular_scores"
    assert len(r["direction_scores"])==72
    assert sum(r["direction_scores"])==pytest.approx(1,abs=1e-6)
    assert r["top_relative_deg"]==90
    assert r["world_angles_deg"][18]==80
    assert not r["score_is_probability"] and r["source_count"] is None


def test_silence_mono_and_independent_noise_reject():
    m=dummy_model()
    assert m.predict(np.zeros((7680,2)))["status"]=="quiet"
    x=np.random.default_rng(19).normal(0,.1,7680)
    assert m.predict(np.stack([x,x],axis=1))["status"]=="center_or_mono"
    independent=np.random.default_rng(47).normal(0,.1,(7680,2))
    r=m.predict(independent)
    assert r["status"]=="incoherent" and not np.any(r["direction_scores"])


def test_uniform_model_rejected():
    m=dummy_model();m.arrays["b3"][:]=0
    assert m.predict(coherent_signal())["status"]=="uninformative_model"


def test_input_model_validation():
    m=dummy_model()
    with pytest.raises(ValueError):m.predict(coherent_signal(),float("nan"))
    with pytest.raises(ValueError):m.predict(np.zeros(7680))
    with pytest.raises(ValueError):m.scores_from_features(np.zeros(4))
    with pytest.raises(ValueError):LearnedAudioModel({**m.arrays,"scale":np.zeros_like(m.arrays["scale"])})


def test_features_ignore_common_gain_and_reverse_interaural_sign():
    x=coherent_signal();a,_=angular_features(x);b,_=angular_features(x*.25);r,_=angular_features(x[:,::-1])
    assert np.allclose(a,b,atol=1e-5)
    n=int(BINS.sum())
    assert np.allclose(a[:n],-r[:n],atol=1e-5)
    assert np.allclose(a[n:2*n],r[n:2*n],atol=1e-5)
    assert np.allclose(a[2*n:3*n],-r[2*n:3*n],atol=1e-5)
