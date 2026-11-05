# Flatwater: measurements

What was measured on each original and how each part of each model was identified, one file per plug-in. Every section
is listed here, so a section a comment or a document cites by name is found below. The data is in
[measurements/](measurements). Written by `scripts/doc_index.py`.

## Flatwater Vocal Comp

[plugins/FwVocalComp/MEASUREMENTS.md](plugins/FwVocalComp/MEASUREMENTS.md) (Renaissance Vox):

- [Setup](plugins/FwVocalComp/MEASUREMENTS.md#setup)
- [1. Latency, parameter maps and fixed gain](plugins/FwVocalComp/MEASUREMENTS.md#1-latency-parameter-maps-and-fixed-gain)
- [2. Detector model](plugins/FwVocalComp/MEASUREMENTS.md#2-detector-model)
- [3. Look-ahead](plugins/FwVocalComp/MEASUREMENTS.md#3-look-ahead)
- [4. Static law (the compressor)](plugins/FwVocalComp/MEASUREMENTS.md#4-static-law-the-compressor)
- [5. Slow path](plugins/FwVocalComp/MEASUREMENTS.md#5-slow-path)
- [6. Learned slow path](plugins/FwVocalComp/MEASUREMENTS.md#6-learned-slow-path)
- [7. Gate](plugins/FwVocalComp/MEASUREMENTS.md#7-gate)
- [8. Not measured, and why](plugins/FwVocalComp/MEASUREMENTS.md#8-not-measured-and-why)
- [9. Differences from the product documentation](plugins/FwVocalComp/MEASUREMENTS.md#9-differences-from-the-product-documentation)
- [CPU](plugins/FwVocalComp/MEASUREMENTS.md#cpu)
- [Provenance](plugins/FwVocalComp/MEASUREMENTS.md#provenance)

## Flatwater Bass Lift

[plugins/FwBassLift/MEASUREMENTS.md](plugins/FwBassLift/MEASUREMENTS.md) (Renaissance Bass):

- [Setup](plugins/FwBassLift/MEASUREMENTS.md#setup)
- [1. Path, latency and output trim](plugins/FwBassLift/MEASUREMENTS.md#1-path-latency-and-output-trim)
- [2. The original path](plugins/FwBassLift/MEASUREMENTS.md#2-the-original-path)
- [3. The generated path](plugins/FwBassLift/MEASUREMENTS.md#3-the-generated-path)
- [4. Gain control of the generated path](plugins/FwBassLift/MEASUREMENTS.md#4-gain-control-of-the-generated-path)
- [5. The generated-signal limiter](plugins/FwBassLift/MEASUREMENTS.md#5-the-generated-signal-limiter)
- [6. The output stage](plugins/FwBassLift/MEASUREMENTS.md#6-the-output-stage)
- [7. Learned block](plugins/FwBassLift/MEASUREMENTS.md#7-learned-block)
- [8. The model against the reference](plugins/FwBassLift/MEASUREMENTS.md#8-the-model-against-the-reference)
- [9. Declined changes](plugins/FwBassLift/MEASUREMENTS.md#9-declined-changes)
- [10. Meters](plugins/FwBassLift/MEASUREMENTS.md#10-meters)
- [CPU](plugins/FwBassLift/MEASUREMENTS.md#cpu)
- [Provenance](plugins/FwBassLift/MEASUREMENTS.md#provenance)

## Flatwater BassX Lift

[plugins/FwBassXLift/MEASUREMENTS.md](plugins/FwBassXLift/MEASUREMENTS.md) (MaxxBass):

- [Setup](plugins/FwBassXLift/MEASUREMENTS.md#setup)
- [1. Structure, latency and the faders](plugins/FwBassXLift/MEASUREMENTS.md#1-structure-latency-and-the-faders)
- [2. The original path](plugins/FwBassXLift/MEASUREMENTS.md#2-the-original-path)
- [3. The generated path](plugins/FwBassXLift/MEASUREMENTS.md#3-the-generated-path)
- [4. The level control](plugins/FwBassXLift/MEASUREMENTS.md#4-the-level-control)
- [5. Learned block](plugins/FwBassXLift/MEASUREMENTS.md#5-learned-block)
- [6. The model against the reference](plugins/FwBassXLift/MEASUREMENTS.md#6-the-model-against-the-reference)
- [7. Declined changes](plugins/FwBassXLift/MEASUREMENTS.md#7-declined-changes)
- [8. Meters](plugins/FwBassXLift/MEASUREMENTS.md#8-meters)
- [9. Editor behaviour](plugins/FwBassXLift/MEASUREMENTS.md#9-editor-behaviour)
- [10. Wheel and Ctrl across the suite's references](plugins/FwBassXLift/MEASUREMENTS.md#10-wheel-and-ctrl-across-the-suites-references)
- [CPU](plugins/FwBassXLift/MEASUREMENTS.md#cpu)
- [Provenance](plugins/FwBassXLift/MEASUREMENTS.md#provenance)

## Flatwater Comp

[plugins/FwComp/MEASUREMENTS.md](plugins/FwComp/MEASUREMENTS.md) (H-Comp):

- [Setup](plugins/FwComp/MEASUREMENTS.md#setup)
- [1. Path and latency](plugins/FwComp/MEASUREMENTS.md#1-path-and-latency)
- [2. Detector](plugins/FwComp/MEASUREMENTS.md#2-detector)
- [3. Static curve and makeup](plugins/FwComp/MEASUREMENTS.md#3-static-curve-and-makeup)
- [4. Attack and smoothing](plugins/FwComp/MEASUREMENTS.md#4-attack-and-smoothing)
- [5. Release](plugins/FwComp/MEASUREMENTS.md#5-release)
- [6. Learned gain correction](plugins/FwComp/MEASUREMENTS.md#6-learned-gain-correction)
- [7. Mix, output and the limiter](plugins/FwComp/MEASUREMENTS.md#7-mix-output-and-the-limiter)
- [8. Analog stage](plugins/FwComp/MEASUREMENTS.md#8-analog-stage)
- [9. Tempo sync and determinism](plugins/FwComp/MEASUREMENTS.md#9-tempo-sync-and-determinism)
- [10. The model against the reference](plugins/FwComp/MEASUREMENTS.md#10-the-model-against-the-reference)
- [11. Declined changes](plugins/FwComp/MEASUREMENTS.md#11-declined-changes)
- [12. The VU needle and the limiter lamp](plugins/FwComp/MEASUREMENTS.md#12-the-vu-needle-and-the-limiter-lamp)
- [13. Editor behaviour](plugins/FwComp/MEASUREMENTS.md#13-editor-behaviour)
- [CPU](plugins/FwComp/MEASUREMENTS.md#cpu)
- [Provenance](plugins/FwComp/MEASUREMENTS.md#provenance)

## Flatwater Delay

[plugins/FwDelay/MEASUREMENTS.md](plugins/FwDelay/MEASUREMENTS.md) (H-Delay):

- [Setup](plugins/FwDelay/MEASUREMENTS.md#setup)
- [1. The kernel](plugins/FwDelay/MEASUREMENTS.md#1-the-kernel)
- [2. The glide](plugins/FwDelay/MEASUREMENTS.md#2-the-glide)
- [3. Loop gain and feedback](plugins/FwDelay/MEASUREMENTS.md#3-loop-gain-and-feedback)
- [4. Filters](plugins/FwDelay/MEASUREMENTS.md#4-filters)
- [5. LFO](plugins/FwDelay/MEASUREMENTS.md#5-lfo)
- [6. Sync, tapers and parameter resolutions](plugins/FwDelay/MEASUREMENTS.md#6-sync-tapers-and-parameter-resolutions)
- [7. Routing, mix and output](plugins/FwDelay/MEASUREMENTS.md#7-routing-mix-and-output)
- [8. Analog](plugins/FwDelay/MEASUREMENTS.md#8-analog)
- [9. Other rates](plugins/FwDelay/MEASUREMENTS.md#9-other-rates)
- [10. The model against the reference](plugins/FwDelay/MEASUREMENTS.md#10-the-model-against-the-reference)
- [11. Declined changes](plugins/FwDelay/MEASUREMENTS.md#11-declined-changes)
- [12. The editor](plugins/FwDelay/MEASUREMENTS.md#12-the-editor)
- [CPU](plugins/FwDelay/MEASUREMENTS.md#cpu)
- [Provenance](plugins/FwDelay/MEASUREMENTS.md#provenance)

## Flatwater Bass Leveler

[plugins/FwBassLeveler/MEASUREMENTS.md](plugins/FwBassLeveler/MEASUREMENTS.md) (Bass Rider):

- [Setup](plugins/FwBassLeveler/MEASUREMENTS.md#setup)
- [1. The level read](plugins/FwBassLeveler/MEASUREMENTS.md#1-the-level-read)
- [2. The gain law](plugins/FwBassLeveler/MEASUREMENTS.md#2-the-gain-law)
- [3. The note detector](plugins/FwBassLeveler/MEASUREMENTS.md#3-the-note-detector)
- [4. Response Fast re-rides](plugins/FwBassLeveler/MEASUREMENTS.md#4-response-fast-re-rides)
- [5. Timing, Idle and the start](plugins/FwBassLeveler/MEASUREMENTS.md#5-timing-idle-and-the-start)
- [6. Learned blocks](plugins/FwBassLeveler/MEASUREMENTS.md#6-learned-blocks)
- [7. Declined changes](plugins/FwBassLeveler/MEASUREMENTS.md#7-declined-changes)
- [8. The model against the reference](plugins/FwBassLeveler/MEASUREMENTS.md#8-the-model-against-the-reference)
- [9. Factory presets](plugins/FwBassLeveler/MEASUREMENTS.md#9-factory-presets)
- [10. The editor](plugins/FwBassLeveler/MEASUREMENTS.md#10-the-editor)
- [Provenance](plugins/FwBassLeveler/MEASUREMENTS.md#provenance)

## Flatwater Vocal Leveler

[plugins/FwVocalLeveler/MEASUREMENTS.md](plugins/FwVocalLeveler/MEASUREMENTS.md) (Vocal Rider):

- [Setup](plugins/FwVocalLeveler/MEASUREMENTS.md#setup)
- [1. The level read](plugins/FwVocalLeveler/MEASUREMENTS.md#1-the-level-read)
- [2. The vocal detector](plugins/FwVocalLeveler/MEASUREMENTS.md#2-the-vocal-detector)
- [3. The ride](plugins/FwVocalLeveler/MEASUREMENTS.md#3-the-ride)
- [4. Music side-chain](plugins/FwVocalLeveler/MEASUREMENTS.md#4-music-side-chain)
- [5. The model against the reference](plugins/FwVocalLeveler/MEASUREMENTS.md#5-the-model-against-the-reference)
- [Declined changes](plugins/FwVocalLeveler/MEASUREMENTS.md#declined-changes)
- [CPU](plugins/FwVocalLeveler/MEASUREMENTS.md#cpu)
- [Provenance](plugins/FwVocalLeveler/MEASUREMENTS.md#provenance)

## Flatwater Vocal Chain

[plugins/FwVocalChain/MEASUREMENTS.md](plugins/FwVocalChain/MEASUREMENTS.md) (CLA Vocals):

- [Setup](plugins/FwVocalChain/MEASUREMENTS.md#setup)
- [1. Input: Sensitivity and Balance](plugins/FwVocalChain/MEASUREMENTS.md#1-input-sensitivity-and-balance)
- [2. The fixed path (every colour clear)](plugins/FwVocalChain/MEASUREMENTS.md#2-the-fixed-path-every-colour-clear)
- [3. Bass and Treble](plugins/FwVocalChain/MEASUREMENTS.md#3-bass-and-treble)
- [4. Compress](plugins/FwVocalChain/MEASUREMENTS.md#4-compress)
- [5. De-essers](plugins/FwVocalChain/MEASUREMENTS.md#5-de-essers)
- [6. Reverb](plugins/FwVocalChain/MEASUREMENTS.md#6-reverb)
- [7. Delay](plugins/FwVocalChain/MEASUREMENTS.md#7-delay)
- [8. Pitch (the doubler)](plugins/FwVocalChain/MEASUREMENTS.md#8-pitch-the-doubler)
- [9. The wet sends](plugins/FwVocalChain/MEASUREMENTS.md#9-the-wet-sends)
- [10. Output and noise](plugins/FwVocalChain/MEASUREMENTS.md#10-output-and-noise)
- [Learned blocks](plugins/FwVocalChain/MEASUREMENTS.md#learned-blocks)
- [Declined changes](plugins/FwVocalChain/MEASUREMENTS.md#declined-changes)
- [The whole chain](plugins/FwVocalChain/MEASUREMENTS.md#the-whole-chain)
- [Factory presets](plugins/FwVocalChain/MEASUREMENTS.md#factory-presets)
- [CPU](plugins/FwVocalChain/MEASUREMENTS.md#cpu)
- [Open](plugins/FwVocalChain/MEASUREMENTS.md#open)
- [Provenance](plugins/FwVocalChain/MEASUREMENTS.md#provenance)

## Flatwater Pitch Bend

[plugins/FwPitchBend/MEASUREMENTS.md](plugins/FwPitchBend/MEASUREMENTS.md) (Vocal Bender):

- [Setup](plugins/FwPitchBend/MEASUREMENTS.md#setup)
- [Stage classes](plugins/FwPitchBend/MEASUREMENTS.md#stage-classes)
- [1. Mix](plugins/FwPitchBend/MEASUREMENTS.md#1-mix)
- [2. Modulation routing and depth](plugins/FwPitchBend/MEASUREMENTS.md#2-modulation-routing-and-depth)
- [3. M1 and M2: LFO and sequencer](plugins/FwPitchBend/MEASUREMENTS.md#3-m1-and-m2-lfo-and-sequencer)
- [4. AM follower](plugins/FwPitchBend/MEASUREMENTS.md#4-am-follower)
- [5. PT follower](plugins/FwPitchBend/MEASUREMENTS.md#5-pt-follower)
- [6. Modulation of the modulators' own controls](plugins/FwPitchBend/MEASUREMENTS.md#6-modulation-of-the-modulators-own-controls)
- [7. The shifter: no-shift path](plugins/FwPitchBend/MEASUREMENTS.md#7-the-shifter-no-shift-path)
- [8. The shifter: pitch, formant and grains](plugins/FwPitchBend/MEASUREMENTS.md#8-the-shifter-pitch-formant-and-grains)
- [9. The shifter: level](plugins/FwPitchBend/MEASUREMENTS.md#9-the-shifter-level)
- [10. The shifter: a moving pitch](plugins/FwPitchBend/MEASUREMENTS.md#10-the-shifter-a-moving-pitch)
- [11. Sample nulls with a shift](plugins/FwPitchBend/MEASUREMENTS.md#11-sample-nulls-with-a-shift)
- [12. Hard material (behaviour)](plugins/FwPitchBend/MEASUREMENTS.md#12-hard-material-behaviour)
- [Deliberate differences](plugins/FwPitchBend/MEASUREMENTS.md#deliberate-differences)
- [Declined changes](plugins/FwPitchBend/MEASUREMENTS.md#declined-changes)
- [Factory presets](plugins/FwPitchBend/MEASUREMENTS.md#factory-presets)
- [Numerics on every platform](plugins/FwPitchBend/MEASUREMENTS.md#numerics-on-every-platform)
- [CPU](plugins/FwPitchBend/MEASUREMENTS.md#cpu)
- [Open](plugins/FwPitchBend/MEASUREMENTS.md#open)
- [Provenance](plugins/FwPitchBend/MEASUREMENTS.md#provenance)
