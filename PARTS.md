# Parts and inventory

Inventory status update: October 4, 2026. Mike reports much of the hardware ordered. The September 17 cart and prices below are historical candidates, not an as-built inventory or current purchase total. Starting scope: one pressure sensor, three current sensors (hot-water tank, UV light and pump), no flow sensor, two 12V solenoid valves, two illuminated pump/heater selectors and two illuminated flush buttons. Reconcile actual ordered/received models and quantities before assigning wiring.

## Working parts list from pasted cart

October 4 build direction: Mike accepts the ordered hardware and selected contactor ratings; purchasing/selection checks below remain historical. CT conditioning/ADS assembly wiring is Mike's work. Use CONNECTIONS.md to connect the selected parts and establish the firmware channel map; do not reopen contactor procurement for this stage.

D39 scope update: one installed control supply. No second supply, redundancy/ORing module, backup transfer connector, mechanical override contactor or extra selector contacts are required for failure recovery. Controller/supply failure is an accepted repair event. Existing DMWD selectors are candidates for input-only position sensing; the earlier combined coil-control/sensing limitation no longer applies. No replacement industrial selectors selected or ordered. Service spares are optional, not automatic BOM additions.

Prices below come from Mike's pasted cart, not live retailer quotes. Quantities incorporate his subsequent decision to replace the 16A AC-coil contactor with a third 25A DC-coil contactor. Product links, exact SKUs, certificates and manufacturer datasheets were not supplied for these listings. Listing claims such as TUV, waterproof, amperage and material suitability are not independently verified here. Listed quantity is not quantity still needed after stock reconciliation.

| ID | Candidate | Cart qty | Unit/package price | Extended | Review before ordering |
|---|---|---:|---:|---:|---|
| B01 | CSZD spiral cable wrap, 10 ft, 1/4 in | 1 | $5.99 | $5.99 | Optional wiring accessory; verify bundle diameter, material and suitability for the location. |
| B02 | International Connector T&G DIN rail, five 11 in lengths, 35mm wide x 7.5mm high | 1 | $14.89 | $14.89 | User selected 11-inch rails from screenshot; five-pack. Confirm mounting points, end stops and final component fit. |
| B03 | Heschen 25A, 2-pole 2NO, 12V DC coil | 3 | $14.49 | $43.47 | USER SELECTED: three matching DC-coil contactors. Use assumed 20A circuits and up to 1HP/240V pump for planning; exact model's motor/heater duty and coil demand remain to verify. |
| B04 | Heschen 16A, 2-pole 2NO, 12V AC coil — removed | 0 | $9.49 | $0.00 | Superseded by one additional B03. Original cart quantity was one; no longer part of the planned purchase. |
| B05 | Dinkle DK4N red/black 10-gang terminal assembly | 1 | $19.99 | $19.99 | User fixed count at ten independent hot-conductor positions: pump 4, heater 4, UV 2. Resolve power-supply branch using a suitable multi-connection terminal or rated internal splice; no extra terminal positions assumed. Verify assembly bridges, terminations and covers. |
| B06 | DIN supply, 12V DC 2A / 24W | 1 | $17.90 | $17.90 | HOLD FOR LOAD BUDGET: brand/model unidentified in pasted list. Verify isolation, ratings, derating and total simultaneous/pickup current. |
| B07 | ANMBEST 2-in/8-out distribution board | 1 | $13.99 | $13.99 | Propose low-voltage distribution only pending specifications; verify topology, aggregate rating, terminal guards and branch protection. |
| B08 | SazkJere SCT-013-020 20A/1V CTs, pack of 4 | 1 | $26.99 | $26.99 | Current-detection direction: heater, pump, UV and spare. Add appropriate signal conditioning and ADC; verify CT range, aperture and small UV-load performance. CircuitSetup is no longer the target interface. |
| B09 | DMWD 22mm three-position selector, red LED | 1 | $14.99 | $14.99 | B0DCHBC5Z1 inspected. Suitable contact pattern for D39 position-only sensing: commons to A16 ground, two NO terminals to separate inputs; NC unused. Verify actual pinout/continuity and LED voltage. No coil current through selector. |
| B10 | DMWD 22mm three-position selector, blue LED | 1 | $14.99 | $14.99 | Candidate for position-only sensing, same intended wiring as B09. Verify actual blue variant has matching action/pinout and suitable LED voltage. No extra contacts or interface planned. |
| B11 | BNTECHGO 12AWG silicone wire, red/black, 10 ft each | 1 | $14.98 | $14.98 | Do not treat as universal panel/mains wire. Verify insulation/listings, fine-strand terminal compatibility and circuit ampacity; lengths/colors may not cover needs. |
| B12 | Namunanee ABS enclosure, 16.5 x 12.6 x 6.1 in | 1 | $55.99 | $55.99 | HOLD FOR LAYOUT: outer dimensions only. Verify usable backplate/depth, material/environmental rating, door controls, glands and heat. |
| B13 | Beduan normally closed 1/2 in NPT 12V DC valve | 1 | $22.99 | $22.99 | Assign to spin flush or system flush after stock check. Verify pressure range/minimum differential, flow, continuous-duty coil, debris tolerance, seals and water-use suitability. |
| B14 | Bonsicoky brass terminal bars, six positions each, two-pack | 1 | $5.99 | $5.99 | User added from screenshot. Candidate only: conductor sizes, torque, ratings and suitability unverified. Ground capacity may need more than six positions; separately supplied circuit neutrals must not share a common bar here. |
| B15 | MECCANIXITY DIN rail PCB carrier, black, for 100 x 72mm board | 1 | $9.69 | $9.69 | User requested addition for ADS assembly. Listing specifies 35mm DIN rail and 1.5-2.0mm PCB thickness. Mount owned ADS1115 plus CT conditioning on matching interface/prototype board. Board inclusion unclear; verify before duplicate purchase. Overall carrier dimensions/rail footprint still to establish. |

Latest B05 screenshot specifies ten separate circuits, individual blocks 6.1 x 39.6 x 40.35mm, two SS2 end brackets and one end cover, supplied on a five-inch aluminum rail. Bare block row is 61mm/2.40 inches plus end hardware; the 1.59 x 1.56-inch metadata does not describe overall row length. Transfer blocks to the selected rail for a consolidated layout, or reserve the full five-inch supplied assembly footprint.

Ground/neutral update: DIN-mounted replacements are now preferred, exact models and prices pending. The following subtotal retains the old bar allowance for reference and must be revised when replacements are selected.

**Candidate-list subtotal including the new bar pack: $273.15**, before tax/shipping and stock adjustments ($271.75 with the previously pasted distribution-board coupon). Excluding the original $55.99 enclosure leaves a $217.16 non-enclosure candidate subtotal. Reusing the existing case would avoid a new enclosure cost; substituting the latest pictured $75.99 larger option would make the candidate total $293.15. Neither is a finalized shopping total: exact enclosure size/ownership mapping, other stock and missing parts remain unresolved. Three 25A contactors at $14.49 total $43.47. The CT row is one four-piece pack; the rail row is one five-piece pack; B14 is one two-piece pack.

The subtotal above is historical and excludes the subsequently added B15 carrier; per Mike's direction, running totals are not being maintained during selection. This is not a complete project cost. Stock, replacements and missing components will change it. Availability and delivery text from the pasted cart have not been treated as current.

## Inventory reconciliation

| Item | Current record | Next step |
|---|---|---|
| Standard KC868-A16 boards | Mike said on September 14 that he has several | Allocate one; record revision and physical dimensions |
| HiLetgo ADS1115 breakout modules | Mike confirms this model in stock; screenshot is a three-pack | Target one board using three of four inputs; validate sequential CT sampling. Remaining owned boards are fallback/spares, not baseline hardware. No ADC purchase planned. |
| Waveshare board | Prior conversation identified an ESP32-S3-POE-ETH-8DI-8DO from an image | Optional owned hardware; exact identity/role not reverified here |
| UV monitor | Working ESPHome-flashed Sonoff-style device discussed previously; exact installed model unknown | Identify model and whether to retain it |
| Spin-flush valve | Existing function confirmed September 17 | Identify voltage, coil load and reuse suitability |
| System-flush hardware | Existing function confirmed; specific installed hardware unspecified | Identify valve/actuator and reuse suitability |
| B01-B13 cart items | Some are reportedly already owned, but item-level stock unknown | Record owned quantity and condition against each ID |
| CircuitSetup meter/host | Superseded by CT + ADC direction | Excluded from active purchase plan |
| 22mm pushbuttons | Approximately two on hand from Ultra Lift, reported September 17 | Allocate to manual flushes after checking momentary action, contact blocks and illumination voltage; no duplicate purchase planned |
| 12-to-24V DC step-up converter | One on hand, reported September 17 | Preferred reuse candidate; check model, regulation, protection, current capability and mounting |
| Divided enclosure | Existing case owned with horizontal divider; latest purchase banner shows 13 x 9.2 x 5.6-inch variant | Earlier mapping to B12 size is no longer certain. Compare actual case with latest 18.1 x 12.6 x 6.4-inch selected option at pictured $75.99; title conflicts with selected dimensions. No replacement selected. |
| Sonoff room thermostat controller | Existing and used to maintain water-closet air temperature | Retain working control; identify model/integration if reusing temperature data. No duplicate room sensor planned. |
| Low-voltage DIN distribution | Existing hardware reported September 17 | Recommend reuse if suitable; exact model/topology and whether it duplicates B07 remain unknown |
| Pressure transmitter | Exactly one LEFOO T2000 owned; screenshot specifies 0-100 PSI, 4-20mA, 8-36VDC, G1/4 male, Hirschmann connector and 1m cable, accuracy +/-1% FS | Validate this unit before buying a second. Confirm physical label/pinout and loop supply margin; current LFT2000 family documentation differs at 10-36V. G1/4 requires matching parallel-thread fitting/seal, not assumed 1/4 NPT. |
| Pump/heater/UV equipment | Pump and hot-water tank: 240V, assumed 20A breakers; pump 3/4-1HP; Cactus Pure UV: 120V | Proceed with these user-directed assumptions; exact nameplates/model remain useful for final equipment checks |

For each final BOM row record: exact model/link, required quantity, usable owned quantity, quantity to buy, verified rating/interface, dimensions, price date and decision. Do not infer ownership from a cart.

## Missing or unresolved items

These are planning categories, not automatic additions to the order.

| Category | What remains to specify |
|---|---|
| Current measurement interface | Three initial-build CTs confirmed October 4: hot-water tank, UV light and pump. Reuse one owned HiLetgo ADS1115 with three conditioned channels. Still need per-CT bias/protection, connectors, low-voltage logic supply, mounting, I2C connection and sequential-sampling validation. Additional ADCs are fallback only. No CircuitSetup/host/AC voltage-reference purchase planned. |
| Flow measurement | Deferred from initial build by agreement. Preserve future input/space; FLOW_OPTIONS.md is reference only. No meter added to subtotal. |
| Pressure measurement | Test owned LEFOO T2000 0-100 PSI/4-20mA using owned 12-to-24V converter; Mike reports previous 12V/A16 attempt produced no reading. Match actual A16 input revision or provide suitable receiver/shunt; retain CT ADCs for current measurement. Add second transmitter only after successful test if filter differential is retained. |
| Pressure winterization/replacement | Mike declined the Ashcroft route on cost. Use the one owned LEFOO and test at 24V/known pressure before further spending. No replacement or second sensor purchase planned now. Winter survival remains unverified; previous test was mouth pressure only. |
| UV switching | Device appropriate for ballast load/inrush and chosen normal operating policy |
| Two valve circuits | Reuse versus replacement count; suitable interface/suppression and individual protection |
| Manual flush controls | Two illuminated pushbuttons selected October 4: spin flush and UV/system flush. All requests go through A16. All four door-control LEDs confirmed 12V DC; terminal assignments are in CONNECTIONS.md. |
| Indication | Desired power/enable/running/fault indications; identify what each LED actually means |
| Electrical protection/disconnection | Circuit protection, disconnect arrangement, touch-safe covers and all-sources labels |
| Distribution and wiring | Ground/neutral provisions as applicable; separate AC and DC terminals, branch fusing, suitable conductors, ferrules/lugs as required by terminals |
| Enclosure assembly | Rail mounts, controller standoffs, end clamps, wire duct, markers, cable glands/strain relief and door harness |
| Plumbing/drains | Water-compatible fittings, valve unions/service access, permitted drain connection/backflow arrangement and freeze exposure |
| Optional additions | Simple leak detector with compatible dry-contact interface, alarm-only proposal; placement/model unselected. UV temperature sensing excluded from initial build. No main isolation valve planned. |

## Selection sequence

Ground/neutral candidate under review (not selected or added to subtotal): PZRT seven-position brass bus bar with insulated DIN base, pictured at $8.99 each ($17.98 for two). Screenshot explicitly specifies a snap-in mount for 35 x 7.5mm rail, a 50mm bar length, 6mm holes and M5 screws. All seven connections share one bar. Image appears to place the bar across the rail, so 50mm is not established as the occupied length along the rail; obtain overall base dimensions for layout. No manufacturer current/voltage rating, conductor range, torque specification or grounding-use certification was verified. The mechanical style matches the requested arrangement; electrical suitability and connection count including bonding remain unresolved.

1. Reconcile stock and equipment ratings.
2. Use three B03 contactors with 12V DC coils; B04 is removed. Verify B03 motor duty using the agreed planning assumptions and match valves/interfaces to their loads.
3. Select monitoring package and sensors as compatible sets.
4. Calculate power and thermal budgets.
5. Lay out actual component footprints and door depth.
6. Price the remaining exact quantities and produce the final order list.
