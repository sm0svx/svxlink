# SvxLink tests

Unit/component tests for SvxLink and the bundled Async framework. They are
registered with CTest and require no special hardware, network, or sound card.

## Running

```sh
cmake -S src -B build
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

The TCL smoke test additionally needs `tclsh` and `python3` (both standard).

## What is covered

| CTest name | Area | Covers |
| --- | --- | --- |
| `CmdParserTest` | svxlink | command match/dispatch (prefix, exact, sub-command) |
| `DtmfDigitHandlerTest` | svxlink | DTMF command accumulation, anti-flutter, specials |
| `tcl_event_scripts_load` | svxlink | every TCL event script loads under tclsh |
| `CommonTest` | misc | common.h: setValueFromString, splitStr, SepPair |
| `ConfigTest` | async/core | Async::Config get/set, typed coercion, listSection |
| `AsyncMsgTest` | async/core | Async::Msg pack/unpack: scalars, string, vector, map |
| `StateMachineTest` | async/core | Async::StateMachine transitions + hierarchical dispatch |
| `AudioPipeTest` | async/audio | AudioAmp gain, AudioValve open/close, AudioSplitter fan-out |
| `AudioClipperTest` | async/audio | AudioClipper amplitude clamping |
| `AudioDelayLineTest` | async/audio | AudioDelayLine delays audio by the configured time |
| `AudioReaderTest` | async/audio | AudioReader synchronous pull reads |
| `AudioFilterTest` | async/audio | AudioFilter lowpass/highpass/bandpass response |
| `AudioFsfTest` | async/audio | AudioFsf frequency sampling filter passband/stopband |
| `DtmfDecoderTest` | trx | DTMF decode (pre-existing) |
| `DtmfEncoderTest` | trx | DtmfEncoder -> DtmfDecoder clean round-trip |
| `GoertzelTest` | trx | single-bin DFT: amplitude recovery, off-bin rejection |
| `ModulationTest` | trx | modulation fromString/toString round-trip |
| `TrxFactoryTest` | trx | RxFactory/TxFactory TYPE-driven creation + error cases |
| `SquelchTest` | trx | SquelchOpen always-open; SquelchVox thresholds |
| `SigLevDetTest` | trx | SigLevDetConst returns the configured constant level |
| `EmphasisTest` | trx | pre-emphasis boosts highs; pre+de recovers the signal |
| `ResamplerTest` | trx | AudioInterpolator x2 / AudioDecimator /2 + round-trip |
| `ReflectorMsgTest` | reflector | protocol message pack/unpack round-trip |

## Conventions

Each C++ test is a small standalone program that prints `ok`/`FAIL` lines and
exits 0 on success, 1 on failure — no test framework dependency. Audio-pipe
tests drive samples through a component and measure the captured output (RMS,
or per-tone level via a Goertzel filter) to assert frequency/gain behaviour.
