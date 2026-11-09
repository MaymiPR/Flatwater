# Flatwater Vocal Chain: the factory presets against the reference's own

Each of the reference's 43 presets (the faders, colours and Balance from its preset file, set as host parameters;
Sensitivity and Output at 0, as the reference keeps them when a preset loads) against Flatwater Vocal Chain loading
its factory preset of the same place (our names). Residual in dB, lower is better: three held-out vocal parts at
-6 dBFS peaks after 6 s of silence, 48 kHz, the Noise off in both. "Pitch clear" is the same preset with the Pitch
stage cleared in both: the doubler's Wide and Spreader colours pick their splice points from the signal and are
matched in behaviour, not sample for sample (NULLTESTS.md). Written by `plugins/FwVocalChain/nulltest/null_presets.py`.

| # | preset | doubler | part 24 (dB) | part 27 (dB) | part 33 (dB) | Pitch clear, median (dB) |
|---:|---|---|---:|---:|---:|---:|
| 1 | Essentials: Default | Stereo | -43.6 | -45.6 | -38.5 | -44.0 |
| 2 | Essentials: Lead Starter | Stereo | -35.3 | -33.5 | -29.7 | -33.6 |
| 3 | Essentials: Lead Starter Dry | clear | -35.4 | -33.7 | -29.8 | -33.7 |
| 4 | Essentials: Wide Double | Spreader | -20.8 | -5.9 | -7.2 | -29.0 |
| 5 | Essentials: Up Front Dry | clear | -24.3 | -26.6 | -25.0 | -25.0 |
| 6 | Essentials: Up Front | Stereo | -24.3 | -26.6 | -24.9 | -25.0 |
| 7 | Essentials: Everything Up | Spreader | -17.2 | -7.2 | -4.8 | -25.7 |
| 8 | Essentials: Everything Up Dry | Stereo | -27.2 | -30.2 | -25.3 | -27.8 |
| 9 | Essentials: Slap Lead | clear | -37.9 | -34.0 | -36.4 | -36.4 |
| 10 | Essentials: Chamber Lead | Stereo | -27.7 | -29.5 | -26.9 | -27.7 |
| 11 | Essentials: Open Room | clear | -32.0 | -31.1 | -31.5 | -31.5 |
| 12 | Essentials: Brooding | Wide | -36.2 | -32.2 | -16.2 | -33.2 |
| 13 | Essentials: Bright Wall Dry | clear | -30.6 | -32.0 | -29.5 | -30.6 |
| 14 | Essentials: Heavy Wall | clear | -30.1 | -31.6 | -30.2 | -30.2 |
| 15 | Essentials: Rock Lead High | Stereo | -38.9 | -32.3 | -29.3 | -32.4 |
| 16 | Essentials: Rock Lead High Dry | clear | -39.4 | -32.9 | -29.5 | -32.9 |
| 17 | Essentials: Rock Lead Low | clear | -30.1 | -30.9 | -27.2 | -30.1 |
| 18 | Essentials: Rock Lead Low Dry | clear | -30.1 | -30.9 | -27.2 | -30.1 |
| 19 | Essentials: Space Lead | Spreader | -23.4 | -11.1 | -23.3 | -28.6 |
| 20 | Essentials: Spoken Word | clear | -30.5 | -31.9 | -29.5 | -30.5 |
| 21 | Essentials: All Clear | clear | -78.3 | -78.3 | -43.6 | -78.3 |
| 22 | Backing: Backing Starter | Stereo | -29.7 | -30.7 | -28.2 | -29.6 |
| 23 | Backing: Airy Backing | clear | -37.6 | -29.7 | -24.6 | -29.7 |
| 24 | Backing: Answer Line | Spreader | -23.0 | -8.3 | -22.6 | -31.2 |
| 25 | Backing: Backing Wash | clear | -34.5 | -27.2 | -30.2 | -30.2 |
| 26 | Backing: Backing Left | clear | -29.4 | -31.3 | -28.8 | -29.4 |
| 27 | Backing: Backing Right | clear | -29.5 | -31.1 | -28.9 | -29.5 |
| 28 | Backing: Backing Orbit | Stereo | -35.0 | -29.6 | -29.2 | -30.5 |
| 29 | Backing: Backing Close | clear | -32.1 | -30.3 | -27.4 | -30.3 |
| 30 | Backing: Backing Chamber | Stereo | -41.4 | -38.9 | -36.6 | -39.2 |
| 31 | Backing: Backing Rock | Stereo | -30.1 | -30.9 | -27.2 | -30.1 |
| 32 | Studio: Distant Choir | Stereo | -42.2 | -35.9 | -28.8 | -40.3 |
| 33 | Studio: Presence Vocal | Wide | -43.2 | -45.3 | -26.4 | -43.5 |
| 34 | Studio: Broad Snare | Wide | -54.7 | -50.1 | -25.2 | -78.3 |
| 35 | Studio: Parallel Air | Stereo | -46.5 | -41.5 | -38.8 | -78.3 |
| 36 | Studio: Robotic Vocal | Wide | -34.3 | -32.1 | -11.2 | -35.8 |
| 37 | Studio: Eerie Vocal | Stereo | -33.7 | -34.2 | -32.6 | -33.8 |
| 38 | Studio: Wider Percussion | Wide | -40.8 | -35.2 | -12.1 | -59.0 |
| 39 | Backing: Backing Lo-Fi | Stereo | -30.3 | -29.5 | -28.8 | -29.7 |
| 40 | Studio: Background Space | Spreader | -3.5 | -9.3 | -24.5 | -34.8 |
| 41 | Studio: Vocal Space | Spreader | -17.3 | -29.6 | -29.0 | -34.5 |
| 42 | Studio: Squeezed Voice | Spreader | -19.2 | -30.4 | -17.1 | -32.3 |
| 43 | Studio: Wet Ad-Libs | Spreader | -23.8 | -11.7 | -23.6 | -28.2 |

Over the 129 renders: median **-30.1 dB**, worst -3.5; with the Pitch stage clear median **-31.2 dB**, worst -24.2.
- Doubler clear: 48 renders, median -30.6 dB, worst -24.3.
- Doubler Stereo: 42 renders, median -30.5 dB, worst -24.3.
- Doubler Wide: 15 renders, median -34.3 dB, worst -11.2.
- Doubler Spreader: 24 renders, median -18.2 dB, worst -3.5.
