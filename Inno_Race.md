# Inno_Race — Step-by-Step Implementation Plan

> LAN multiplayer hover-car time-attack racing game · Unreal Engine 5.7.2 · outer-space theme · controller supported · built for 1000+ participants (Innovision 2026)

**Status:** Draft v2 — **peer-hosted, no backend.** The first player into a lobby becomes the host (listen server); later players join as clients.

**What changed from v1:** the backend service, orchestrator, matchmaker, dedicated server processes, admin web dashboard and big-screen web page are removed. Their jobs move into the game itself: LAN session discovery and hosting (Phase 7), local data storage (Phase 6), and an in-game admin menu (Phase 8). A side benefit: a listen server does not need a source-built engine, so the standard Launcher build of UE 5.7.2 is enough.

**What changed in v2.1:** the hover car is now a simple, game-based model (no real hover simulation). Gravity is off, the speed thrust has **5 automatic clutch levels** (no manual shifting), and **fuel and boost pickups** float on the track. Phase 1, the track/pickup steps in Phase 2, the edge cases (new group L, E-91 to E-100) and the class list were updated to match.

**How to read this document**

- Phases are ordered so there is a **playable build at the end of every phase**.
- IDs: `L-n` locked decision · `D-n` open decision · `S-n` screen · `E-n` edge case · `C.Key` config key.
- Values marked *default* are starting points to tune in playtests, not requirements.
- Effort figures are rough solo-developer estimates in working days.

---

## 1. Decisions

### 1.1 Locked (from you)

| ID | Decision |
|---|---|
| L-1 | Simultaneous race on the same track; **cars pass through each other** (no vehicle-vs-vehicle collision). Winner = lowest finish time. |
| L-2 | PvP / PvPvP: two or more players per match. Plan supports 2–4; default 2. |
| L-3 | Two match types, **toggled from the Settings menu**: **Type A** = one match of **3 races on 3 different tracks** (T1, T2, T3). **Type B** = one match of **1 race on a different track** (T4). |
| L-4 | LAN only. Unreal Engine 5.7.2. Keyboard and gamepad both supported. |
| L-5 | Three selectable hover cars. |
| L-6 | UI flow follows the wireframes (screen inventory in Appendix B). |
| L-7 | **No backend.** The **first player to join the lobby becomes the host** (listen server). Later players are **clients** by default. |
| L-8 | **Game-based hovering** (no real hover simulation), gravity off (about 0). Speed thrust has **5 clutch levels** (1 = the car starts hovering, up to 5) that change **automatically**. **Fuel and boost pickups** float on the track at designed intervals. |

### 1.2 Open decisions (with the default this plan assumes)

| ID | Question | Default used in this plan |
|---|---|---|
| D-1 | Type A match winner: total time or most race wins? | **Total time** over 3 races. Tie-break: more race wins, then faster last race. |
| D-2 | Is Type B's track a 4th unique map? | **Yes** (T4). Four race maps in total. |
| D-3 | Identity model. | **Name-only**, stored locally. Names are made unique **inside a lobby** (auto-suffix); the same name on different stations is allowed. |
| D-4 | Attempts per participant. | **Unlimited** by default. If `C.MaxAttempts` > 0 it is enforced only on the station where the name was created. |
| D-5 | Players per match. | **2** default. PvPvP (3–4) opt-in through config. |
| D-6 | DNF scoring in Type A. | DNF race time = slowest finisher of that race (or the track time limit if nobody finished) **+ 10 s**. |
| D-7 | Host advantage (the host's own car has zero network delay). | Accept; reduced by timestamped movement samples and equal-spec PCs (section 7). |
| D-8 | Hardware: number of stations and PC specs. | Unknown. Sizing in section 10. |
| D-9 | Solo runs (no opponent) count on the leaderboard? | **Yes**, flagged as solo (configurable). |
| D-10 | Leaderboard scope without a backend. | **Per-station local leaderboard** + CSV export for organizers to merge. Optional LAN result sharing is described in Phase 8 (item 6). |
| D-11 | Accelerator input. | **Hold-to-accelerate** (RT / W); the clutch shifts automatically. An auto-accelerate accessibility mode is a one-line config if you want it. |

### 1.3 Tunable defaults (config keys)

| Key | Default | Meaning |
|---|---|---|
| `C.MatchMode` | B | Local setting: A or B (section 8) |
| `C.StationId` | — | Label for this PC, shown on leaderboard rows |
| `C.PlayersMin` / `C.PlayersMax` | 2 / 2 | Players per match (PvPvP: max 4) |
| `C.SearchSec` | 3 | LAN search window before becoming host |
| `C.SessionJitterMs` | 0–1500 | Random delay before hosting, so simultaneous searchers don't all host |
| `C.MergeScanSec` | 2 | A host alone in its lobby rescans for other lobbies at this interval |
| `C.QueueFillSec` | 15 | After min players are ready, wait this long for more (PvPvP only) |
| `C.QueueFallbackSec` | 20 | Host alone this long → offer a solo run |
| `C.ReadyTimeoutSec` | 30 | Lobby ready check |
| `C.LoadGateSec` | 45 | Max wait for all clients to finish loading |
| `C.CountdownSec` | 3 | 3-2-1-GO |
| `C.GraceSec` | 30 | After the first finisher, the others get this long |
| `C.IntermissionSec` | 10 | Auto-continue between races (Type A) |
| `C.PlacementBannerSec` | 4 | Win / 2nd / DNF banner before Results |
| `C.DnfPenaltySec` | 10 | See D-6 |
| `C.MaxAttempts` | 0 (off) | See D-4 |
| `C.NameMin` / `C.NameMax` | 3 / 16 | Name length |
| `C.IdleWarnSec` / `C.IdleToMenuSec` | 15 / 60 | Menu idle → Main Menu, session data cleared |
| `C.NetTimeoutSec` | 6 | Connection timeout (lobby and race) |
| `C.SummaryAckSec` | 5 | Host waits this long for clients to confirm they received the final summary |
| `C.FpsCap` | 60 | Fixed on every station for fairness |
| `C.NetSendHz` | 30 (60 near the finish) | Movement samples sent to the host |
| `C.InterpDelayMs` | 100 | Remote-car interpolation buffer |
| `C.SpeedSanityFactor` | 1.15 | Host sanity limit = clutch-5 top speed × boost factor × this value |
| Per-track: `ParTimeSec`, `TimeLimitSec` | par ×3 | Hard cap on race length |

---

## 2. System overview

```
 Station 1 — HOST (first into the lobby)        Stations 2..4 — CLIENTS
┌─────────────────────────────────────┐       ┌────────────────────────────┐
│ Full game + listen server:          │  UDP  │ Full game:                 │
│  match state machine, timing,       │◄─────►│  UI, hover car, local      │
│  scoring, session advertisement     │gameplay│  store                    │
│  local store                        │       └────────────────────────────┘
└─────────────────────────────────────┘
          LAN session discovery (UDP broadcast): clients find open lobbies
```

Every station runs the identical build. Whoever finds no open lobby creates one and becomes the host for that match. When the match ends the session closes and everyone is a free agent again.

**Key design principles**

1. **One match loop, driven by a playlist.** Type B is a playlist of length 1; Type A is length 3. No separate code paths.
2. **The lobby owns its mode.** A lobby advertises its mode; stations only see and join lobbies of **their own** mode and build.
3. **Host-authoritative timing.** Clients never decide their own finish time; the host computes it from timestamped movement samples.
4. **No external services.** Everything runs inside the game; any station can host.
5. **Every wait has a timeout and every screen has an exit.**
6. **Results survive the session.** The host sends each race result and a final summary to every client; each station stores its own player's result locally before the session closes.

**Technology**

| Layer | Choice | Note |
|---|---|---|
| Game | UE 5.7.2 C++ (Blueprints for UI styling) | Launcher build is enough (listen server) |
| UI | CommonUI + Enhanced Input | Gamepad focus, Back actions, glyph switching |
| Networking | Listen server; owner-authoritative movement | L-1 means no collision conflicts to resolve |
| Discovery | LAN sessions via Online Subsystem Null, or a small custom UDP broadcast beacon | Decide in the Phase 0 spike |
| Storage | Local JSON-lines file + settings file per station | Atomic writes, rolling backup, CSV export |

---

## 3. State machines

### 3.1 UI flow (client)

| ID | Screen (wireframe) | Entry | Exits |
|---|---|---|---|
| S-01 | Main Menu | App start, session reset, idle timeout | New Participant → S-02 · Leaderboard → S-12 · Settings → S-13 · Credits → S-14 |
| S-02 | New Participant (name entry) | From S-01 or Back from S-03 | Confirm → S-03 · Back → S-01 |
| S-03 | Car Selection | Name confirmed; Play Again | Find Match → S-04 · Back → S-02 |
| S-04 | Searching | Find Match | Lobby found → S-05 as **client** · none found → S-05 as **host** · Cancel → S-03 |
| S-05 | Match Lobby / VS | Joined or hosting | All ready → S-06 · Leave / ready timeout → S-03 · Host closed lobby → back to S-04 automatically (once) |
| S-06 | Loading ("Travelling to the COSMOS…") | Host starts travel; between races | Loaded → S-07 · Failure → S-15 or S-04 |
| S-07 | Countdown (3-2-1-GO) | All loaded | → S-08 |
| S-08 | Race HUD (+ Pause overlay) | GO | Finish / DNF → S-09 · Host lost → S-15 |
| S-09 | Placement banner (Win / "You finished 2nd!" / DNF / Opponent disconnected) | You finished or were marked DNF | Auto after `C.PlacementBannerSec` → S-10 (Type A, races 1–2) or S-11 |
| S-10 | Intermission (Type A) | Race 1 or 2 ended | Auto after `C.IntermissionSec` → S-06 (next map) |
| S-11 | Results ("Game Finished"), with **Waiting** sub-state while others are unfinished | Final race ended for you | Main Menu → S-01 · Leaderboard → S-12 · Play Again → S-03 |
| S-12 | Leaderboard (local) | From S-01 or S-11 | Back → caller |
| S-13 | Settings | From S-01 (or Pause overlay) | Back → caller |
| S-14 | Credits | From S-01 | Back → S-01 |
| S-15 | Connection Lost / Host left / Opponent disconnected | Link failure | Any button → S-01 |
| S-16 | Modals | Anywhere | Confirm Leave · Admin PIN · Error · Controller Disconnected · Idle Warning · Solo prompt |

A navigation stack tracks "caller" so Back returns to the screen that opened it.

### 3.2 Match states (host game mode)

```
Lobby → LoadGate → Countdown → Racing → Finishing(grace) → RaceResults
   → [Intermission → seamless travel → LoadGate …] → MatchComplete → Detach
```

| State | Enter when | Leave when |
|---|---|---|
| Lobby | Session created | Everyone ready and ≥ `C.PlayersMin` (or solo confirmed), or timeouts |
| LoadGate | Host starts travel (session locked: no more joins) | All report "map loaded", or `C.LoadGateSec` (late ones are dropped) |
| Countdown | Gate passed | `StartServerTime` reached (host time + `C.CountdownSec`) |
| Racing | GO | First finisher, or race time limit, or all other players gone |
| Finishing | First finisher | All finished, or `C.GraceSec` expired → unfinished = DNF |
| RaceResults | Finishing ended | Compute ranks/times; send race result to every client; store locally |
| Intermission | More races in the playlist | `C.IntermissionSec`; then seamless travel |
| MatchComplete | Playlist done | Final standings computed; summary sent reliably to each client |
| Detach | Summary delivered (all acks, or `C.SummaryAckSec`) | Every station leaves the session and shows Results **offline** from its stored copy; the host closes the session |

### 3.3 Master sequence

```
Station A (becomes host)                 Station B (client)
 Search LAN ── nothing found ──►
 Create session (mode, build, slots)
                                          Search LAN ── finds A ──► Join
 Lobby ◄────── ready flags (replicated) ──────► Lobby
 All ready → lock session → ServerTravel(T1) ─► clients follow
 LoadGate → Countdown (host time) → Race → Finish → Results
 Host sends race results + final summary ─────► each station stores its own result
 Detach: both leave the session; Results screen runs offline on each station
```

---

## 4. Phased implementation

### Phase 0 — Foundations and LAN spike (2–3 days)

**Goal:** two PCs on the LAN: one hosts, the other finds and joins.

1. Install UE 5.7.2 (Launcher build). Create C++ project `InnoRace`, single game module.
2. **LAN session spike (do this first).** Try Online Subsystem Null LAN sessions (create / find / join). If it is awkward, write a small UDP broadcast beacon instead and use direct `open <ip>` travel. Verify on your **real network**: switches, AP isolation or VLANs can block broadcast; use wired Ethernet.
3. Enable plugins: CommonUI, Enhanced Input, Niagara (and Online Subsystem Null if you use it).
4. Source control: Git + Git LFS (or Perforce). Naming: `BP_`, `WBP_`, `DA_`, `M_`, `T_`. Folders: `Content/InnoRace/{Cars,Tracks,UI,Maps,Data,Audio,VFX}`.
5. Config: `DefaultGame.ini` defaults; per-station `InnoRace.ini` (mode, StationId, admin PIN hash, tunables); command-line overrides such as `-StationId=`.
6. Logging categories: `LogInnoNet`, `LogInnoRace`, `LogInnoUI`, `LogInnoStore`.
7. Stamp a **BuildId** into every build; sessions advertise it and stations only list sessions with the same BuildId (E-33, E-40).
8. Windows firewall: package a one-click script (`netsh advfirewall`) that allows the game's UDP ports on all profiles so the first-run prompt never appears on stage (E-41).
9. Single-instance lock so a station can't run two copies.

**Acceptance:** one station hosts, the other finds it in under `C.SearchSec`, joins, both load an empty map; BuildId logged on both.

### Phase 1 — Hover car vertical slice, offline (5–8 days)

**Goal:** a simple, game-based hover car that is easy to drive with keyboard and gamepad. There is **no real hover simulation**: gravity is off, speed comes from an automatic 5-level clutch, and a small assist keeps the car floating above the track.

**Controls (kept simple)**

| Action | Keyboard | Gamepad |
|---|---|---|
| Steer | A / D | Left stick |
| Accelerate (hold) | W | RT |
| Brake (hold) | S | LT |
| Pause | Esc | Start |

No manual shifting, no reverse, and no manual boost or reset buttons. Boost triggers automatically when collecting a Boost pickup orb; resetting is automatic if the vehicle goes off-track (1.5s timeout). Trigger pressure scales acceleration or braking rate; the top speed of each band is the same regardless.

**Car anatomy**

- **4 hover nozzles** under the body: ray origins for the hover assist; their glow shows how hard the assist is working. They apply no force.
- **2 rear speed nozzles:** flame size follows the clutch level and grows during boost.
- **One physics body with gravity off** (`bEnableGravity = false`; race maps override world gravity to 0). Keep a `GravityScale` of 0–0.1 as a tunable. Vehicle-vs-vehicle collision stays off (L-1). Do not use Chaos Vehicles.

**Clutch bands (automatic)** — starting values, to be tuned with test laps

| Clutch | Name | Band top speed (m/s) | Accel (m/s²) | Turn rate (°/s) | Fuel burn (/s while accelerating) |
|---|---|---|---|---|---|
| 1 | Drift | 20 | 18 | 120 | 0.6 |
| 2 | Cruise | 35 | 14 | 100 | 0.9 |
| 3 | Pulse | 50 | 11 | 85 | 1.3 |
| 4 | Surge | 65 | 9 | 70 | 1.7 |
| 5 | Nova | 80 | 7 | 58 | 2.2 |

The names are placeholders for the "Name" plate in your HUD sketch.

**Auto-shift rules** (pure logic, unit-tested)

- Holding accelerate: the speed target is the current band's top speed, with the band's acceleration scaled by trigger pressure.
- **Upshift** when speed ≥ 97% of the band's top speed for 0.15 s (no dwell while boosting).
- **Downshift** when speed < 90% of the previous band's top speed. The gap between the two thresholds prevents shift flutter.
- Coasting (no input) decelerates at 8 m/s²; braking at 40 m/s². The clutch follows the speed down through the bands.
- **Turn rate** comes from the current band, smoothed over about 0.2 s. Sideways slip decays quickly (grip about 6 per second), which gives a light drift.
- **Clutch 1 engages at GO**, and the car lifts from its landed height to hover height over about 0.4 s. Before GO the car sits landed and input is ignored.
- The tightest corner a band can take is about `top speed ÷ turn rate in rad/s`: clutch 3 ≈ 34 m, clutch 5 ≈ 79 m. Design corner radii against this so sharp corners force the player to lift or brake.

**Hover assist (the "fake" hovering)**

1. Every physics step, cast 4 short rays down from the nozzles. Average the hit distances and the surface normals.
2. Hold the car at `HoverHeight` (starting value 1.2 m) with a smooth damped move; no springs.
3. Rotate the car's up direction toward the average normal so it follows banked and curved tracks.
4. Tilt only the **visual mesh** for steering bank and bob; the collision body stays level.
5. If no ray hits for 1.5 s, the car is off the track and respawns at the last checkpoint (gravity is off, so this replaces "falling").

**Fuel**

- Capacity 100. Fuel burns only while accelerating, at the band's rate, so coasting and braking are free.
- Fuel pickups add 20 each; overflow is wasted. At 20% the HUD flashes and the audio changes.
- **At 0 the car goes into limp mode:** the clutch is locked to band 1 (top speed 20 m/s), burn stops. A fuel pickup restores normal clutching. The car can always finish, so fuel never causes a DNF by itself.
- **Budgeting rule:** run a flat-out test lap (accelerate always, brake only where needed). Set capacity plus all fuel pickups on the track to about 90–100% of the fuel that lap burns, so the best line needs the pickups and lifting in corners.

**Boost**

- Automatically triggered upon collecting a **Boost orb** pickup (no button press required).
- Activates rocket boost for 2.5 s: accelerates as if the accelerator were held, with acceleration ×2, top speed up to ×1.35 of the clutch-5 top speed (108 m/s), and turn rate ×0.85. Boost burns no fuel. Collecting another boost orb while active refreshes or extends the boost duration.

**Pickups**

- Designers place `APickupSpawnPoint` actors (type, amount) floating on the track at about hover height.
- **Fuel cell:** amber hex prism. **Boost orb:** cyan sphere with a ring. They differ in shape as well as colour (colour-blind friendly).
- Placement guideline: none in the first ~10% of the track; fuel roughly every third of the track on the main line; boost orbs on riskier lines.
- **Every player has their own copy of each pickup**, collectable once per race, never respawning. This is fair in a time attack and keeps networking trivial: pickups are local actors with fixed IDs per track and are not replicated.

**Steps**

1. `AHoverCarPawn`: physics body with gravity off, 4 nozzle sockets, 2 speed-nozzle sockets, a child visual mesh for banking.
2. `UHoverAssistComponent`: the 4 rays, height hold, alignment, off-track timer.
3. `UHoverClutchComponent`: the auto-shift state machine.
4. Movement in a fixed physics step (async physics tick or substepping), in this order: read input → update auto-clutch → set speed target (brake = 0; boost overrides) → move forward speed toward the target with the band's acceleration → damp sideways slip → apply steering → run the hover assist → set the body's velocity → burn fuel.
5. `UHoverFuelComponent` and `UHoverBoostComponent` (limp mode, charges, timers).
6. `UHoverCarData`: the 5 bands, hover/fuel/boost parameters. The 3 cars differ **only in this data**, with an equal total stat budget so none is dominant.
7. Input: Enhanced Input contexts `IMC_Gameplay` (KB + gamepad) and `IMC_UI`; dead zones and trigger curves.
8. Camera: spring arm, speed-based FOV; lock camera and FOV values so settings can't give an advantage (E-90).
9. **Frame-rate independence:** fixed step and `C.FpsCap` applied at startup. A faster PC must not drive faster.
10. Safety: off-track respawn, stuck reset (hold Reset), wall scrape (impacts reduce forward speed, and the clutch follows the speed down).
11. Greybox test track with a few corners sized per band, 2 fuel pickups and 2 boost pickups.
12. HUD: clutch gauge (5 stepped bars, widest on top, filled from the bottom up to the current clutch, the number above and the name plate below), fuel bar, 3 boost pips, speed readout.

**Acceptance:** a 60–90 s lap is fun at 60 FPS; the clutch shifts smoothly with no flutter near thresholds; fuel, limp mode and boost behave as described; each pickup is collected exactly once; behaviour is within tolerance at 30/60/144 FPS; the 3 cars are distinguishable; no tunnelling at top or boost speed.

### Phase 2 — Race logic, offline (3–4 days)

1. `ATrackRoot` with ordered checkpoint triggers, finish plane, start grid slots (up to 4).
2. `UTrackData` per track: map soft reference, `ParTimeSec`, `TimeLimitSec`, checkpoint count, display name, and a pickup summary (total fuel amount, boost pickup count).
3. **Finish rule:** all checkpoints visited in order, then crossing the finish plane. Crossing time is interpolated between the two samples that straddle the plane: `t_cross = t_prev + (t_now − t_prev) · d_prev / (d_prev − d_now)`, where `d` is the signed distance to the plane.
4. HUD: timer, speed, clutch gauge, fuel bar, boost pips, checkpoint split, wrong-way warning, "Missed checkpoint" arrow.
5. Respawn (E-49/E-50), wrong-way (E-51), skipped-checkpoint rejection (E-52).
6. Editor validation tool: checkpoints ordered and spaced, finish present, 4 grid slots, kill volumes cover the void, every pickup has a unique ID and sits within hover reach of the track.
7. **Pickups on the track:** place fuel and boost pickups per the Phase 1 rules; the validation tool reports the fuel budget (capacity + pickups) so you can compare it with the flat-out test lap.

**Acceptance:** an offline lap produces a correct time; skipping a checkpoint blocks the finish; the validation tool passes on all tracks; pickups are collected once and counted in the fuel budget.

### Phase 3 — UI framework and navigation, mock data (6–9 days)

**Goal:** every screen reachable and escapable with gamepad only and with keyboard only.

1. `UInnoUIFlowSubsystem` (GameInstance subsystem) owns the navigation stack and screen registry (`EInnoScreen` ↔ widget class). It survives map travel and session changes.
2. Root layout with layers: Menu stack, Modal stack, HUD/overlay layer, Toast layer.
3. `UInnoActivatableWidget` base (CommonActivatableWidget): default focus widget, Back action, idle-timer reset on any input.
4. Navigation operations: `Push`, `Pop`, `ReplaceRoot`, `ResetToMainMenu` (clears session data).
5. **Gamepad on-screen keyboard** (grid; also accepts a physical keyboard) and a "Random callsign" button.
6. Input-type switching: glyphs follow the last-used device (CommonUI).
7. Shared widgets: toast, confirm modal, admin PIN modal, idle warning, controller-disconnected overlay, solo prompt.
8. Build each screen against mock view-models: Main Menu, Name Entry, Car Selection (3D showcase in the garage map), Searching, Match Lobby (N-row list), Loading, Countdown, HUD, Placement banner variants, Intermission, Results (table: N players × M races, with a Waiting sub-state), Leaderboard (paged), Settings, Credits, Connection Lost variants.
9. All strings via String Tables (localization-ready), which also fixes typo drift (Participant, Finished, Continue).
10. Design at 1920×1080 with DPI scaling and safe zones; test 1080p, 720p, ultrawide.
11. Automated navigation-graph test: every screen reachable, every screen has at least one exit.

**Acceptance:** complete menu walkthrough with gamepad only and with keyboard only; no dead ends; no focus loss after a mouse click or device swap.

### Phase 4 — Networking: host and clients, 2-player race (7–10 days)

1. **Classes:** `AInnoRaceGameMode` (match states, runs on the host), `AInnoRaceGameState` (replicated state, `StartServerTime`, `RaceIndex`, grace deadline), `AInnoPlayerState` (name, car, ready, loaded, last checkpoint, finish time, status), `AInnoPlayerController`.
2. **Join validation** in `PreLogin`: reject if the lobby is not in the Lobby state, the session is locked, the BuildId differs, or the lobby is full (E-40, E-42). If a joining name already exists in the lobby, the host appends "(2)", "(3)" (E-03).
3. **Movement:** owner-authoritative. Each client simulates its own car and sends samples at `C.NetSendHz` (60 Hz near the finish line). **Every sample carries the sender's estimate of host time.** The host validates and replicates; remote clients interpolate with the `C.InterpDelayMs` buffer, extrapolate at most ~200 ms, then freeze (E-55). Also replicate the clutch level (1 byte) and the boost/limp flags for engine glow on remote cars; fuel and boost meters stay local.
4. **Host sanity checks** on client samples: speed above `C.SpeedSanityFactor × (clutch-5 top speed × boost factor)` (about 124 m/s with the starting values), impossible jumps, checkpoint order (E-53).
5. **Synchronized start:** the host sets `StartServerTime = host world time + C.CountdownSec`. Every client counts down against synced host time. Input stays disabled until that moment; the host ignores or clamps movement before it.
6. **Finish detection on the host:** from the two timestamped samples straddling the finish plane (the host's own car uses its local samples). Race time = `crossing time − StartServerTime`, stored as integer milliseconds. This keeps the host's zero-latency advantage down to clock-sync error (a few ms on a wired LAN).
7. **Finishing rules:** first finisher starts the grace timer; all finished → end immediately; grace expiry or time limit → unfinished = DNF.
8. **Disconnects:** a dropped client → DNF for the remaining races (E-62/E-63). **A dropped host ends the match for everyone** (E-65): there is no host migration.
9. **Spawn grid:** slots assigned randomly per match.
10. **Test under bad networks:** `Net PktLag`, `Net PktLoss` on 2+ real machines; also test with the lowest-spec PC acting as host.

**Acceptance:** two stations complete a race; both show the same ranking; start moments agree; killing a client yields DNF for that player and a clean result for the host; killing the host sends the client to Connection Lost within `C.NetTimeoutSec`.

### Phase 5 — Match playlist, seamless travel, intermission (4–6 days)

1. `UInnoMatchConfig` (primary data asset): mode ID, ordered playlist of `UInnoTrackData`, penalties, timeouts. Two instances: `DA_TypeA` (T1→T2→T3) and `DA_TypeB` (T4).
2. Host loop: for each track in the playlist → Race → Intermission → seamless travel. Override `PlayerState::CopyProperties` so name, car and accumulated times carry across maps.
3. **Scoring library** (plain C++, unit-tested): `RankRace`, `ApplyDnfPenalty`, `ComputeStandings` with the D-1 tie-break rules. The only place scoring rules exist.
4. Preload the next map and assets during Intermission (async soft-reference loading). Keep sky, materials and car assets shared across maps to cut load time.
5. Each race has its own LoadGate, countdown, grace timer, and time limit.
6. **Placement vs final outcome:** per-race placement is known the moment you cross the line (nobody who finishes later can beat you). In Type A the **final** banner waits until standings are final, after the last race's grace window.
7. After each race the host sends the **race result** to every client (reliable); at the end it sends the **final summary** (`FInnoMatchSummary`). Clients keep these copies, which is what makes the Detach step safe.
8. Intermission shows your race placement, running total, next track name and the auto-continue timer.

**Acceptance:** scoring unit tests pass (ties, DNF, all-DNF, solo, drop-outs); a 3-race Type A match completes with 2 stations including one DNF and one client disconnect; Type B runs through the same loop.

### Phase 6 — Local data and storage, no backend (2–3 days)

1. `UInnoLocalStore` subsystem. Results live in a **JSON-lines file** under `Saved/InnoRace/`, appended and flushed per record; a rolling backup copy is made after every completed match. A corrupt last line is skipped and logged, never fatal.
2. Records: `LocalParticipant` (id, name, StationId, attempts used), `LocalRaceRecord` (MatchId, Mode, RaceIndex, TrackId, TimeMs, Status, Rank, CarId, PlayerCount, Solo), `LocalMatchRecord` (final rank, total time). A MatchId GUID prevents double-recording (E-67).
3. Name validation: charset and length, blocked-word list from `InnoRace.ini` (E-01…E-05). The same name on the same station is the same participant ("Welcome back"; E-13).
4. Settings: `GameUserSettings` plus `InnoRace.ini` (mode, StationId, admin PIN hash, tunables).
5. Leaderboard computation: best-per-name for the active mode, tie handling, paging, prefix search; rows show the StationId tag.
6. **CSV export** from the admin menu to `Saved/InnoRace/Exports/`, with an option to copy to a removable drive path.
7. Attempts are consumed **only when a match completes** (E-66), and enforced only if `C.MaxAttempts` > 0.
8. Failure handling: if a write fails, keep the record in memory, retry, and show an admin banner (E-09).

**Acceptance:** restart the game mid-event and the data persists; a truncated last line doesn't crash it; the exported CSV opens in Excel.

### Phase 7 — LAN sessions: discovery, hosting, joining (3–5 days)

1. **Search:** find LAN sessions filtered by `Mode`, `BuildId`, joinable, open slots, not started. If several qualify, prefer the one with the most players, then the lowest `SessionId` (a random 64-bit value created with the session).
2. **Become host:** if nothing is found within `C.SearchSec`, wait a random `C.SessionJitterMs`, search once more, then create the session and enter the Lobby as host. Session settings advertise Mode, BuildId, HostStationId, SessionId, current/max players, Started flag.
3. **Merge rule:** a host that is still **alone** in its lobby rescans every `C.MergeScanSec`. If it finds another joinable lobby of the same mode with a lower `SessionId` (or more players), it closes its own lobby and joins that one. Only the higher SessionId yields, so two simultaneous hosts converge deterministically (E-20).
4. **Ready check** runs in the host's GameMode using replicated PlayerState flags. Idle clients are kicked after `C.ReadyTimeoutSec` (to Car Selection with a message); the host reopens the slot.
5. **Solo fallback:** a host alone for `C.QueueFallbackSec` sees "No opponent found. Start a solo run? / Keep waiting".
6. **PvPvP:** with ≥ `C.PlayersMin` ready and the lobby not full, the host waits up to `C.QueueFillSec` for more.
7. **Lock before start:** set the session non-joinable and stop advertising before travel.
8. **Host leaves in the lobby:** clients see a toast and auto-search again once (one of them will become the new host).
9. Admin diagnostics screen: lists discovered sessions and peers, BuildId, mode, and the local IP, to debug "nobody can find anyone".

**Acceptance:** 4 stations press Find Match at the same moment → exactly 2 lobbies of 2, no stuck duplicates; killing a host in the lobby → its client re-hosts or joins another lobby within ~10 s.

### Phase 8 — Leaderboard, Results, in-game admin menu (3–5 days)

1. **Leaderboard widget:** mode tabs (Type A / Type B), local paging (10–15 rows), prefix search, own row highlighted, ties share a rank (E-75). Never mix modes.
2. **Rules for boards:** Type A match winner = most race wins (tie-break: lowest cumulative total time); Type B = best finished race time. Aborted or abandoned matches are not eligible. Solo runs per D-9.
3. **Results screen:** rank, per-race times, gaps to the winner, car, Play Again, Main Menu, Leaderboard. It reads from the locally stored summary, so it works after the session has closed.
4. **Admin menu** (PIN, hidden entry in Settings): set the mode, view discovery diagnostics, export CSV, clear local data (with confirmation), rename or delete a local participant, void a local result, view the last errors.
5. Per-station "stats" panel: matches hosted/joined, average load time, average FPS (helps find slow PCs).
6. **Optional — LAN result sharing:** after each match, every station broadcasts a compact `ResultAnnounce` (MatchId, name, StationId, mode, time). Other stations merge it into a second "event-wide" board, deduplicated by MatchId + name. It is eventually consistent: a station that rebooted misses announcements unless it asks peers to resync on boot. Only build this if you want a live event-wide ranking without any central machine.

**Acceptance:** a Type A match between two stations leaves the correct total time and rank in each station's local leaderboard; the CSV export matches what the screen shows.

### Phase 9 — Content and polish (runs in parallel from Phase 1, ~10–15 days)

1. **Tracks T1–T4:** same visual identity, different layouts. Suggested length: ~60–90 s for Type A tracks, ~90–120 s for Type B (tune in playtests). Set `ParTimeSec` and `TimeLimitSec` from real test laps. Size corner radii against the clutch table (tightest radius ≈ speed ÷ turn rate in rad/s) and place fuel/boost pickups per the Phase 1 budget rule.
2. **Menu and garage maps:** live space background, live hover-car garage, car showcase loop sequence.
3. Audio: music, UI, engine, countdown, finish, DNF. Controller vibration for boost and finish.
4. VFX with Niagara within a budget for the lowest-spec station.
5. Performance: scalability presets, fixed FPS cap, check Lumen/Nanite/VSM cost on your real PCs. **The host runs the game and the authority logic, so test the weakest PC as host.** Warm up shaders/PSO in the menu to avoid hitches on the first race.
6. Fixed gameplay-affecting settings (FOV, camera) so cosmetic options can't give advantages (E-90).

### Phase 10 — Hardening and dress rehearsal (5–7 days)

1. Soak test 4+ hours with real stations; watch memory, handle leaks, FPS on the host.
2. **Chaos tests:** kill the host in the lobby / mid-race / on the Results screen; kill a client in each state; unplug a network cable on the host and on a client; unplug a controller; two stations hosting simultaneously; fill the disk; corrupt the results file; change the mode on one station only.
3. Run the full edge-case checklist (section 5) as pass/fail.
4. Backup/restore drill: restore the results file from the rolling backup; practice the CSV merge across stations.
5. Train volunteers with the runbook (section 10); print the quick-fix sheet.
6. Freeze the build, stamp the BuildId, create a rollback package and a one-click deploy script (build + firewall script + `InnoRace.ini`).

---

## 5. Edge-case catalogue

### A. Name entry and identity

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-01 | Empty name | Inline error; Confirm disabled |
| E-02 | Length and charset | `C.NameMin`–`C.NameMax`; letters, digits, space, `_`, `-`; trim and collapse spaces |
| E-03 | Two players with the same name in one lobby | Host auto-suffixes the later one ("Alex (2)"); shown consistently everywhere |
| E-04 | Blocked words | Rejected with a generic message; list lives in `InnoRace.ini` |
| E-05 | Emoji / unsupported Unicode / RTL | Rejected by the charset rule |
| E-06 | Gamepad user cannot type | On-screen keyboard and Random Callsign |
| E-07 | Controller disconnected during entry | Overlay "Reconnect controller"; keyboard still works |
| E-08 | Back from name entry | Nothing is saved; the participant is created locally only on Confirm |
| E-09 | Local save fails (disk full, permissions) | Keep in memory, retry, show an admin banner; the game still plays |
| E-10 | Double-press Enter | Debounced; creation is idempotent |
| E-11 | Person walks away after naming | Idle timeout resets the station |
| E-12 | Same person plays under different names | Not preventable without a central identity; organizers dedupe in the exported CSV |
| E-13 | Returning player types their old name on the same station | Same participant ("Welcome back"); on another station it is a separate local record |

### B. Car selection

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-14 | Cycling past the ends | Wraps; holding the stick repeats with a delay |
| E-15 | Back to name entry | Allowed; re-confirming the same name is not a duplicate |
| E-16 | Showcase not loaded / hitch | Placeholder image; input never blocked |
| E-17 | Changing car after entering the lobby | Not allowed; Leave returns to Car Selection |
| E-18 | `C.MaxAttempts` reached (if enabled) | "No attempts left"; Find Match disabled; Leaderboard still available |

### C. Discovery, hosting, joining

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-19 | No lobby found | After `C.SearchSec` + jitter the station hosts; after `C.QueueFallbackSec` alone, offer a solo run; Cancel is always available |
| E-20 | Two stations search at the same instant and both host | Merge rule: the one with the higher SessionId closes and joins the other; toast "Opponent found" |
| E-21 | Lobby fills while a client is joining | Join fails → automatic re-search (up to 3 tries), then host |
| E-22 | Stations set to different modes | They never see each other's lobbies. Searching and Lobby show the mode name; the runbook says to set all stations identically |
| E-23 | Odd number of players | The last one gets the solo fallback |
| E-24 | PvPvP, waiting for a 3rd/4th | Start with ≥ min ready after `C.QueueFillSec` |
| E-25 | Host station reboots or crashes while clients are in the lobby | Clients' connections time out → auto re-search once |
| E-26 | Network blip in the lobby | UE connection timeout (`C.NetTimeoutSec`) treats it as a leave; the client re-searches |
| E-27 | LAN broadcast blocked (VLAN, AP isolation, different subnets) | Everyone ends up hosting alone; the diagnostics screen shows zero peers; runbook: check cabling/subnet; fallback is a direct-IP join entered in the admin menu |

### D. Lobby

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-28 | A client never presses READY | `C.ReadyTimeoutSec` → host kicks them to Car Selection with a message; slot reopens. If the **host** idles, the lobby closes and clients re-search |
| E-29 | Client presses LEAVE | Host sees "Opponent left", slot reopens, lobby continues. **Host** leaving closes the lobby; clients auto re-search with a toast |
| E-30 | Everyone readies simultaneously | Host GameMode is single-threaded: the start fires exactly once |
| E-31 | Un-ready | Allowed until all are ready, then locked ("Starting…") |
| E-32 | Player count drops below minimum | Back to waiting, or the solo prompt |
| E-33 | BuildId differs between stations | Their lobbies are invisible to each other; after several empty searches show the hint "Possible version mismatch" |
| E-34 | Client crashes in the lobby | Host connection timeout frees the slot |

### E. Loading and connection

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-35 | Host fails to load the race map | Host shows an error and returns to Car Selection; clients get Connection Lost and re-search |
| E-36 | Client cannot connect to the lobby | Retry twice, then "Couldn't join lobby" and re-search |
| E-37 | One client loads slowly | LoadGate waits up to `C.LoadGateSec`; then that client is dropped (DNF for the rest) and the others proceed; opponent-left toast |
| E-38 | First-run shader hitching | Warm up in the menu; show real loading progress |
| E-39 | Game port already in use | Single-instance lock prevents a second copy; otherwise show an error and flag it in diagnostics |
| E-40 | BuildId mismatch at login | `PreLogin` rejects with a clear message |
| E-41 | Windows firewall prompt or block on first host | Pre-grant the rule with the Phase 0 script; verify on every station |
| E-42 | Joining after the match has started or the session is locked | `PreLogin` rejects; the session is also no longer advertised |

### F. Countdown and race start

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-43 | Client clock estimate is off | Uses synced host time; nobody moves before the host's start time |
| E-44 | Alt-tab / focus loss | Fullscreen-windowed with focus lock; input resets to neutral; no pause |
| E-45 | Controller unplugged mid-race | Non-pausing overlay; neutral input (accelerator released, so the car coasts down); keyboard accepted; replug resumes |
| E-46 | Keyboard and gamepad used together | Both accepted (clamped); glyphs follow last-used |
| E-47 | Different PC speeds | Fixed physics step and `C.FpsCap`; FPS below ~30 shows in the admin stats |
| E-48 | Pause pressed | Overlay only (Resume / Settings / Leave); the race continues; Leave asks for confirmation then counts as DNF |

### G. In-race

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-49 | Fell off the track (no track under the rays for ~1.5 s) | Respawn at the last checkpoint, speed zeroed, clutch back to 1, fuel and boost kept, clock keeps running |
| E-50 | Stuck | "Hold Reset" prompt after ~5 s near zero speed |
| E-51 | Wrong way | HUD warning, no penalty |
| E-52 | Checkpoint skipped | Finish rejected on the host; HUD arrow back to the missed checkpoint |
| E-53 | Modified client / speed hack | Host clamps or respawns; repeated violations → DNF. A tampered **host** cannot be detected, so run supervised stations |
| E-54 | Overlapping cars | No collision; ghost/translucent shading at close range |
| E-55 | Packet loss or lag on remote cars | Interpolation buffer; extrapolate ≤ ~200 ms then freeze; own car unaffected |
| E-56 | Nobody finishes | Track `TimeLimitSec` ends the race; everyone is DNF |

### H. Finish, results, scoring

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-57 | Two cars finish within one tick | Crossing interpolation; still equal in ms → shared placement; leaderboard tie-break by earlier recorded crossing |
| E-58 | First finisher | Placement banner immediately; grace timer starts for the others |
| E-59 | Everyone finished | End at once, no grace |
| E-60 | Finished after the grace window | DNF; the late time is not recorded |
| E-61 | Everyone DNF in a race | All get the same penalty time; Type A continues |
| E-62 | Type A: a client drops after race 1 | DNF penalty for races 2–3 (D-6) |
| E-63 | Opponent disconnects mid-race | Toast; you continue; after you finish the banner is "Opponent disconnected", never "YOU WON" over nobody |
| E-64 | You (a client) disconnect | Connection Lost → Main Menu; host marks DNF; your station records the match as abandoned (not leaderboard-eligible) |
| E-65 | **Host disconnects or crashes mid-race** | All clients get Connection Lost; the match is void; nothing is recorded as completed; attempts are not consumed; everyone can search again |
| E-66 | Attempts and incomplete matches | Attempts are consumed only when a match completes |
| E-67 | Summary received twice or after a restart | MatchId prevents double-recording |
| E-68 | Host closes the session before a client has the final summary | Host waits for acks up to `C.SummaryAckSec`; a client that missed it shows only the race results it already received, marked "summary unavailable" |
| E-69 | Type A: final winner depends on most race wins | Per-race banners auto-advance; the final banner waits for final standings (race win count, with total time as tie-breaker); Waiting sub-state shown meanwhile |
| E-70 | Clock drift across PCs | Irrelevant: all race times come from host time only; local timestamps in records are informational |

### I. After the match

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-71 | Idle on Results or menus | Warning at `C.IdleWarnSec`, then Main Menu at `C.IdleToMenuSec`; session data cleared |
| E-72 | Host presses Main Menu while clients still view Results | Safe: Detach already happened; Results run offline from the stored summary |
| E-73 | Play Again | Confirms, allows a new car, goes through discovery again (the same people are not guaranteed to be paired) |
| E-74 | Back from Leaderboard / Settings | Returns to the screen that opened it |
| E-75 | Large local leaderboard | Paging, prefix search, ties share a rank, long names truncated |

### J. Settings and mode toggle

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-76 | Non-admin opens the mode row | Read-only; the admin PIN modal is required to change it; lock out for 30 s after 3 wrong PINs |
| E-77 | Mode changed on one station mid-event | Applies to that station's next search only; lobbies already advertising the old mode are unaffected |
| E-78 | Settings file missing or corrupted | Reset to defaults (Type B) and show an admin warning |
| E-79 | Audio/video settings | Saved locally; "Reset to defaults"; gameplay-affecting values locked |

### K. Event operations

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-80 | Station reboots mid-match | Auto-launch to Main Menu; counts as E-64 (client) or E-65 (host) |
| E-81 | Wi-Fi in use | Not supported: wired Ethernet only |
| E-82 | The hosting PC is weaker than the others | Use identical specs, test the weakest PC as host, and keep graphics scalability fixed per station |
| E-83 | Time or date changes | Use monotonic clocks; local record timestamps are informational |
| E-84 | Power loss on a station | Appended JSON lines survive; the rolling backup covers a corrupt tail |
| E-85 | Disk full | Log rotation; admin banner (E-09) |
| E-86 | Switch failure | Stations show Connection Lost; runbook step |
| E-87 | Duplicate StationId on two PCs | Diagnostics flags duplicates when a peer's StationId matches the local one |
| E-88 | Version drift between stations | BuildId filtering prevents mixed matches (E-33) |
| E-89 | Disconnected station while searching | Search simply finds nothing; diagnostics show the cause |
| E-90 | Graphics options causing unfair visibility | Lock FOV, camera and gameplay-affecting post effects; expose only cosmetic options |

### L. Hover car, fuel, boost, pickups

| ID | Scenario | Expected behaviour |
|---|---|---|
| E-91 | Fuel reaches 0 | Limp mode: clutch locked to band 1 (20 m/s), burn stops, HUD turns red; a fuel pickup restores normal clutching |
| E-92 | Fuel runs out during boost | The boost finishes, then limp mode |
| E-93 | Boost pressed with 0 charges | Denied sound and HUD pulse; no effect |
| E-94 | Same pickup touched twice (respawn, lag) | Each pickup ID is collectable once per player per race |
| E-95 | State after respawn | Clutch 1, speed 0; fuel and boost keep their values; collected pickups stay collected |
| E-96 | Accelerator held before GO | Ignored until GO (no false start); at GO the car lifts and engages clutch 1 |
| E-97 | Speed hovering near a shift threshold | Hysteresis (97% up, 90% of the previous band down) plus a 0.15 s dwell prevent flutter; covered by unit tests |
| E-98 | Finish line crossed | Engine shuts down, controls disabled, the car glides to a stop |
| E-99 | Brake at standstill | No reverse; brake does nothing at 0 speed; Reset covers stuck cars |
| E-100 | Wall impact | Forward speed is reduced (scrape); the clutch follows the speed down automatically; tune so a wall is never faster than braking |

---

## 6. Local data model

**Files per station (`Saved/InnoRace/`)**

| File | Content |
|---|---|
| `results.jsonl` | One line per completed race/match record for this station's participants (append-only) |
| `results.bak` | Rolling backup, refreshed after each completed match |
| `participants.json` | Local participants and attempt counts |
| `InnoRace.ini` | Mode, StationId, admin PIN hash, tunables, blocked words |
| `Exports/*.csv` | Admin exports |

**Record shapes (simplified)**

```
LocalParticipant { Id GUID, Name, StationId, AttemptsUsed, CreatedAt }
LocalRaceRecord  { MatchId GUID, Mode, RaceIndex, TrackId, TimeMs, Status, Rank,
                   CarId, PlayerCount, Solo, SessionId }
LocalMatchRecord { MatchId GUID, Mode, TotalTimeMs, FinalRank, Completed, Abandoned }
```

**Session advertisement** (LAN session settings): `Mode`, `BuildId`, `HostStationId`, `SessionId`, `PlayersCurrent/Max`, `Started`.

**Replicated match data**

```
FInnoRaceResult  { RaceIndex, TrackId, Entries[ PlayerName, CarId, Status, TimeMs, Rank ] }
FInnoMatchSummary{ MatchId, Mode, Races[FInnoRaceResult], Standings[ PlayerName, TotalMs, FinalRank ] }
```

**CSV columns:** `MatchId, Mode, StationId, Name, RaceIndex, TrackId, TimeMs, Status, Rank, CarId, PlayerCount, Solo, FinalRank, TotalTimeMs`.

---

## 7. Networking and timing details

- **Host:** created by the first station that finds no open lobby. The host is also a player. `GameMode` exists only on the host.
- **Time source:** the game state's server world time on the host. `StartServerTime` is replicated once; clients derive the countdown from it.
- **Authoritative time:** `crossing time − StartServerTime`, integer ms, computed by the host. HUD time on clients is an estimate; Results always show host values.
- **Movement:** owner-authoritative; each sample is timestamped with the sender's estimate of host time. Remote cars interpolate. Because there is no vehicle collision (L-1), small latency differences never change gameplay outcomes.
- **Pickups, fuel and boost are local:** pickups are per-player actors with fixed IDs (not replicated) and the meters live on each client. The host only enforces the speed limit above, so cheating fuel or boost would show up as impossible speed.
- **Host advantage (D-7):** the host's own car has no network delay. Timestamped samples reduce the difference to clock-sync error (a few ms on wired LAN). Keep PCs equal and run supervised.
- **Seamless travel (Type A):** same session across race maps; `PlayerState` carries data; a short transition map is used by the engine.
- **Detach:** after the final summary is acknowledged, every station (including the host) leaves the session; the Results screen is rendered from local data.
- **Bandwidth:** negligible at 2–4 cars; the real costs are host CPU and map load time (measure in Phase 10).

---

## 8. Settings and mode toggle design

1. The mode is a **local setting per station**, edited in Settings behind the admin PIN (read-only for players).
2. A lobby **advertises its mode**; stations only see lobbies of their own mode and build. Searching and Lobby show the mode name ("3-Race Series" / "Quick Race").
3. The mode is **snapshotted into the lobby when it is created**; later changes on any station never affect a running match.
4. **Operational use:** with no central switch you change the mode station by station, or deploy a new `InnoRace.ini` with the build. Set **all stations identically** before opening (E-22). If the queue gets long, move everyone to Type B to triple turnaround.

---

## 9. Testing strategy

| Layer | What | How |
|---|---|---|
| Unit | Scoring: rank, ties, DNF penalty, all-DNF, solo, drop-outs | UE automation tests on the scoring library |
| Unit | Name validation | Automation tests |
| Unit | Local store: append, corrupt-tail recovery, backup | Automation tests |
| Unit | Hover logic: auto-clutch thresholds, fuel burn, limp mode, boost timing | Automation tests on the pure-logic components |
| Integration | Search → host/join → lobby → race → summary → local store | 2–4 real stations |
| Discovery | 4 stations press Find Match simultaneously; kill a host in the lobby; mismatched BuildId or mode | Real LAN |
| UI | Navigation graph (reachability, exits), focus, device swap | Automation + manual checklist |
| Network | Lag, loss, jitter | `Net PktLag`, `Net PktLoss`; router emulation |
| Soak | 4+ hours with real stations | Dress rehearsal |
| Chaos | Kill host/client in each state, unplug cables/controllers, change mode on one station | Phase 10 checklist |
| Playtest | Track length, car feel, par times, banner/timeout lengths | 10+ real players before the event |

---

## 10. Event-day plan

### 10.1 Capacity sizing (rough: one match per participant, 2 players per match, 8-hour event)

| Mode | Match length | Matches needed | Matches/hour | Station pairs needed | Stations |
|---|---|---|---|---|---|
| Type B | ~5 min | ~500 | ~63 | ~6 | ~12 |
| Type A | ~13 min | ~500 | ~63 | ~14 | ~28 |

There is no separate server PC: every match runs on the two participating stations. With fewer stations, default to Type B, use Type A for finals or quiet hours, and consider `C.MaxAttempts` = 1.

### 10.2 Pre-event checklist

- [ ] Same BuildId on every station; same **mode** on every station.
- [ ] Each station has a unique `C.StationId`.
- [ ] Wired network on one subnet; LAN discovery verified from every station (diagnostics screen shows peers).
- [ ] Firewall script run on every station.
- [ ] Admin PIN set; blocked-word list reviewed; local data cleared from rehearsals (after exporting).
- [ ] Controllers tested, spares ready; stations auto-launch to Main Menu.
- [ ] Plan for collecting results: who exports CSVs, how often, and where they are merged (D-10).
- [ ] Volunteers know the runbook below.

### 10.3 Runbook (quick fixes)

| Symptom | Action |
|---|---|
| "Searching…" never finds anyone | Check cable, subnet, same mode and BuildId, firewall; open diagnostics to see peers |
| A station is stuck in a lobby | Press Leave; if the screen is frozen, relaunch the game |
| A match is stuck or one PC froze | Relaunch the frozen station; the other will get Connection Lost and can search again |
| Long queue | Switch all stations to Type B (PIN → Settings) |
| A player wants to rename | Admin menu → rename (only before results are exported) |
| Disputed result | Admin menu → void the result on that station; the player can play again |
| End of hour / end of day | Export CSV on each station; merge on one PC; publish the combined ranking |

---

## 11. Risk register

| Risk | Impact | Mitigation |
|---|---|---|
| LAN discovery blocked by the network | Nobody can match | Phase 0 spike on the real network; diagnostics screen; direct-IP fallback |
| Host leaves → everyone's race ends | Frustration, lost time | No host migration by design; attempts not consumed; fast re-search; use stable PCs |
| Host advantage | Unfair results | Timestamped samples, equal PCs, supervised stations (D-7) |
| No central leaderboard | Per-station rankings only | CSV export and merge, or optional LAN result sharing (Phase 8, item 6); decide D-10 early |
| Hover physics feels bad or unstable | Core fun lost | Time-box Phase 1; prototype on a greybox before art |
| Station count too low for 1000+ players | Long queues | Type B default; schedule Type A for quiet periods |
| Host PC too weak for host + game | Stutter, bad times | Test the weakest PC as host; scalability presets |
| Gamepad on-screen keyboard is annoying | Slow throughput | Random callsign; physical keyboard accepted on the same screen |
| Cheating | Unfair leaderboard | Host sanity checks, supervised stations, admin void tools |
| Shader hitches on the first race | Bad first impression | Warm-up in the menu; test on a clean machine |
| Scope creep | Misses the deadline | Backlog only after the event milestone; freeze features at Phase 9 |

---

## 12. Backlog (after the event-critical milestone)

- Optional LAN result sharing / event-wide leaderboard (Phase 8, item 6), if not built earlier.
- Host migration or match restore after a host crash.
- Returning-participant PIN so names are unique and attempts are enforceable event-wide.
- A central results service, if you later add a server PC.
- Ghost opponents for the no-opponent fallback.
- Spectator / replay camera and a big-screen mode.
- Personal-best ghost, achievements, per-car boards, daily track rotation.

---

## Appendix A — Suggested C++ class list

`UInnoUIFlowSubsystem`, `UInnoActivatableWidget`, `UInnoSessionSubsystem` (search, host, join, merge rule), `UInnoLocalStore`, `UInnoAdminMenu`, `UInnoMatchConfig`, `UInnoTrackData`, `UHoverCarData`, `AHoverCarPawn`, `UHoverClutchComponent`, `UHoverFuelComponent`, `UHoverBoostComponent`, `UHoverAssistComponent`, `AHoverPickup`, `APickupSpawnPoint`, `ATrackRoot`, `AInnoRaceGameMode`, `AInnoRaceGameState`, `AInnoPlayerState`, `AInnoPlayerController`, `FInnoRaceResult`, `FInnoMatchSummary`, `FInnoScoring` (pure C++ scoring library).

## Appendix B — Screen inventory vs wireframes

| Wireframe | Screen | Notes |
|---|---|---|
| Main_Menu_Screen_1 | S-01 | Fix "Participent". No Quit button in kiosk mode. |
| New_Participant_s_Screen | S-02 | Add on-screen keyboard, inline errors, Random Callsign |
| Car_Selection_Screen | S-03 | Rename "Enter Lobby" → "Find Match"; show focused vs confirmed car |
| (new) | S-04 Searching | Shows the mode name; Cancel |
| Match_Lobby_-_VS_screen | S-05 | N-row list, ready indicators, track list, mode; host sees "Waiting for players…" and the solo prompt |
| Loading_Screen | S-06 | Add progress; also used between races |
| (new) | S-07 Countdown, S-08 HUD | HUD shows "Race n/3" and the running total in Type A, plus the clutch gauge (5 stepped bars, number, name plate), fuel bar, boost pips and speed |
| Winner_s_Screen_1 / Loser_s_Screen_1 / dnf_Screen / Connection_Issue_Screen | S-09 (variants), S-15 | One data-driven widget with variants: Win, Placed Nth, DNF (with reason), Opponent disconnected, Host left |
| (new) | S-10 Intermission | Race result, running total, next track, timer |
| Game_Finshed_Screen_1 + Waiting_Screen | S-11 | Fix "Finshed". Waiting is the sub-state while others finish. Runs offline from the stored summary. |
| (new) | S-12 Leaderboard, S-13 Settings, S-14 Credits | Leaderboard is local; Settings: mode (PIN-locked), audio, video, controls |
| (new) | S-16 Modals | Confirm Leave, Admin PIN, Error, Controller Disconnected, Idle Warning, Solo prompt |
