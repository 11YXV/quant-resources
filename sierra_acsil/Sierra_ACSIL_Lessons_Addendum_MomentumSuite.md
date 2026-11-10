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
