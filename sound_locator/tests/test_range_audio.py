import sys
from pathlib import Path
import numpy as np
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from range_audio import RangeAudioModel,event_features,BINS,LOG_GRID


def model_fixture():
    audio=np.random.default_rng(71).normal(0,.05,(7680,2))
    x,c,_=event_features(audio,30,np.zeros(BINS.sum()))
    a={"range_mean":x,"range_scale":np.ones(27),"range_coef":np.zeros(27),"range_intercept":np.log(5.),
       "reference_x":np.zeros((2,27)),"reference_log_m":np.log([4.,6.]),"class_mean":c,"class_scale":np.ones(21),
       "class_coef":np.zeros(21),"class_intercept":10.,"noise_floor":np.zeros(BINS.sum()),"residual_sigma":.2,
       "domain_limit":2.,"class_threshold":.98,"auto_enabled":1,"log_min":np.log(2.),"log_max":np.log(20.)}
    return RangeAudioModel(a),audio


def test_range_requires_context_and_supported_class():
    model,audio=model_fixture()
    assert not model.predict(audio,30)["informative"]
    assert model.predict(audio,30,event_class="gunshot",context_verified=True)["status"]=="unsupported_event_class"
    model.a["auto_enabled"]=np.array(0.)
    assert model.predict(audio,30,context_verified=True)["status"]=="automatic_classification_not_validated"


def test_range_weak_likelihood_and_wide_uncertainty_roundtrip(tmp_path):
    model,audio=model_fixture();path=tmp_path/"range.npz";model.save(path);model=RangeAudioModel.load(path)
    r=model.predict(audio,30,context_verified=True)
    assert r["informative"] and r["median_m"]==pytest.approx(5.)
    assert len(r["likelihood"])==len(LOG_GRID)
    assert min(r["likelihood"])>=.7 and max(r["likelihood"])<=1
    assert r["interval_m"][1]/r["interval_m"][0]>4
    assert not r["interval_is_calibrated"] and not r["score_is_probability"]


def test_range_unknown_silent_outside_domain_reject():
    model,audio=model_fixture()
    assert model.predict(np.zeros_like(audio),30,context_verified=True)["status"]=="insufficient_event_energy"
    model.a["class_intercept"]=np.array(-10.)
    assert model.predict(audio,30,context_verified=True)["status"]=="unknown_audio_class"
    model.a["class_intercept"]=np.array(10.);model.a["domain_limit"]=np.array(.001)
    assert model.predict(audio*.2,30,context_verified=True)["status"]=="outside_calibration_domain"


def test_class_features_ignore_common_gain_but_range_does_not():
    _,audio=model_fixture();x,c,_=event_features(audio,30);y,d,_=event_features(audio*.5,30)
    assert np.allclose(c,d,atol=1e-5)
    assert x[0]-y[0]==pytest.approx(6.0206/12.,abs=1e-4)
    with pytest.raises(ValueError):event_features(audio,float("nan"))
