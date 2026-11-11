# Lessons Learned — addendum from the MNQ momentum suite (2026-11-10)

From building `NoiseBandMomentum_NQ_MNQ`, `GapOpeningDrive_NQ_MNQ` and `TrendDayVwapPullback_NQ_MNQ`. Numbered from #76 (the Patel script already cites #73–#75). "Mock" marks anything seen only in the mock, not in Sierra.

## 1. API facts

- **The public 2414 headers lack `sc.ConvertDateTimeUTCToChartTimeZone` and `sc.GetChartTimeZoneStringRef`.** Both exist in your SDK 2955 (PersonP3T addendum §1). A local compile against 2414 fails on them. For a local check, add the two function pointers to a *copy* of `sierrachart.h`; never change the script to suit the old headers. Same family as #59.
- **Everything else these scripts call compiles against 2414** with MinGW `-Wall -Wextra`, 0 warnings: `s_SCNewOrder::Stop1Offset`, `TextTag`, `TimeInForce`, `AttachedOrderStop1Type`, `sc.Volume[]`, `DRAWSTYLE_POINT`, `SCDateTime(int, int)`.

## 2. Gotchas

76. **A cancel rule for a resting order must read bar highs and lows, not only the last price.** `GapOpeningDrive` cancels its stop entry when the other side of the opening range trades first. The first version tested `lastPx`. With the study called once a minute instead of on every tick, a touch between two calls was missed and 92 extra trades were taken over 7 years (mock). Sierra does not call a study on every trade either. **Fix: check `sc.Low` / `sc.High` of every bar since the order went in (the live bar and the one before it), keyed by the bar's time so a recalculation cannot break it.** Applies to any "void if X trades first" rule.

77. **A stop-first tally on 5-minute bars can hide a real edge when the stop is tight.** GOD's stop is 0.10 × the average day range, often inside the entry bar's own range. The on-chart tally (stop first whenever the bar contains it) shows PF 1.06. The same rules on 1-minute bars with the same worst-case rule show PF 1.35, and a tick path 1.49. This is #54 in reverse: there the bar-level guess flattered the result, here it buries it. **Rule: when the stop distance is smaller than a typical bar, the bar-level number is a floor, not an estimate — say so in the log and go to tick Replay.**

78. **Run the study in the mock in three call patterns, not one.** Every tick, once a minute, and every call doubled. NBM and TVP were identical in all three; GOD was not (#76). A single dense run would have passed all three scripts.

79. **One shared template beats three hand-synchronised files.** The three scripts share one engine driver and one order layer and differ in a 30-line decision function, the inputs and the header. They are generated from `research/gen` (`gen.py`). A fix to the order layer is made once. Each generated `.cpp` is still fully self-contained for the remote build.

80. **Several strategies on one account: follow each leg by its own order IDs, not by the account position.** `MomentumSuite_NQ_MNQ` gives each strategy one entry order with its own attached stop and reads the parent's and the stop's status (`SCT_OSC_FILLED`, working) to know where the leg is. The account position is used for one thing: it must equal the sum of the open legs, or everything is flattened after 5 s. This replaces the "one strategy per account" limit of #72 for strategies inside one study. It needs `AllowEntryWithWorkingOrders = 1`, `AllowMultipleEntriesInSameDirection = 1`, `CancelAllOrdersOnEntries... = 0` and `MaximumPositionAllowed` = the legs added up (#2, #8). Mock only so far.

81. **In a multi-leg study `FlattenAndCancelAllOrders()` and `CancelAllOrders()` hit every leg.** Use `sc.CancelOrder(parentID)` for one leg's resting entry, and for one leg's discretionary exit move that leg's own stop to just behind the market (verify-then-modify) and let it fill. Flatten only when the leg is alone or when everything must go.

82. **Process new entries before discretionary exits in the same call.** With exits first, NBM's "only leg, so flatten" fired on the same 30-minute mark on which TVP wanted to enter, and 39 TVP trades were refused over 7 years ("a flatten is in flight"). With entries first, the new leg is already working, so the exit takes the stop-tightening path and both happen.

83. **An exit that trails "N ticks behind the market on every call" makes the result depend on how often the study is called.** Same fills, 1.5% less net when the mock called the study once a minute instead of on every tick. Expect a Replay result to move with the chart update interval; it is not a bug.

84. **Standing requirement (2026-11-10): every new script must be testable tick by tick in Replay on ONE Trade Account.** Build it as a leg of the suite or with the same own-order tracking.

85. **A decision that reads another chart must wait until that chart's bar is final.** When bar k of the trading chart closes, the reference chart's bar k may not have its last trade yet, or may not exist yet. `MomentumSuite` v2 marks the decision as pending, and on every study call checks whether the reference chart has a bar for the same time AND a later one; only then does it read the close and decide. If the trading chart closes bar k+1 first, the decision is dropped and counted. Never read `ref[SC_LAST][last]` at the moment your own bar closes. (Mock only; first Replay check.)

86. **Feed a reference market's session statistics from its final bars only, with a cursor.** One pass, in order, never the forming bar; the same closed-bar discipline as #26/#27 applied to the second chart.

87. **Inputs appended in a later version must make "reads 0" mean "feature off / default value".** v2's three new legs read 0 on a v1 study already on the chart: the on/off inputs read No, contract counts and distances fall back to their defaults, and the reference chart reads 0 = none. The old study keeps behaving as v1 until re-added (#15, #64).

88. **When several legs must leave on the same call, decide first, act second.** Pass 1 marks every leg the engine wants out; pass 2 flattens if nothing else is staying, otherwise tightens each leaving leg's stop. With one pass, the first leg saw the second as "staying" and both took the slower path.

89. **"Signals on the chart after a recalculation are history" must not swallow the signal of the bar that just closed.** The order layer synced its "last signal handled" to the engine's latest signal on the first live call after any recalculation (#63). When a recalculation landed on the exact tick after a signal bar closed, that fresh signal was marked handled and never traded (found by injecting recalculations at a second interval: one EDB trade in seven years). **Fix: key "handled" by the signal bar's time, which survives recalculations, and treat as history only a signal that was already handled or whose bar is older than the bar that just closed.** The lateness and delay checks still stop anything stale. Applied to the suite and the three single-strategy files.

90. **Inject recalculations at more than one interval.** One interval (every 7,919 ticks) passed for v1 and v2 and only hit the #89 case once nine legs gave it more signal bars to land on. A second interval costs one more run.

## 3. Research record (NQ 1-minute, 2017-01 → 2024-03, $2.50 per round trip)

- **Intraday mean reversion lost in every form tried**: gap fade, VWAP 1.5–2.5σ fade, failed breakout of the prior-day and overnight extremes, Kaufman's midday support/resistance. Also on ES. Do not spend another script on a fade without new evidence.
- **Gao et al.'s "first half-hour predicts the last half-hour" did not hold on NQ.**
- **A profit target hurt every momentum variant.** The edge is in the few days that run to the close (GOD: 18% winners, average win 6.8 × average loss).
- **An opening-range break only pays on the side of the overnight gap** (PF 1.38 with the gap, 1.00 against it).
- **The noise-band rule applied to the overnight session lost** (PF 0.80). Holding long overnight made money to 2021 and lost after, with a drawdown six times NBM's.
- **All three scripts are long volatility.** 2017 and 2019 made nothing. Correlation between them is 0.35–0.53.

## 4. Open items for the first Replay

- A MARKET parent and a STOP parent, each with `Stop1Offset` and no target: is the attached stop created at fill ∓ offset? (`FILL VERIFY ok`, difference 0.00.)
- Does the TALLY after the chart loads agree with the Replay result for NBM and TVP? (It will not for GOD, #77.)
- Entry timing: how many seconds after the bar close does the first study call arrive at your chart update interval? The scripts log it when it exceeds the limit.
- Out-of-sample: everything after 2024-03-07.

## 5. Research record, second batch (2026-11-11)

- **NQ's relative strength against ES in the first half hour persists to the close** (PF 1.31 / 1.59; all 17 settings positive; also against YM). Entered later in the day the effect is gone.
- **Adding to an NBM trade that is already ahead pays** (PF 1.37 / 1.62) but is the same bet again.
- **ES breaking its noise band before NQ does** gives a low-correlation entry (0.12 with the rest) that is thin in 2017–2021.
- **Rejected:** band retest, lunch-range breakout, relative strength after 11:00, ES confirmation as an NBM filter.

## 6. Research record, third batch (2026-11-11)

- **Adding to a trade that is already ahead worked on every base leg tried** (NBM, TVP, RSL, GOD, ESL), in both periods. It is leverage on trend days, not a new edge: it raises the worst month as well as the average.
- **The first 15 minutes carry the same information as NBM's 10:00 decision** (EDB, PF 1.39 / 1.54); at 10 or 20 minutes it is weaker.
- **Fading still loses, even restricted to low-volatility regimes.** That was the last reasonable place for a mean-reversion leg to hide.
- **Rejected:** opening volume surge, resting VWAP limit without confirmation, overnight relative strength, weekday effects.
- **Count so far: about 45 ideas tried, nine legs kept.** Treat every measured figure as an upper bound.

## 7. Reversion trio (2026-11-11): DailyWashoutRebound, ValueAreaReentry, CompressionExpansionBreakout

91. **`snprintf(dst, n, "%s", dst)` is undefined behaviour.** The cancel retry called `CancelPending(T.cancelWhy)`, which copied the stored reason onto itself. g++ 13 at `-O2 -Wall` reports it (`-Wrestrict`); the earlier `-Wall -Wextra` builds did not. **Fix: `if (why != T.cancelWhy)` before the copy.** Fixed in `gen_rev`; still present in `gen/tmpl_trader.inc` and the three momentum single-strategy files. Compile the mock build at `-O2` as well as `-O0`.

92. **To enter AT the session open, decide on the bar that ends at the open.** The order layer trades a signal on the first call after its bar closes. A decision on the first session bar enters at 09:35 and gave up a third of DWR's edge. The engine now processes the one pre-open bar (`tod + bar seconds == session open`), builds the day's references there, and emits the signal on it. The chart must hold that bar (overnight session loaded). No change to the order layer was needed.

93. **Build a day's references once per date, at the first bar handled for it, separately from the session reset.** With a pre-open decision the old "new session" block would have cleared a pending signal and the day's trade count at 09:30. Banking the finished session and resetting the day state now happen together at the first bar of the date (pre-open bar or first session bar); the session's own statistics start at the first session bar.

94. **Back-adjust the research data the way the chart will be.** Unadjusted contract data puts a false return on every roll day (about 1% a quarter for NQ in 2022–23), which is enough to move an RSI of daily closes. The feed is now difference-adjusted at each roll, offsets rounded to the tick.

95. **A volume profile from 5-minute bars (volume spread evenly over each bar's range, 1-point bins) gave the same value-area result as one from 1-minute bars.** `sc.VolumeAtPriceForBars` is not needed, and the study stays testable in the mock.

**Research record, fourth batch.** Mean reversion inside the day lost again in every new form (overnight session, 1–15-minute shocks, value-area edges, initial balance, late-day VWAP, hedged NQ–ES spread, sweep-and-reclaim, volume climax). It works across days (buy the open after a daily washout; the sell-side mirror does not) and against yesterday's value area. A daily-loss halt tighter than one full stop turned DWR negative. About 75 ideas tried in total; twelve kept.

## 8. Fourth new script (2026-11-11): EndOfDayFlow

96. **Normalisation can decide whether an edge exists.** The end-of-day flow measured as a move in average day ranges was flat in 2017–2021; as a percent move from the prior close it was positive in both periods. Use the unit the mechanism works in: leveraged-ETF rebalancing is proportional to the percent return.

97. **Check exit-minute sensitivity for any trade that is held only minutes.** EDF's 2017–2021 profit factor moved from 1.54 (exit 15:55) to 1.25 (15:50 or 15:59). A 5-minute shift that changes the result that much means that period's edge is thin.

98. **Never write a MEASURED block before the final mock run.** EDF's header was drafted with provisional figures and replaced by the measured ones before the commit. Generate the header from the report, or write it last.

**Research record, fifth batch.** Positive in both periods only for the end-of-day flow (percent move, NQ). Rejected: month-end rebalancing, pre-holiday, pre-FOMC drift, cross-day NQ–ES / NQ–YM relative-value reversal, same-time-of-day persistence, narrow initial-balance breakout, VWAP fade in high-volatility regimes. The "down yesterday" cells of the Yu–Rentzler–Wolf sign table are the washout script again.

## 9. DiversifiedSuite (2026-11-11): four legs on one account

99. **A pre-open decision and session-bar decisions can share one engine.** DWR decides on the bar that ends at 09:30; VAR, CEB and EDF decide on session bars. With the "references once per date" rule (#93) the three session-bar legs trade exactly as in their single files, whether or not the pre-open bar is processed (verified fill for fill).

100. **Reconcile a multi-leg study against its single files, not only against itself.** Each leg alone must reproduce its single file exactly; all legs together must equal the single files' trades minus the refused legs. Here both held to the cent, which also proves the refusal rule refused nothing else.

101. **Measure the cost of the one-net-position rule before building.** Here 5% of signals were refused, worth about $12 a month; in MomentumSuite about 2%. If the refused legs had been a large share of the edge, separate accounts would have been the better design.

102. **A cap on the open risk of all legs is the portfolio-level stop-loss a multi-leg study needs.** Per-leg caps do not limit three legs open in the same direction. 'Max $ risk of all open legs' sums each open or working leg's stop distance x contracts and reduces, then skips, the new leg.
