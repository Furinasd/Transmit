# Transmit original mechanical / electromagnetic audio sources

Eight deterministic, original synthesized one-shots. No sampled recordings,
downloaded assets, external packages, or Fab content are used. These source WAVs
are authored for this repository; no third-party audio licence is required.

Regenerate from any working directory:

```sh
python3 Scripts/Presentation/generate_audio.py
python3 Scripts/Presentation/generate_audio.py --verify
```

All sources are **48,000 Hz / mono / signed PCM16**, with a **-3 dBFS peak**,
1.5 ms onset smoothing and a 45–60 ms end fade. Fixed per-event random seeds make
generation repeatable. The script validates format, exact frame count, clipping,
DC offset below 0.001 full scale, zero endpoints and endpoint steps below 0.001.

| Source | Duration | Character | Initial runtime gain |
| --- | ---: | --- | ---: |
| `S_Transmit_Capture.wav` | 0.30 s | Intake pressure rises into a dry latch at 195 ms | 0.65 |
| `S_Transmit_Transfer.wav` | 0.35 s | Coil discharge, descending body and air release | 0.65 |
| `S_Transmit_Dock.wav` | 0.90 s | Relays at 40 / 230 / 440 ms, transformer settles | 0.85 |
| `S_Transmit_Telegraph.wav` | 0.80 s | Rising coil with accelerating pulses | 0.42 |
| `S_Transmit_Intercept.wav` | 0.45 s | Dry brake strike, immediate descending low tail | 0.85 |
| `S_Transmit_RamImpact1.wav` | 0.65 s | Compact steel crack and a few bright fragments | 0.90 |
| `S_Transmit_RamImpact2.wav` | 1.30 s | Heavier low body, secondary shear, debris cascade | 1.00 |
| `S_Transmit_Complete.wav` | 1.80 s | Warm open fifth, late quiet third, restrained release | 0.35 |

These gains are starting points for the combined mix, not playtest-approved
balances. In particular the completion chord is sustained and needs lower gain
than the transient peaks. Ram 2 should read as heavier through timbre and duration
without forcing the mix above headroom.

## Unreal integration

Import only these WAV sources through Unreal AssetTools into the dedicated
`/Game/Transmit/Presentation/Audio/` folder, retaining the source basenames. The expected
SoundWave object path for Capture is
`/Game/Transmit/Presentation/Audio/S_Transmit_Capture.S_Transmit_Capture`.

Use one-shot playback on confirmed presentation events. Trigger Capture and
Transfer only on successful actions, Dock on arrival activation, and Ram cues
on actual impact. Play the 0.8 s Telegraph at warning onset; coordinate warning
duration with A rather than changing gameplay timing to fit audio. Stop its
AudioComponent on Intercept so a charge does not continue underneath a stopped
attack. Stop active presentation audio on Reset. Use local spatial playback for
world machinery; completion can use non-spatial playback at the lower gain above.

No SoundWave assets or maps were authored by this audio-only subtask. Import,
reference resolution, spatial attenuation, event alignment, simultaneous-event
headroom and audible quality in the actual gameplay mix still require Editor
validation and Ely's final listening judgment. Statistical validation does not
establish subjective audio quality.
