# Three new MNQ studies — none of them intraday momentum

Built 2026-11-11. Three Sierra Chart ACSIL studies for MNQ, day session only, flat by 15:55 New York. Each uses a different source of edge from the nine legs of `MomentumSuite_NQ_MNQ.cpp`.

| File | Idea | Entry | Exit |
|---|---|---|---|
| `DailyWashoutRebound_NQ_MNQ.cpp` (DWR) | Cross-day mean reversion, long only | 2-period RSI of the last day-session closes below 30 → buy the 09:30 open | Stop 0.75 × average day range; 15:55 |
| `ValueAreaReentry_NQ_MNQ.cpp` (VAR) | Auction theory, the "80% rule" | Open outside yesterday's value area, then a 30-minute close back inside (10:00–12:30) → trade towards the far side | Stop 0.20 × range; 15:55 |
| `CompressionExpansionBreakout_NQ_MNQ.cpp` (CEB) | Volatility cycle (NR4) | Only after a narrow-range day: first 5-minute close outside the 30-minute opening range, before 13:00 | Stop 0.25 × range; 15:55 |

**Status: not built or run in Sierra Chart.** The files were run in the same mock of Sierra as the momentum suite and match an independent Python model trade for trade. All figures are from that mock on data ending 2024-03-07. Sierra's documentation site could not be reached from this session, so no API call was re-checked online; the order layer is the momentum suite's, unchanged.

**Headline: 1 MNQ per script made $331 a month in 2017–2024, with a −$3,568 drawdown. That is a third of the $1,000 target.** See Part F for what that means.

---

## Part A — Research findings

**Data and method.** Same NQ and ES 1-minute data as before (2016-08 → 2024-03, by contract), this time for the full 23-hour session and back-adjusted into a continuous series the way a Sierra continuous chart is. Costs $2.50 per MNQ round trip. Choices made on 2017–2021, checked on 2022–2024. Nothing after March 2024 exists in this data; "optimised for the current market" is therefore not something I can claim.

**Books used** (this repository): Kaufman, *Trading Systems and Methods* (RSI and overbought/oversold, Market Profile and price distribution, Crabel's narrow-range and opening-range work); Chan, *Quantitative Trading* and *Algorithmic Trading* (mean reversion at the daily horizon, pairs); Tsay, *Analysis of Financial Time Series* (volatility clustering); *Statistical arbitrage pairs trading strategies* (spread z-scores); López de Prado and Bailey et al. (overfitting — why every setting below is a plateau value and the number of ideas tried is stated); Aronson & Masters (out-of-sample confirmation); Carver (volatility-based stops and sizing).

**External research re-tested:** Baltussen, Da, Lammers, Martens (2021), *Hedging demand and market intraday momentum* (rest-of-day return predicts the last 30 minutes); Boyarchenko, Larsen, Whelan, *The Overnight Drift*; Lou, Polk, Skouras (2019), *A tug of war* (overnight vs intraday return components); Grant, Wolf, Yu (2005) (reversals after large opening moves).

**What the data said (about 30 new ideas; with the earlier 45, about 75 in total)**

| Idea | 2017–21 PF | 2022–24 PF | Verdict |
|---|---|---|---|
| **Buy the open after a daily washout** (RSI2, low close in range, down streaks, new 5/10-day low) | 1.4–2.0 | 1.1–1.5 | **Built (DWR).** Every definition positive in both periods |
| **Open outside prior value area, re-entry, hold to close** | 1.2–1.4 | 1.1–1.4 | **Built (VAR).** Weak but consistent; also on ES |
| **Opening-range close-break after a narrow-range day** | 1.3–1.5 | 1.3–1.9 | **Built (CEB).** NQ only |
| Sell the open after a daily run-up (mirror of DWR) | 0.6–1.2 | 0.6–1.2 | Rejected |
| Fade the 08:30–09:30 pre-market move at the open, exit 10:00 | 1.0–1.3 | 1.2–1.5 | Held back: uncorrelated, but flat in 2017–21 with any stop and negative on ES |
| Rest-of-day return → last 30 minutes (Baltussen) | 1.0–1.2 | 1.0–2.0 | Rejected: weak before 2022, and it is momentum |
| Overnight session, fade or follow the noise band | 0.9–1.1 | 0.8–1.2 | Rejected both ways |
| Time-of-day drift (48 half-hour slots, incl. the 02:00–03:00 "overnight drift") | — | — | Nothing stable |
| NQ-vs-ES hedged spread, momentum or reversion | 0.4–1.3 | 0.5–1.1 | Rejected: costs of two legs |
| 1–15-minute shock reversal | 0.75–0.98 | 0.75–1.04 | Rejected |
| Value-area edge fade, POC magnet, initial-balance fade | 0.7–1.1 | 0.7–1.2 | Rejected |
| Gap fill by gap size, extreme opening-move reversal, late-day VWAP reversion | 0.4–1.1 | 0.5–1.4 | Rejected |
| Volume-climax reversal, sweep-and-reclaim of the day's extreme | 0.6–1.3 | 0.5–1.3 | Rejected |
| Turn of month, weekday, opex Friday | — | — | Rejected (Monday long is positive in both periods, but it is one of five weekdays tried) |
| Last 5 days' intraday returns → fade today (tug of war) | 1.14–1.25 | 1.20–1.24 | The long half is DWR; the short half fails on ES |

The pattern matches the earlier finding: **inside the day, NQ rewards following and punishes fading. Mean reversion only shows up across days** (DWR) **or against a reference from the previous day** (VAR).

---

## Part B — Three architectures

| | A: Cross-day mean reversion (DWR) | B: Auction / value-area re-entry (VAR) | C: Volatility compression → expansion (CEB) |
|---|---|---|---|
| Edge proposed | Short-term selling in index futures overshoots; buyers return the next session | An open outside yesterday's value that cannot hold is a failed auction; price rotates back through value | Range contracts and expands in cycles; the first directional move after a quiet day carries further |
| Trades when | RSI(2) of daily closes < 30, about 1 day in 4 | Open outside value and a 30-minute close back inside, about 1 day in 4 | Yesterday NR4, about 1 day in 4 |
| Direction | Long only | Both (profit was on the short side) | Both |
| Works when | Pullbacks in rising or choppy markets; rebounds in volatile years | Gap opens that fail | Quiet days followed by trend days |
| Fails when | Selling continues for days (worst month −$2,700) | Open outside value that drives away; low-range days | Whipsaw around the opening range; quiet day follows quiet day |
| Automation | Market at the open, one stop | Market at a 30-minute mark, one stop | Market at a 5-minute close, one stop |

---

## Part C — Quantitative evaluation

The three `.cpp` files in the mock, 2017-01-01 → 2024-03-07, 1 MNQ each, $2.50 per round trip.

| Metric | DWR | VAR | CEB | All three |
|---|---|---|---|---|
| Total net profit | $15,706 | $5,332 | $7,768 | $28,806 |
| Average monthly net | $181 | $61 | $89 | $331 |
| Median monthly net | $90 | −$20 | $22 | $123 |
| Best / worst month | $1,968 / −$2,700 | $1,157 / −$458 | $1,618 / −$861 | $2,752 / −$1,057 |
| Profitable / losing months (of 87) | 55 / 30 | 41 / 45 | 47 / 40 | 52 / 35 |
| Total trades | 465 | 450 | 412 | 1,327 |
| Win rate | 54.8% | 33.8% | 43.7% | 44.2% |
| Profit factor | 1.38 | 1.27 | 1.42 | 1.36 |
| Expectancy per trade | $33.78 | $11.85 | $18.86 | $21.71 |
| Maximum drawdown | −$3,546 | −$1,554 | −$1,318 | −$3,568 |
| Longest losing streak (expected for the loss rate) | 7 (8) | 13 (15) | 8 (10) | 12 |
| Average win / loss | $223 / −$196 | $165 / −$66 | $146 / −$80 | $184 / −$107 |
| Trades per day | 0.25 | 0.24 | 0.22 | 0.71 |
| Costs included | $1,162 | $1,125 | $1,030 | $3,318 |
| 2017–21 PF, $/month | 1.49, $146 | 1.20, $37 | 1.36, $63 | 1.38, $247 |
| 2022–24 PF, $/month | 1.30, $264 | 1.36, $118 | 1.49, $151 | 1.35, $533 |
| At $5.00 / $7.50 per round trip | $167 / $154 | $48 / $35 | $77 / $66 | $293 / $255 |
| t-statistic of the trade mean | 2.6 | 1.7 | 2.5 | 3.9 |
| Same rule on ES (PF 2017–21 / 2022–24) | 1.09 / 1.14 | 1.15 / 1.24 | **0.69** / 1.78 | — |

**By year, all three:** 2017 +$426 · 2018 +$1,047 · 2019 +$868 · 2020 +$7,522 · 2021 +$4,974 · 2022 +$8,070 · 2023 +$5,020 · 2024 (to 7 March) +$882. No losing year, but 2017–2019 made almost nothing.

**Correlation of daily P&L:** DWR–VAR 0.00, DWR–CEB 0.07, VAR–CEB 0.22. Against the momentum suite: DWR 0.14, VAR 0.29, CEB 0.25. All three trade on the same day on 16 days in seven years.

**Monte Carlo, one year, 5-day blocks:** 9% of years lose; median +$3,771; 5th percentile −$808; drawdown median −$1,749, 95th percentile −$3,814, 99th −$5,074.

**Exits tested on the final rules:** targets of 0.5 and 1.0 × range, breakeven after +0.25 and +0.5, trailing 0.5 and 1.0. None improved a script in both periods. So the scripts use one attached stop and the 15:55 exit, and never modify an order.

**Tested versus hypothetical.** Tested: everything above, on 2017–2024 data, in a mock. Not tested: any date after 2024-03-07; fills in Sierra; slippage at the 09:30 open, where DWR sends a market order.

---

## Part D — Selection

All three are kept as separate scripts because their returns are nearly uncorrelated and each has a different trigger. Ranked by evidence:

1. **DWR** — the strongest. Six definitions of the same idea all work, both periods, NQ and ES. Its risk is concentrated: long only, a wide stop, and a worst month three times the others'.
2. **CEB** — good numbers on NQ in every setting, but it fails on ES in 2017–2021, and it is the one closest in kind to the momentum suite.
3. **VAR** — the weakest. A t-statistic of 1.7, a negative median month, and all the profit on the short side. It earns its place for being consistent across settings and markets, not for size.

---

## Part E — Implementation

Each `.cpp` is self-contained and holds the three steps: **Step 1** `Orders` = *Signals only* (engine, lines, on-chart TALLY); **Step 2** `Orders` = *Send orders* (one market entry with an attached stop, fill detection, exits, limits); **Step 3** the hardening already in the order layer (chart and feed checks, recalculation handling, orphan positions, stop-coverage check, flatten retries, CME short sessions).

They are generated from `research/gen_rev` (one shared engine, one shared order layer, a 30-line decision function each). The order layer is the momentum suite's with one fix (below). The engine is new in three places: it can decide on the bar that ends at 09:30 so DWR enters at the open; it keeps a volume-by-price histogram for the value area; it keeps the RSI and narrow-range state.

**How it was tested**

| Check | DWR | VAR | CEB |
|---|---|---|---|
| Trades, mock vs Python model | 470 / 471 | 452 / 452 | 418 / 418 |
| Same day, side, entry price, P&L | all 470 | all 452 | all 418 |
| Recalculations injected (two intervals) | identical | identical | identical |
| Called once a minute / every call doubled | identical | identical | identical |
| AddressSanitizer + UBSan | clean | clean | clean |
| Invariant violations (3.1 M calls) | 0 | 0 | 0 |
| `-Wall -Wextra` warnings in the study | 0 | 0 | 0 |

(Counts include the 2016 warm-up months. The one DWR difference is Good Friday 2017, where the data has a gap and the study correctly refused a late entry.)

Faults injected on DWR: wrong time zone (no entries), 12 failed flattens (re-issued, MANUAL ACTION line), a foreign position (flattened), risk-% sizing, $ cap per trade, daily loss halt, signals-only mode.

**A bug found in the shared order layer:** when a cancel had to be repeated, the stored reason text was copied onto itself (`snprintf` with overlapping buffers, undefined behaviour). Fixed in `gen_rev/tmpl_trader.inc`. The same line is still in the three momentum single-strategy files and their template.

---

## Part F — Deliverables, sizing and the $1,000 question

### Is $1,000 a month supported?

**Not by these three alone.**

- Measured: **$331 a month for 1 MNQ per script**, at index levels averaging about 10,500.
- If the same edge in basis points held at NQ 25,000, that would be about $700 a month (range $600–$940 using the 2017–21 or 2022–24 rate; about $150 at the worst year's rate). Drawdowns scale the same way: about −$8,500 where history shows −$3,568. This is an extrapolation, not a result.
- 3 MNQ per script reaches $1,000 a month on the measured rate, with a historical drawdown near −$10,700 and a 99th-percentile year near −$15,000 before any index-level scaling.

**Together with the momentum suite** (same mock, same data, 1 MNQ per leg): suite alone $1,226 a month, drawdown −$4,023; suite plus these three **$1,557 a month, drawdown −$5,186, worst month −$1,797 (suite alone −$2,386), 61 of 87 months positive (suite alone 56)**. The new three add about a quarter to the income and soften the worst month. That is their real use: diversifiers, not a replacement.

**Account size, these three at 1 MNQ each** (Monte Carlo, historical index level): with $10,000 a drawdown of a quarter of the account happens in 23% of years; with $25,000 in 0.2%. At today's index level, double the dollar figures.

**Read this before sizing**

- About 75 ideas have been tried across both rounds to find twelve. Survivors of a search look better than they are.
- 2017–2019 made $780 a year. A repeat of those years is the normal bad case, not a tail.
- DWR's daily-loss halt: a tight one turned the result negative in the mock. Leave it off for DWR, or set it wider than one full stop.
- These three cannot share a Trade Account with each other or with the suite as written: each flattens positions it did not open. A combined study (like the suite) is the next build if you want them on one account.

### Inputs (all three)

Strategy inputs come first and differ per file (RSI length and thresholds; value-area share, bin, marks and times; narrow-range count, opening-range minutes, last entry; stop × range). Then, identical in all three: day session open / close, flat time, directions allowed, sessions in the average range, max trades per day, tally cost, verbose log, orders on/off, allow live, size mode (fixed or risk % of the account input), fixed contracts, account size, risk %, max contracts, max $ risk per trade, daily loss halt, daily profit lock, max losses in a row, safety checks, untracked-position handling, max entry delay, CME early closes.

Defaults: 1 contract fixed; account input $10,000 (used only by risk-% sizing); every halt off.

### Install and build

1. Copy the three `.cpp` files to `C:\SierraChart\ACS_Source`.
2. Analysis >> Build Custom Studies DLL >> select **one** file >> Remote Build.
3. Chart: MNQ, 5-minute bars, New York time zone, at least 30 days loaded, **overnight session included** (DWR needs the 09:25 bar; VAR needs volume). For long tests use the continuous contract, back-adjusted.
4. Add the study. Keep the Trade Window's Attached Orders list empty. One Trade Account per script.
5. Read the banner, `CHART CHECK` and `TALLY` lines in the Message Log.

### Replay test

1. Global Trade Simulation Mode ON; Trade >> Clear Trade Simulation Orders & Data.
2. Set commission.
3. Chart >> Replay Chart >> Accurate Trading System Back Test Mode, start after the first 30 loaded days. **Run 2024-03 to today: that is the out-of-sample test this research could not do.**
4. Check: one `engine SIGNAL #n ... TRADED` or `NOT traded -- reason` per `SIGNAL`; `FILL VERIFY ok`; no open trades at the end; every exit before 15:56; the TALLY close to the Replay result.

### Rithmic paper account

Real-time data required. `Allow LIVE orders` = Yes only with Trade Simulation Mode off and the paper account selected. 1 contract. Compare DWR's fill with the 09:30 open print for a few weeks: that slippage is the number this research assumed (one tick).

### Not verified

- Anything in Sierra Chart: compile, attached-stop placement, Replay results.
- Any API call against current Sierra documentation (site unreachable from this session).
- Any result after 2024-03-07.
- MNQ specifications were confirmed from CME's contract page via search ($2 × index, tick 0.25 = $0.50); Globex hours were not re-confirmed.

### Files

- `research/gen_rev/` — templates, the three decision functions, header texts, `gen.py`.
- `research/batch4/` — data build (`mk.py`, `adj.py`), exploration (`e1.py` … `e14.py`), final reference model (`ref2.py`), model-vs-mock comparison (`cmp.py`), exit tests (`exits.py`), report (`report4.py`).

---

## Fourth script (2026-11-11): `EndOfDayFlow_NQ_MNQ.cpp` (EDF)

**Rule.** At 15:00 New York, if NQ is at least 1.0% away from the prior day-session close, trade in the direction of that move at market; attached stop 0.40 × average day range; no target; flat at 15:55. One trade a day, about one day in three.

**Why it should work.** Leveraged and inverse ETFs must rebalance at every close by an amount proportional to the day's return, in its direction, and short-gamma option dealers hedge the same way. Both flows are mechanical and concentrate near the close. Sources: Baltussen, Da, Lammers & Martens (2021, *Journal of Financial Economics*); Barbon, Beckmeyer, Buraschi & Moerke, *Liquidity provision to leveraged ETFs and equity options rebalancing flows* (SSRN 3925725). Barbon et al. find the leveraged-ETF channel the larger one, and later work finds that channel has weakened over time. The Nasdaq-100 leveraged-ETF complex (TQQQ / SQQQ) grew roughly tenfold from 2017 to 2022, which fits the effect being strongest on NQ after 2021. That last point is my hypothesis, not a tested result.

**How it differs from MomentumSuite.** It decides once, at 15:00, from a fixed percent move against yesterday's close, and holds 55 minutes; the suite decides from intraday noise bands, mostly in the morning. It does trade with the day's move, so it is on the suite's side on many days. Daily correlation with the suite is 0.26.

### Research round 5 (about 10 new ideas; about 85 in total)

| Idea | 2017–21 PF | 2022–24 PF | Verdict |
|---|---|---|---|
| **End-of-day flow, percent move from prior close** | 1.54 | 2.31 | **Built (EDF).** 58 of 64 settings positive in both periods |
| Same, move measured in average ranges | 0.94–1.13 | 1.85–2.02 | Flat before 2022: the percent measure is what holds |
| Same on ES / YM | 0.71–0.87 / ~1.0 | 1.27–1.52 / ~1.0 | ES only after 2021; YM nothing |
| Month-end rebalancing (fade month-to-date return) | 0.3–1.5 | 0.5–2.4 | Rejected: inconsistent, 60–120 trades |
| Pre-holiday long | 1.1–1.3 | 1.3–1.4 | Rejected: 65 trades in 7 years |
| Pre-FOMC drift (Lucca & Moench), long to 13:55 | 0.45–0.56 | 1.2–2.9 | Rejected |
| Yesterday / overnight sign combinations (Yu, Rentzler & Wolf) | — | — | Only "down yesterday → long", which is the washout script |
| Cross-day NQ vs ES / YM relative-value reversal | 1.1–1.35 | 0.7–1.1 | Rejected |
| Same-time-of-day persistence (Heston, Korajczyk & Sadka) | 0.80–0.85 | 0.88–0.95 | Rejected: costs |
| Narrow initial balance, close break | 0.7–1.1 | 1.1–1.4 | Rejected |
| VWAP fade only in high-volatility regimes | 0.8–1.2 | 0.6–0.85 | Rejected: intraday fading loses again |
| Sell after RSI(2) > 95 (washout mirror) | 1.29–1.33 | 1.26 (36 trades) | Not a new script; the washout file's 'RSI sell' input can switch it on |
| OOPS (open beyond yesterday's extreme, back through it) | 0.86–1.20 | 1.15–1.55 | Overlaps the value-area script |

### Measured (the `.cpp` in the mock, 2017-01 → 2024-03, 1 MNQ, $2.50 per round trip)

| Metric | EDF | Four new scripts | MomentumSuite + four new |
|---|---|---|---|
| Total net profit | $12,612 | $41,418 | — |
| Average / median month | $145 / $2 | $476 / $207 | $1,702 / $933 |
| Best / worst month | $1,921 / −$476 | $4,068 / −$1,100 | — / −$1,859 |
| Profitable months (of 87) | 47 (5 without a trade) | 57 | 62 |
| Trades | 560 | 1,887 | — |
| Win rate | 57.7% | 48.2% | — |
| Profit factor | 1.88 | 1.44 | — |
| Expectancy per trade | $22.52 | $21.95 | — |
| Maximum drawdown | −$862 | −$3,703 | −$5,060 |
| Longest losing streak | 7 | 11 | — |
| Average win / loss | $84 / −$61 | $148 / −$96 | — |
| Trades per day | 0.30 | 1.01 | — |
| Costs included | $1,400 | $4,718 | — |
| 2017–21 / 2022–24 per month | $73 / $313 | $321 / $846 | $1,128 / $2,978 |
| At $5.00 per round trip | $129 | $422 | — |
| Daily Sharpe | 1.81 | 1.75 | 2.19 |

EDF by year: 2017 +$90 · 2018 +$1,234 · 2019 −$435 · 2020 +$3,355 · 2021 +$153 · 2022 +$6,262 · 2023 +$2,290 · 2024 (to March) −$336.

Four new scripts by year: 2017 +$515 · 2018 +$2,282 · 2019 +$433 · 2020 +$10,876 · 2021 +$5,127 · 2022 +$14,331 · 2023 +$7,310 · 2024 (to March) +$545. Monte Carlo, one year: 3.7% of years lose; drawdown median −$1,604, 99th percentile −$4,769.

**Verification.** 562 of 562 trades identical to the Python model (same days, sides, entry prices, P&L). Identical under recalculations at two intervals, once-a-minute calls and doubled calls; sanitizers clean; 0 warnings; 0 invariant violations. Faults: wrong time zone, failed flattens, foreign position, signals-only, risk-% sizing, $ cap, daily-loss halt, all behaving as in the other three.

**Read this before sizing EDF**

- Half its profit is from 2022, and the quiet years made nothing.
- In 2017–21 the result depends on the exit minute: PF 1.54 at 15:55, 1.25 at 15:50 or 15:59.
- Measured in average ranges instead of percent, the same idea was flat before 2022.
- It is the strongest candidate for your 2024–today export to confirm or kill. If that period looks like 2022–24, it is the best single script here; if it looks like 2017–21, it is small.

### Where the $1,000 target stands

- Four new scripts at 1 MNQ each: **$476 a month** measured (index levels averaging about 10,500), drawdown −$3,703.
- With MomentumSuite at 1 MNQ per leg: **$1,702 a month**, drawdown −$5,060, 62 of 87 months positive.
- If the same edge in basis points held at NQ 25,000, the four new scripts alone would be about $1,030 a month, with drawdowns about 2.4 times the historical ones. That is an extrapolation, not a result.
- None of it is tested after March 2024.

---

## Combined study for one account (2026-11-11): `DiversifiedSuite_NQ_MNQ.cpp`

The four scripts above (DWR, VAR, CEB, EDF) in one study, on one chart and one Trade Account. Built like `MomentumSuite_NQ_MNQ.cpp`, and it can run next to MomentumSuite only if each has **its own Trade Account**.

**How it shares the account**

- Each strategy owns one leg: its own market entry with its own attached stop, followed by its order IDs.
- The account position must equal the sum of the open legs; a difference lasting 5 seconds flattens everything.
- A leg may not open against another open leg. That refused 95 of 1,902 signals in 2017–2024 (VAR 42, EDF 30, CEB 23; nearly all against a DWR long). They made +$1,018 in seven years, so one account costs about $12 a month.
- Legs end on their own stop or at 15:55. No stop is ever modified.
- Size per leg: fixed contracts, or risk % of the account input per leg (capped per leg). Two optional caps: $ risk per leg, and $ risk of all open legs together.

**Measured** (the file in the mock, 2017-01 → 2024-03, 1 MNQ per leg, $2.50 per contract round trip)

| Metric | Combined study | Four files on four accounts |
|---|---|---|
| Testing period | 2017-01-01 → 2024-03-07 | same |
| Contracts | 1 per leg, at most 3 open | 1 per script |
| Total net profit | $40,400 | $41,418 |
| Average / median month | $464 / $176 | $476 / $207 |
| Best / worst month | $3,810 / −$1,054 | $4,068 / −$1,100 |
| Profitable / losing months (of 87) | 55 / 32 | 57 / 30 |
| Legs | 1,792 | 1,887 |
| Win rate | 48.5% | 48.2% |
| Profit factor | 1.45 | 1.44 |
| Expectancy per leg | $22.54 | $21.95 |
| Maximum drawdown | −$3,859 | −$3,703 |
| Longest losing streak | 10 | 11 |
| Average win / loss | $149 / −$96 | $148 / −$96 |
| Legs per day | 0.96 | 1.01 |
| Costs included | $4,480 | $4,718 |
| 2017–21 / 2022–24 per month | $310 / $830 | $321 / $846 |

By year: 2017 +$508 · 2018 +$2,142 · 2019 +$117 · 2020 +$10,540 · 2021 +$5,319 · 2022 +$13,206 · 2023 +$7,780 · 2024 (to March) +$790. At $5.00 per round trip: $413 a month.

By leg ($ a month, PF): DWR 181 (1.38) · VAR 63 (1.32) · CEB 88 (1.45) · EDF 133 (1.87). DWR is never refused (it enters first, at the open); the others lose a few legs each.

Monte Carlo, one year: 4.2% of years lose; drawdown median −$1,651, 99th percentile −$5,006. Account size at 1 MNQ per leg (historical index level): a 25% drawdown happens in 21% of years with $10,000, 0.2% with $25,000.

With MomentumSuite on a second account: $1,690 a month, drawdown −$5,216, worst month −$1,859, 62 of 87 months positive; daily correlation between the two studies 0.35.

**Tests**

| Check | Result |
|---|---|
| Each leg alone vs its single-strategy file | identical fills, all four |
| All four together vs the four files minus refused legs | $44,719.50 gross both (one day differs by $0.01: the mock's averaged-price rounding) |
| Recalculations at two intervals, once-a-minute calls, doubled calls | identical |
| AddressSanitizer + UBSan; `-Wall -Wextra` warnings | clean; 0 |
| Invariant violations (3.1 M calls) | 0 |
| Faults: wrong time zone, 12 failed flattens, foreign contract, signals only | entries blocked / re-issued with MANUAL ACTION line / flattened after 5 s / no orders |
| Controls: per-leg sizes 2/1/3/2, risk % sizing, $500 open-risk cap, $400 daily loss, losing-streak halt | position never above the cap; each control acted as set |

**Not verified:** anything in Sierra (build, attached stops, two stops working at once, Replay results) and anything after 2024-03-07. The study is built from `research/suite_dsu/` (`assemble.sh`).
