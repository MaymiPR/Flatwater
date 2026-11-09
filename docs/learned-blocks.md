# Learned blocks

How a Flatwater plug-in replaces one stage of the reference that resists identification with a small network of our
own weights, while every identified stage around it stays exact. The procedure applies only to a behaviour that is
itself program-dependent, after hidden design values and structural finds have been tried.

## Where the suite uses one

| Plug-in | Block | Network | Trainer | Exporter | Header |
|---|---|---|---|---|---|
| Flatwater Bass Lift | the gain of the created bass | GRU, 32 units | `plugins/FwBassLift/nulltest/train_net.py` | `plugins/FwBassLift/scripts/export_net.py` | `plugins/FwBassLift/Source/dsp/GainNet.h` |
| Flatwater BassX Lift | the harmonics compressor (Ratio, Response) | GRU, 48 units, trained end to end through the identified recursion | `plugins/FwBassXLift/nulltest/train_net.py` | `plugins/FwBassXLift/scripts/export_net.py` | `plugins/FwBassXLift/Source/dsp/Net.h` |
| Flatwater Comp | a gain correction over the plug-in's own states | GRU correction | `plugins/FwComp/nulltest/train_net.py` | `plugins/FwComp/scripts/export_net.py` | `plugins/FwComp/Source/dsp/Net.h` |
| Flatwater Vocal Comp | the slow path of the level control | GRU, 48 units | `plugins/FwVocalComp/nulltest/train_net.py` | `plugins/FwVocalComp/scripts/export_net.py` | `plugins/FwVocalComp/Source/dsp/Net.h` |
| Flatwater Vocal Chain | a gain correction of Compress, two nets per colour averaged | GRU, 32 units | `plugins/FwVocalChain/nulltest/train_compress.py` | `plugins/FwVocalChain/scripts/export_net.py` | `plugins/FwVocalChain/Source/dsp/CompressNet.h` |

Each plug-in's MEASUREMENTS.md holds the block's structure, target, dataset, results and cost.

## Procedure

1. **Make the structure around the block exact.** Every identified stage before and after the block nulls on its
   own; the block's inputs and outputs are signals the model computes exactly. The offline renderer dumps them
   (`VcRender dump=1` writes the detector level, the fast ratio and the fitted slow ratio per sample).
2. **Extract the target exactly from the reference.** Where the stage is a pure gain, the reference's own gain sample
   by sample: output / delayed input / fixed gain. Where a module has an amount control, its fully wet equivalent.
   The target comes from the reference's output only, never from any of its files.
3. **Measure the ceiling.** Scale the model's output by the target's gain per short block (2 ms and 10 ms) and null
   it; the gap between the model and that ceiling is what a gain correction can win. A gap of a few dB does not pay
   for a network.
4. **Render the dataset.** Thousands of short captures (2 to 4 s) of the reference with only that module on, across
   every setting and level: the material the plug-in is used on, plus very quiet input and plenty of steady tones and
   level steps, since a network behaves only where it has seen data (`plugins/<Id>/nulltest/render_trainset*.py`). Pieces above
   +4 dBFS peak are left out. Keep a held-out set (every 7th piece, or a random 10 % after the fixed grid and preset
   cases) and a separate validation set that neither training nor checkpoint choice sees, at several input levels.
5. **Ablate the features on a short run first.** Give the network the signal the reference listens to (Flatwater
   BassX Lift: the high-passed input, worth 14 dB); an explicit hand-built detector was 8 dB worse than letting the
   network read the signal.
6. **Train on the GPU**, one process per output file, with checkpoints that resume (`requirements-train.txt`; a CUDA
   build of torch for the GPU). Supervise on the extracted gain first, then fine-tune on the null residual itself:
   training on the residual alone stalls where the block sits behind a `min()` (no gradient while it is inactive).
   Losses: the per-example residual in dB; corrections trained on the weak cases with a penalty on any case more
   than 0.5 dB worse (a mean over all cases drowns the weak ones); gradient clipping; steps with NaN skipped. When
   the structure is recursive, train end to end through it: the identified recursion and its exact adjoint in C as
   an autograd function, the fixed filters as FFT convolutions (Flatwater BassX Lift: `plugins/FwBassXLift/nulltest/generator.c`).
7. **Add the guards.** The block must not spoil what the identified model already nulls: a floor at the static law's
   ratio of the loudest level of the last second, and no correction while the target is flat for 20 ms (the
   correction fades out over 20 ms). Without them a network costs 10 to 60 dB on steady tones and level steps.
8. **Export to a header** marked "Do not edit by hand" in the plug-in's folder: `inline constexpr float` arrays with
   `%.9g` literals, the PyTorch GRU row layout kept, the exporter's numerics equal to the runtime's. The block runs
   on a fixed virtual clock (12 kHz with a phase accumulator, or every 16 samples) so it behaves the same at every
   host rate, and each sample takes the last two finished steps' values (causal).
9. **Check C++ against PyTorch** on saved pieces at two block sizes (512 and 37): -70 dB or lower before anything
   else runs (a correct port reads far lower).
10. **Run the full checks**: the null suite, the settings grid, other sample rates, `ctest`, pluginval at
    strictness 10 and the render regression.
11. **Judge on the validation set**, per single example, each null floored where it stops mattering (-40 dB). When
    every training regresses on a different example, that is variance between trainings: averaging two nets'
    corrections removes it (Flatwater Vocal Chain's Compress, Spank: worst single vocal +2.0 to +5.2 dB for each of
    three nets, +0.8 for two of them averaged), where shrinking the correction, a stronger no-worse penalty and
    choosing the checkpoint on regressions do not. A regression in every training is a bias: add data at those
    settings.
12. **Compare against the plug-in's real fitted stage**, not a fresh estimate, on the same cases. The block ships
    only when it improves the held-out null and its CPU cost is paid for.
13. **Measure the cost**: CPU seconds per 60 s of stereo at 48 kHz through the offline renderer, with and without the
    block, reported beside its null in MEASUREMENTS.md.
14. **Retrain** whenever the analytical model's numerics change.

## Gates

| Gate | Pass |
|---|---|
| Structure | every stage around the block nulls on its own |
| Target | extracted from the reference's output, sample by sample |
| Steady-tone guards | steady tones and level steps null as without the block |
| Held-out set | the held-out null improves; no validation example more than 0.5 dB worse where it can be heard |
| Port | C++ equals PyTorch at -70 dB or lower at block sizes 512 and 37 |
| Cost | CPU seconds per 60 s reported beside the null |
