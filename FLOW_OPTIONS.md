# Optional flow measurement

Reviewed September 17, 2026. No meter selected or added to the purchase subtotal.

## Recommendation

Mike agreed to defer the flow meter from the initial build. Retain timed/manual flush capability and allow a future flow input and serviceable plumbing location. The product research below is reference material for a later enhancement, not an active procurement task.

Mike prefers vertical mounting and confirmed 1-inch copper pipe, with push-to-connect (SharkBite-type) adapters envisaged. Upward versus downward flow remains unknown. He is open to a 3/4-inch device if revisited. Use approximately 6 gpm for comparisons, not as a verified flow ceiling; this supersedes the earlier unresolved per-hour recollections for preliminary sizing. Size by performance and pressure drop rather than connection size alone.

## What it adds

| Use | Value | Limitation |
|---|---|---|
| Total gallons | Track use and maintenance throughput | Counts only branches downstream of its location |
| Usage-triggered filter purges | Flush after measured throughput rather than time alone | Timed purging remains possible without it |
| Abnormal sustained consumption | Better evidence of unexpected water use than pump current alone | Low-flow sensitivity, pulse resolution and intended flushes affect interpretation |
| Flow during a commanded flush | Confirms movement along the metered path if enough volume passes | Does not prove flow in a branch that bypasses the meter or distinguish cabin demand without context |
| Recent use for UV refresh logic | Can show that water has recently passed through the UV path | No pulse does not prove no flow; use a fallback interval or suitable temperature strategy |

Current shows a pump motor operating, not gallons delivered. Pressure shows hydraulic conditions, not how much water crossed the UV chamber. Neither substitutes for flow measurement, but both remain useful without it.

## Pictured meter

The screenshot identifies **DAE AS250U-100P, 1-inch, pulse output**, displayed at $99.99. Manufacturer documentation specifies horizontal mounting with the dial up and **10 gallons per pulse**. At an illustrative 2 gpm, successive pulses are five minutes apart. The local dial has finer resolution than the remote pulse output. This is a poor match for short-flush feedback or prompt flow detection. [F01]

## Vertical candidates

Manufacturer web prices checked September 17; before shipping/tax. These are alternatives, not additions to the BOM.

| Candidate | Price | Mounting | Remote resolution | Assessment |
|---|---:|---|---|---|
| DAE V-100P, 1 inch | $126.99 | Vertical, upward flow only | 1 gallon/pulse | Least expensive listed vertical DAE option; verify actual flow direction. Manufacturer datasheet says not NSF61 certified. [F02] |
| DAE VM-100P, 1 inch | $159.99 | Any orientation, including vertical up/down | 1 gallon/pulse | Flexible mounting; internal non-return valve, possible mechanical clicking; 12.5 inches with couplings. [F03] |
| DAE PD-100, 1 inch | $209.00 | Any orientation, including vertical up/down | 1 gallon/pulse | Manufacturer lists NSF61 certification; 15.5 inches with couplings and about 11 lb 11 oz. Best candidate here if adding a documented potable-water model and its size/cost are acceptable. [F04] |

DAE lists these models for clean city water; this is a well system. Confirm suitability for the actual filtered water before choosing one. The listed V/VM minimum flow or PD minimum test flow is 0.75 gpm: do not promise detection of drips or very slow leaks from the one-gallon pulse specification. [F02-F04]

At an illustrative 2 gpm, one gallon/pulse produces a pulse every 30 seconds, so even these models may produce no pulse during a short cooling flush. Do not use their pulse count as the sole short-flush proof or sole cooling safeguard.

Other products screened: EKM VSPWM-075-HD-NSF offers upward vertical mounting and approximately 0.075 gallon/pulse at $205, in 3/4 inch. Assured Automation WM-PD offers optional 20 pulses/gallon in 1/2- and 3/4-inch sizes. These remain eligible if pressure drop, materials, orientation and outputs meet the design; 3/4 inch is not excluded solely because the pipe is 1 inch. [F05, F06]

## 3/4-inch sizing and filter condition

At an assumed 6 gpm, a suitable 3/4-inch meter can be reasonable in a 1-inch copper line with appropriate reducers. A short smaller device is not equivalent to replacing the entire pipe run with smaller pipe. Assess the exact meter's restriction, fitting losses, peak simultaneous demand and available downstream pressure.

For example, DAE's VM-75P datasheet lists 0.54 psi loss at 5 gpm and 2.60 psi at 10 gpm. This supports considering that size near the proposed flow; it is not a measured 6 gpm value or a guarantee for other meters. [F10]

Filters may be the largest restriction, but pressure drops add. A 6 gpm treatment rating is not necessarily a physical flow limiter; actual flow depends on pressure, demand and total resistance. Additional meter restriction still matters.

Pressure difference across filters under repeatable flowing demand is useful for tracking clogging. Static readings alone are insufficient; sensors around the full bank identify total restriction, not the individual cartridge responsible. Carbon/tannin media can lose treatment capacity without a major pressure increase. Gallon totals can inform service estimates but do not directly prove remaining capacity. Use the actual cartridge's service guidance and relevant water-quality checks alongside pressure monitoring. Pentair separately identifies capacity and inadequate flow as replacement criteria; its example interval is not adopted for the cabin's unidentified cartridges. [F11]

## Ultrasonic alternatives — September 17 follow-up

Mike asked about ultrasonic sensors. Distinguish an external clamp-on sensor from an inline ultrasonic meter; the latter still requires cutting the pipe, but has a factory-defined measuring passage. Flow remains optional and no product has been selected.

**Clamp-on:** avoids cutting/wetted fittings and introduces no hydraulic restriction. Select against actual pipe material, outside diameter and wall thickness, not just nominal size. Installation quality, full-pipe conditions, bubbles, sensor coupling and nearby flow disturbances affect performance. Follow the particular sensor's straight-run requirements. Industrial products such as ONICON F-4300 and KEYENCE FD-Q establish that this is a practical approach; neither has been priced or sized as a final cabin selection. [F07, F08]

The important limit for this project is reliable low-flow detection and response time, not a headline accuracy percentage. For example, the KEYENCE FD-Q32C defaults to a 5 L/min zero cutoff (about 1.32 gpm), adjustable with appropriate zero setup; its listed repeatability is a percentage of full scale and varies with response-time setting. Do not equate that with absolute accuracy at a slow trickle. Exact copper-tube fit and interface remain to verify. [F08]

**Inline candidate: DAE U-100b.** Manufacturer listing is $201.30; 1-inch stainless body/couplings, claimed NSF61 certification, any orientation including vertical up/down, and RS485 Modbus RTU. Published normal-range accuracy is +/-1.5%; the datasheet lists a 0.75 gpm minimum test flow. It displays gallons/GPM. This is now a stronger candidate for investigation than the coarse mechanical pulse options, given Mike is willing to cut the copper. [F09]

The potential advantage is obtaining flow/volume via digital readings instead of waiting for a mechanical gallon pulse. Before selection obtain the register map, supported flow/total fields, actual measurement refresh time and communication power requirements. Confirm filtered-well-water suitability (the specification says clean water), installation clearances and low-flow cutoff. A16 compatibility is plausible via RS485/ESPHome, not yet demonstrated. No extra cloud gateway is planned. This research does not establish that it will reliably verify a short UV cooling flush.

Comparison with the earlier mechanical recommendation: flow is still optional; if added, investigate the U-100b before choosing the PD-100. Keep clamp-on as an alternative if avoiding pipe work becomes a priority. Do not buy an unspecified inexpensive clamp-on device solely on a claimed accuracy number.

## Fit and connection work if a meter is selected

- Match push-to-connect adapters to the actual supplied NPT coupling ends; meter-body union threads are not automatically pipe threads. Lay out the assembled length before cutting copper.
- Retain accessible meter unions and support the assembly, particularly the heavier PD model.
- Follow the chosen model's orientation, flow arrow, full-pipe and inlet/outlet installation requirements; vertical mounting does not imply either flow direction is allowed.
- Prefer a filtered common path if the purpose is treated-water use/UV flow. The precise location depends on the system-flush tee, which remains unrecorded. A downstream meter will not count the spin-filter waste branch upstream.
- The DAE reed outputs are passive dry contacts; no DAE cloud gateway is needed for a locally engineered pulse input. Match the A16 input circuit to the meter's switch voltage/current limits, debounce and verify counting; no terminal wiring is assigned here.
- Keep known automatic-flush usage separate from unexplained-use alarms. Missing/stalled counts are not proof of zero flow.

## Sources

- **F01:** [DAE AS250U-100P](https://daecontrol.com/product/dae-as250u-100p-1-water-meter-with-pulse-output-measuring-in-gallon-coupling/). Orientation, 10 gallons/pulse and reed limits; screenshot supplies the $99.99 comparison price.
- **F02:** [DAE V-100P product](https://daecontrol.com/product/dae-v-100p-1-vertical-water-meter-with-pulse-output-measuring-in-gallon-couplings/) and [manufacturer datasheet](https://daecontrol.com/wp-content/uploads/2025/07/V-100P-Datasheet-1.3g.pdf).
- **F03:** [DAE VM-100P](https://daecontrol.com/product/dae-vm-100p-1-positive-displacement-water-meter-with-pulse-output-measuring-in-gallon-couplings/).
- **F04:** [DAE PD-100](https://daecontrol.com/product/dae-pd-100-1-lead-free-potable-positive-displacement-water-meter-1-npt-couplings-pulse-output-gallon/).
- **F05:** [EKM vertical HD pulse meter](https://www.ekmmetering.com/products/3-4-vertical-water-nsf).
- **F06:** [Assured Automation / Flows.com WM-PD](https://www.flows.com/wm-pd-series-low-flow-water-meter/).
- **F10:** [DAE VM-75P datasheet](https://daecontrols.com/downloads/VM-75P%20Datasheet%202.0.pdf). Specific 3/4-inch meter pressure-drop values.
- **F11:** [Pentair cartridge-change guidance](https://www.pentair.eu/sites/default/files/collaterals/pdfs/IO-Guide_86489.pdf). Capacity and inadequate-flow criteria; pressure assessment during operation.
- **F07:** [ONICON F-4300](https://www.onicon.com/products/f-4300-series-clamp-on-ultrasonic-flow-meter) and [application/ordering guide](https://onicon.com/wp-content/uploads/2022/04/f-4300-application-and-ordering-guide-doc-0003269.pdf). Clamp-on architecture and pipe-specific transducer selection.
- **F08:** [KEYENCE FD-Q series](https://www.keyence.com/products/process/flow/fd-q/) and [FD-Q32C specifications](https://www.keyence.com/products/process/flow/fd-q/models/fd-q32c/). Zero cutoff, response time, full-scale repeatability and liquid/pipe constraints; candidate family only.
- **F09:** [DAE U-100b product](https://daecontrol.com/product/dae-u-100b-1-non-lead-stainless-ultrasonic-flow-meter-with-rs485-communication-ip68-npt-couplings-k-gallons-gpm/), [manufacturer datasheet](https://daecontrol.com/wp-content/uploads/2025/09/U-100b-Datasheet-1.6.pdf), and [manufacturer price listing](https://daecontrol.com/product-category/water-meters/u-series/1-inch/). Price observed September 17, before tax/shipping.
