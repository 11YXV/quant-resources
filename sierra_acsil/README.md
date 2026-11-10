# MNQ day-trading suite — research, selection, three ACSIL scripts

Built 2026-11-10. Three Sierra Chart ACSIL studies for MNQ, day session only, flat by 15:55 New York.

| Script | Idea | Entry | Exit |
|---|---|---|---|
| `NoiseBandMomentum_NQ_MNQ.cpp` (NBM) | Intraday momentum | 30-minute close outside the "noise band" and beyond VWAP | 30-minute close back through max(band, VWAP); disaster stop; 15:55 |
| `GapOpeningDrive_NQ_MNQ.cpp` (GOD) | Opening drive | Stop entry beyond the first 5-minute range, on the side of the overnight gap only | Stop 0.10 × average day range; 15:55 |
| `TrendDayVwapPullback_NQ_MNQ.cpp` (TVP) | Trend-day pullback | After the day has broken a noise band: first 15-minute block that touches VWAP and closes back on the trend side | Stop 0.15 × average day range; 15:55 |

**Status: not yet built or run in your Sierra Chart.** They compile and link with MinGW against the public SDK headers and were run in a mock of Sierra with a simulated broker. The numbers below are from that mock, on data that ends in March 2024. Sierra's Replay is the authority.

---

## Part A — Research findings

**Data.** No Sierra export was available in this session, so I used a public set of NQ 1-minute bars by contract (`yakki-minati/US.Index.Futures.Data`, 2016-08 → 2024-03, UTC). I rolled 8 days before expiry and converted to New York time. The volume profile peaks at 09:30 and 15:59 and the short sessions match the CME calendar in lessons #66, so the time alignment is right. 1,813 full day sessions; tests use 2017-01 → 2024-03-07 (87 months). NQ and MNQ trade at the same prices; all P&L is at MNQ's $2 per point.

**Costs.** $2.50 per contract per round trip ($1.50 commission and fees + one tick of slippage per side). Stress tests use $5.00 and $7.50.

**Split.** Choices were made on 2017–2021. 2022–2024 was the check. Your own Sierra export (2024 → today) has not been seen at all and is the real out-of-sample test.

**From the books in this repository**

- *Kaufman, Trading Systems and Methods (5th ed.), ch. 16 "Day Trading"*: short-term movement is mostly noise, so "for directional entries, waiting for a pullback will improve entries"; opening-range breakouts with a volatility-based distance (Crabel); a break of the range after midday as a directional change; the "TSM Midday Support and Resistance" rules. Ch. 15: time-of-day patterns and opening gaps.
- *Carver, Systematic Trading*: size positions from volatility and a risk target, not from conviction; prefer simple rules that survive across instruments.
- *Bailey, Borwein, López de Prado, Zhu, "The Probability of Backtest Overfitting"* and *López de Prado, Advances in Financial Machine Learning*: the more variants tried, the less a best result means. This is why each choice below is a plateau value, not the best cell, and why the count of things tried is stated.
- *Aronson & Masters, Statistically Sound Machine Learning for Algorithmic Trading*: out-of-sample confirmation and data-mining bias.
- *Chan, Quantitative Trading / Algorithmic Trading*: transaction costs decide whether a short-horizon edge exists at all.

**External research (published claims, re-tested here)**

- Zarattini, Aziz, Barbon (2024), *Beat the Market: An Effective Intraday Momentum Strategy for S&P500 ETF (SPY)* — the noise band and the VWAP trailing exit.
- Gao, Han, Li, Zhou (2018), *Market intraday momentum*, Journal of Financial Economics — the first half-hour return predicts the last half-hour.
- Zarattini & Aziz (2023), *Can Day Trading Really Be Profitable?* — 5-minute opening range, stop at 10% of the 14-day ATR, hold to the close.

**What I measured on NQ**

1. **Momentum works; mean reversion does not.** Every fade I tested lost after costs in both periods: gap fade, VWAP ±1.5–2.5σ fade, failed breakout of the prior-day or overnight high/low, Kaufman's midday support/resistance rule. The same fades also lost on ES.
2. **Gao et al.'s last-half-hour rule did not hold on NQ** (flat to negative, 2017–2024).
3. **The noise-band rule held** in both periods, across band widths 0.8–1.5, and on ES as well as NQ.
4. **A plain opening-range breakout is weak (PF 1.06–1.2) and a profit target makes it worse.** Split by the overnight gap, breaks *with* the gap made PF 1.38 and breaks *against* it PF 1.00.
5. **Overnight is not a good edge.** Holding long from the close to the next open made $124 a month per MNQ in 2017–2024 but lost in 2022–2024, with a $7,788 drawdown and a worst night of −$1,300. The noise-band rule applied to the overnight session lost (PF 0.80). Add full overnight margin (about $2,500–3,000 per MNQ instead of day margin) and it is not worth it.
6. **MES is not better.** The same rules on ES were profitable but weaker (PF 1.23 against 1.41), so the suite stays on MNQ.

---

## Part B — Three architectures

| | A: Trend continuation | B: Mean reversion | C: Opening range / volatility expansion |
|---|---|---|---|
| Edge proposed | Moves larger than the usual noise for that time of day keep going (slow information flow, hedging, stop cascades) | Stretched prices snap back to VWAP / prior levels | The first break after the open sets the day's direction |
| Variants tested | Noise band (NBM), trend-day pullback (TVP), afternoon range break, prior-day / overnight level breaks | Gap fade, VWAP band fade, failed breakouts, midday S/R | ORB 5/15/30/60 min, with and without targets, gap-aligned (GOD) |
| Works when | Trending, volatile years | (did not work) | Days with an overnight gap that the open confirms |
| Fails when | Quiet, range-bound years (2017, 2019) | Trend days, which are the big ones | Open reverses the gap; whipsaw opens |
| Trades | NBM 0.8/day, TVP 0.4/day | 0.5–1.4/day | 0.4/day |

---

## Part C — Quantitative comparison

1 MNQ, $2.50 per round trip, research models on 1-minute bars. IS = 2017–2021, OOS = 2022–2024-03.

| Candidate | Trades | PF IS | PF OOS | $/month | Max DD | Verdict |
|---|---|---|---|---|---|---|
| **A1 Noise band (NBM)** | 1,532 | 1.43 | 1.41 | 239 | −1,388 | **Selected** |
| **A2 Trend-day VWAP pullback (TVP)** | 680 | 1.50 | 1.62 | 150 | −1,599 | **Selected** |
| A3 Afternoon range break | 1,091 | ~1.1 | 1.27 | 49 | −1,990 | Weak in-sample |
| A4 Prior-day high/low break | 851 | 0.73 | 1.00 | −42 | −4,787 | Rejected |
| B1 Gap fade | 938 | 0.99 | 0.82 | −42 | −4,769 | Rejected |
| B2 VWAP 2σ fade | 1,684 | 0.78 | 0.86 | −99 | −10,578 | Rejected |
| B3 Failed breakout (prior day) | 778 | 0.93 | 0.86 | −33 | −3,511 | Rejected |
| B4 Kaufman midday S/R | 3,102 | 0.87 | 1.10 | −24 | −9,829 | Rejected |
| C1 ORB 5-min, both sides | 1,791 | ~1.17 | 1.20 | 107 | −2,516 | Superseded by C2 |
| **C2 ORB 5-min, gap side only (GOD)** | 788 | 1.34 | 1.37 | 88 | −1,248 | **Selected** |
| Overnight long hold | — | + | − | 124 | −7,788 | Rejected |

About 20 hypotheses and roughly 250 parameter cells were tried in total. With that many trials, treat every profit factor above as optimistic.

**Robustness of the three selected**

- NBM: positive in both periods for band 0.8, 1.0, 1.2, 1.5; with and without VWAP; disaster stop none / 0.3 / 0.5 / 0.75; 1, 2, 3 or 9 trades a day.
- GOD: positive in both periods for opening ranges of 3, 5, 10, 15, 30 minutes; stops 0.05–0.25; minimum gap 0–0.4; long and short separately.
- TVP: 60 settings; 100% profitable in 2017–2021, 92% in 2022–2024. The 15-minute confirmation block beats 5 minutes clearly. That choice is the one most exposed to over-fitting.

---

## Part D — Selection

The momentum family is the only one with an edge that survived both periods, costs, and parameter changes. Within it, the three scripts enter at different times for different reasons:

- GOD trades the first hour, on overnight information.
- NBM trades from 10:00, on the size of the move.
- TVP trades the pullback that NBM's exit rule often gives up.

Daily P&L correlations are 0.35–0.53, so they diversify partly, not fully. All three are long volatility: they make money in trending years and nothing in quiet ones. A mean-reversion script would have diversified better, but none had an edge, so I did not build one to fill a slot.

---

## Part E — Implementation

Each `.cpp` is self-contained and holds all three steps:

- **Step 1, engine**: set `Orders` = *Signals only*. The study draws the band / VWAP / range lines, logs every signal, and logs a TALLY of its own virtual trades when the chart loads.
- **Step 2, orders**: `Orders` = *Send orders*. One entry order with an attached stop; fill detection; exits; daily limits.
- **Step 3, hardening**: chart and feed checks, recalculation handling, orphan and inherited positions, stop-coverage check, flatten retries, CME short sessions.

The order lifecycle, inputs and rules are documented at the top of each file. The order layer follows `Sierra_ACSIL_Lessons_Learned.md` and reuses the verified calls from `PatelIchimoku_NQ_3MNQ` v3.

**How it was tested**

1. Compiled with MinGW-w64 (`-std=c++17 -Wall -Wextra`): 0 errors, 0 warnings, DLL links and exports the study function. The public headers are SDK 2414; two members your SDK 2955 has (`ConvertDateTimeUTCToChartTimeZone`, `GetChartTimeZoneStringRef`) were added to a local copy.
2. Run unchanged in a mock of the ACSIL surface with a simulated broker (`research/mock`): every 1-minute bar as 4 ticks, 5-minute chart bars, 3.0 million study calls per script, under AddressSanitizer and UBSan.
3. Trade-for-trade comparison with an independent Python model (`research/ref.py`):

| | Model trades | Mock trades | Matched, same entry price | Gross P&L model / mock |
|---|---|---|---|---|
| NBM | 1,531 | 1,539 | 1,499 | $25,172 / $24,650 |
| GOD | 786 | 790 | 781 | $12,563 / $12,757 |
| TVP | 679 | 682 | 669 | $15,005 / $15,203 |

   Causes of the differences: days with missing minutes (the model fills gaps forward, the mock replays only real bars), the first sessions after the mock's 30-session warm-up, the order of high and low inside a 1-minute bar, and 14 GOD signals the order layer refused (price already through the cancel level or the trigger at the first tick, or a data gap that made the call more than 120 s late).
4. Identical trades with 361 full recalculations injected mid-run, with the study called only once a minute, and with every call doubled.
5. Invariants on every tick (position within the cap, flat outside 09:30–15:56, nothing working at the end): 0 violations.
6. Faults: wrong chart time zone (entries blocked), 12 failed flattens in a row (re-issued every 30 s), a position the study did not open (flattened), $ risk cap, risk-% sizing, daily loss halt, losing-streak halt, profit lock.

The sparse-call run found a real bug: GOD's "other side broke first" cancel read only the last price, so a touch between two study calls was missed (92 extra trades). It now reads the bars' highs and lows since the order went in.

---

## Part F — Performance report

Source: the three `.cpp` files run in the mock, 2017-01-01 → 2024-03-07, starting balance not used (fixed 1 MNQ per script), $2.50 per round trip.

| Metric | NBM | GOD | TVP | Portfolio (1 MNQ each) |
|---|---|---|---|---|
| Testing period | 2017-01 → 2024-03 | same | same | same |
| Contracts | 1 | 1 | 1 | up to 3 at once |
| Total net profit | $20,828 | $10,740 | $13,427 | $44,995 |
| Average monthly net | $239 | $123 | $154 | $517 |
| Median monthly net | $147 | $12 | $100 | $252 |
| Best month | $1,226 | $1,370 | $1,939 | $4,487 |
| Worst month | −$451 | −$514 | −$656 | −$843 |
| Profitable months | 55 | 46 | 49 | 59 |
| Losing months | 32 | 41 | 38 | 28 |
| Total trades | 1,528 | 785 | 677 | 2,990 |
| Win rate | 39.7% | 18.1% | 28.8% | 31.6% |
| Profit factor | 1.41 | 1.49 | 1.56 | 1.46 |
| Expectancy per trade | $13.63 | $13.68 | $19.83 | $15.05 |
| Maximum drawdown | −$1,189 | −$975 | −$1,602 | −$2,172 |
| Longest losing streak | 16 | 22 | 23 | 31 |
| Average winning trade | $118 | $231 | $193 | $151 |
| Average losing trade | −$55 | −$34 | −$50 | −$48 |
| Average trades per day | 0.82 | 0.42 | 0.36 | 1.60 |
| Costs included | $3,820 | $1,962 | $1,692 | $7,475 |
| Three best trades, share of net | 11% | 22% | 16% | 6% |

**By year, portfolio:** 2017 +$94 · 2018 +$6,366 · 2019 −$292 · 2020 +$5,406 · 2021 +$10,984 · 2022 +$13,666 · 2023 +$6,287 · 2024 (to 7 March) +$2,484.

**In-sample / out-of-sample:** $376 a month in 2017–2021, $831 a month in 2022–2024.

**Stress:** $431 a month at $5.00 per round trip, $345 at $7.50. Starting 10, 21 or 63 sessions later: $518, $520, $534. Worst rolling 12 months: −$1,564. Longest run of losing months: 5.

**Monte Carlo** (one year, 5-day blocks resampled 10,000 times): 2% of years lose; median +$5,801; 5th percentile +$1,074; drawdown median −$1,493, 95th percentile −$2,825, 99th −$3,585.

**GOD caveat.** When GOD's entry and stop are inside the same 1-minute bar, the data cannot say which came first. The mock's tick path gives $10,740. Assuming the stop is hit every time gives $7,825 (PF 1.35), which puts the portfolio near $484 a month. On 5-minute bars the same worst-case rule leaves almost nothing (PF 1.06), which is what GOD's on-chart TALLY will show. Only a tick Replay settles it.

### Is $1,000 a month supported?

Not yet, and not at one contract on this evidence.

- Measured, at the index levels of 2017–2024 (average about 10,500): **$517 a month for 1 MNQ per script**, with two flat years out of seven.
- The edge is roughly proportional to the index level. If the same edge in basis points held at NQ 25,000, 1 MNQ per script would be about $1,070 a month on the all-years rate, or about $0 in a year like 2017 or 2019. That is an extrapolation, not a result. Drawdowns scale the same way: roughly −$5,000 where history shows −$2,172, and a 99th-percentile year near −$8,500.
- 2 MNQ per script doubles both.

**Account size.** For 1 MNQ per script at today's index level, plan for a drawdown of $5,000–8,500. An account of about $20,000 keeps that under half the account. $10,000 does not.

**What is not established:** anything after March 2024; fills in real Sierra Replay; live slippage at the 09:35 open (GOD) and on 30-minute marks (NBM, TVP), where other momentum traders act at the same moment.

### What to do next

1. Build each file in Sierra (one at a time) and read the banner, CHART CHECK and TALLY lines.
2. Replay each script from 2024-03 to today in Accurate Trading System Back Test mode, each on its own Sim account, with commission set. This is the out-of-sample test.
3. Export 1-minute MNQ bars 2024-01 → today so the research can be re-run on your data.
4. Only if step 2 holds: Rithmic paper trading on a real-time feed (your current feed is delayed by 10 minutes, lessons #65; the scripts block live entries on it).

### Installation

1. Copy the three `.cpp` files to `C:\SierraChart\ACS_Source`.
2. Analysis >> Build Custom Studies DLL >> select **one** file >> Remote Build. Expect "The compiler response is empty".
3. Chart: MNQ, 5-minute bars, time zone New York, at least 25 days loaded. For long tests use the continuous contract (volume rollover, back-adjusted).
4. Add the study. Keep the Trade Window's Attached Orders list empty. Use a different Trade Account for each script (lessons #72) — three scripts on one account will flatten each other's positions.
5. Global Trade Simulation Mode ON, chart Auto Trading enabled.

### Replay test

1. Trade >> Clear Trade Simulation Orders & Data; confirm 0 fills.
2. Set commission ($0.75 per side or your broker's rate).
3. Chart >> Replay Chart >> Accurate Trading System Back Test Mode, start date after the first 25 loaded days.
4. Check in the Message Log: one `engine SIGNAL #n ... TRADED` or `NOT traded -- reason` per `SIGNAL`; `FILL VERIFY ok`; `Number of Open Trades` = 0; every exit before 15:56.

### Rithmic paper account

Real-time data is required. Set `Allow LIVE orders` = Yes only when Trade Simulation Mode is off and you intend orders to reach the paper account. Start with 1 contract and a daily loss halt.

### Known limits

- One strategy per Trade Account. On a single live account the three scripts cannot run together as written.
- Decisions are made at bar closes; the market order goes in on the first study call after the close, so it depends on the chart update interval.
- The day's trade and loss counters are not kept across a Sierra restart.
- The scripts skip CME holiday sessions from a built-in calendar. An unscheduled early close is not known to them; the attached stop still protects the position.

### Files

- `research/` — data build (`build.py`, `cont.py`, `days.py`), candidate tests (`s1.py` … `s10.py`, `s4.py` overnight, `seg.py`), reference models (`ref.py`), portfolio and report (`port.py`, `report.py`), model-vs-mock diff (`diff.py`).
- `research/gen/` — the template the three files are generated from (`gen.py`). Edit the template, not three copies.
- `research/mock/` — mock `sierrachart.h`, simulated broker and harness.

---

## Follow-up (2026-11-10): strategies claimed on forums and blogs

Reddit blocks the search crawler, nothing from 4chan /biz/ was indexed, and this session could not open forum pages, so the claims below come from search summaries, TradingView listings, blogs and two public GitHub research logs. No source had a verified live record. Each claim was tested with `research/forum_claims.py` on the same data, costs and split as above (1 MNQ, $2.50 per round trip, IS 2017–2021, OOS 2022–2024-03).

| Claim | Version tested | PF IS | PF OOS | $/month IS / OOS | Verdict |
|---|---|---|---|---|---|
| Gap-down rebound, buy the open, exit after 60 min (Quantified Strategies; exact conditions paywalled) | Gap ≥ 0.25 × avg range and prior day down | 1.21 | 0.75 | 17 / −57 | Fails out-of-sample |
| | Gap ≥ 0.1 and prior close in the low quarter of its range | 1.27 | 1.26 | 15 / 42 | Positive but 138 trades in 7 years; too small to build |
| 15-min opening range, 5-min close confirmation, midpoint stop, 2R target (TradingView, Trade That Swing) | As described | 0.99 | 1.01 | −7 / 12 | Break-even |
| | Same, no target | 1.04 | 1.18 | 30 / 270 | Weak in-sample; the gap-side filter in GOD is better |
| Long only in the first two hours (EliteTrader) | Every day; above 50-day average; prior day up; gap up | 0.93–1.02 | 0.99–1.02 | −46 to 18 | No edge |
| ATR trend band / Supertrend on 10-min bars (TradingView vendor) | ATR 10, × 3 | 1.12 | 1.08 | 139 / 192 | Weak; drawdown −$3,200; × 2 and × 4 are flat |
| VWAP reversion in 09:45–11:30 and 13:00–15:00 (prop-firm blog, Reddit post) | 1.5–3 σ, stops 1–2 σ | 0.70–0.95 | 0.79–0.97 | all negative | Loses |
| ICT Silver Bullet | Not re-tested | — | — | — | Two independent backtests found it lost after costs in every test 2020–2025 |

**Result: none of the forum strategies passed, so no new script was built.** The only forum-popular idea that held up is the noise-band momentum already in this suite.
