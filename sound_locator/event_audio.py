"""Audio-only transient detector. Events are acoustic bursts, not known source identities.

Uses only past 10 ms RMS blocks, a rolling median background and a rise condition.
Classification windows are fixed at 160 ms with 20 ms pre-onset context. An event is
released only once its full window is available (about 140 ms after detected onset).
Overlapping source onsets can merge; all directional components in the window must
remain available to a downstream multi-source tracker.
"""
from collections import deque
import numpy as np

RATE=48000
BLOCK=480
WINDOW=7680
PRE=960


def detect_events(samples, *, ratio=2.5, refractory_seconds=.20):
    x=np.asarray(samples)
    if x.ndim!=2 or x.shape[1]!=2 or not np.isfinite(x).all():
        raise ValueError("Expected finite 48 kHz stereo samples")
    if np.issubdtype(x.dtype,np.integer):
        if x.dtype==np.uint8:x=(x.astype(float)-128)/128
        else:x=x.astype(float)/(2**(np.iinfo(x.dtype).bits-1))
    n=len(x)//BLOCK
    if n<10:return []
    rms=np.sqrt(np.mean(x[:n*BLOCK].reshape(n,BLOCK,2).astype(float)**2,axis=(1,2)))
    background=deque(maxlen=50)
    last=-RATE
    result=[]
    for i,energy in enumerate(rms):
        floor=max(float(np.median(background)) if background else float(energy),1e-5)
        onset=i*BLOCK
        rise=energy/max(float(rms[i-1]) if i else floor,1e-6)
        if len(background)>=8 and energy>max(floor*ratio,10**(-50/20)) and rise>1.4 and onset-last>=RATE*refractory_seconds:
            start=max(0,onset-PRE)
            end=start+WINDOW
            if end<=len(x):
                result.append({"start_sample":int(start),"center_sample":int(start+WINDOW//2),
                               "end_sample":int(end),"onset_sample":int(onset),
                               "strength":float(energy/floor),"background_rms":floor,
                               "source_count":None})
            last=onset
        background.append(float(energy))
    return result
