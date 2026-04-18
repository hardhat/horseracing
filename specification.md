# Horse Racing for Zeal 8-bit Computer

## Overview

This game is a Zeal 8-bit adaptation of the Intellivision Horse Racing concept.
The design keeps the original structure and core mechanics:

- A full session is a 10-race program.
- Up to 6 bettors participate in the same session.
- Each race contains 4 horses drawn from a stable of 8 horses.
- Every bettor starts with a simulated bankroll of $750.
- Bets are limited to Win and Exacta only.
- Two outside horses may be jockey-controlled while the inside horses remain CPU-controlled.
- Each horse has persistent hidden traits across the entire 10-race program.
- Races vary by surface and distance.

The Zeal version must present the same overall experience as a party betting game plus a short side-view horse race, but be built specifically for a 10 MHz Z80 and the Zeal video system:

- 320x240, 256-color mode.
- 256 total 16x16 tiles shared across foreground, background, and sprite art.
- 80x40 tile foreground map.
- 80x40 tile background map.
- Independent foreground and background scrolling.
- Up to 128 hardware sprites.
- SNES controller interface used for all game controls.

No additional betting types, horse management systems, career progression, commentary systems, or simulation layers should be added.

## Design Goals

- Recreate the original loop of study, bet, race, settle, and repeat for 10 races.
- Preserve the original emphasis on handicapping from limited past-performance data.
- Preserve the original tactical jockey play of timing lane position, coaxing, and whipping.
- Use Zeal scrolling layers to make the race presentation stronger than the original without changing the rules.
- Keep the implementation simple enough to run reliably on a 10 MHz Z80.

## Game Structure

### Session Length

- One game session consists of exactly 10 races.
- The winner is the bettor with the highest bankroll after race 10.
- Any bettor whose bankroll reaches $0 is out of the game and may not place further bets.

### Players

- Support 1 to 6 human bettors using pass-and-play betting entry.
- Support 0, 1, or 2 human jockeys during each race.
- Because there is only one controller, at most one outside horse can be human-controlled during a race.
- If no jockey chooses to ride, all 4 horses are CPU-controlled.

### Stable

- The game creates a stable of 8 horses at the start of a new 10-race program.
- Each horse is identified primarily by color, matching the source material's presentation.
- Horse names are optional flavor text; color identity is mandatory.
- Hidden horse traits persist for all 10 races.

## Horse Model

Each of the 8 horses must have persistent internal attributes generated at the start of a new session.
These attributes are not shown numerically to the player, but must be inferable from past performances.

Required attributes:

- Break speed: performance leaving the gate.
- Early pace: speed during the first portion of the race.
- Endurance: how quickly speed falls off over distance.
- Stretch speed: finishing performance late in the race.
- Surface affinity for dry tracks.
- Surface affinity for muddy tracks.
- Surface affinity for turf tracks.

Behavior requirements:

- Horses should generally run fastest early, then slow as the race progresses.
- Muddy tracks must magnify fatigue more than dry or turf tracks.
- A horse's apparent style may differ by race condition because distance and surface interact with hidden traits.
- The same horse must remain internally consistent across the full 10-race program.

## Race Program Generation

For each of the 10 races, generate:

- 4 entrants selected from the 8-horse stable.
- 1 race surface: dry, muddy, or turf.
- 1 race length chosen from the original distance groups.

Allowed race lengths:

- 3 or 4 furlongs.
- 5 or 6 furlongs.
- 7 or 8 furlongs.
- 9 or 10 furlongs.

Representation rules:

- The UI may show the exact selected length in furlongs.
- Past-performance summaries must preserve the original grouping logic so prior races can be compared by short, medium, or long distance bands.

## Betting Rules

### Bankroll

- Every bettor begins with $750.
- Maximum bet per wager is the lesser of $250 or the bettor's current bankroll.
- A confirmed wager immediately deducts the amount from that bettor's bankroll.

### Bet Types

Only two bet types are allowed:

- Win: pick the horse that finishes first.
- Exacta: pick the first-place and second-place horses in exact finishing order.

Not allowed:

- Place bets.
- Show bets.
- Quinella, trifecta, daily double, or any other added wager types.

### Odds and Payouts

- Each race starts with tentative odds for the 4 entrants.
- Odds adjust as players place wagers.
- Exacta pays a fixed 15:1, matching the original rules.
- Win bets pay according to the current displayed odds at the time the wager is locked in.

### Betting Flow

For each race:

1. Show the betting sheet.
2. Allow players to inspect past performances.
3. Each bettor chooses whether to bet.
4. If betting, the bettor selects:
	- player identity
	- wager amount
	- wager type
	- horse selection or exacta combination
5. After all bettors are done, choose human jockey participation.
6. Start the race.

## Past Performance Screen

The past performance display is central to the game and must be readable and compact.

For each of the 4 entrants in the upcoming race, show:

- Color identity.
- Current odds.
- Up to the last 3 previous races for that horse.

Each previous race entry must include:

- Distance group or exact furlong count.
- Surface.
- Finish time.
- Finishing order of the top horses in that earlier race.

Behavior rules:

- Race 1 has no prior performance data.
- Only the last 3 races are shown per horse, matching the original limitation.
- This screen must allow players to compare entrants before betting.

## Race Controls

All controls use the SNES controller interface.

### Betting Screen Controls

These mappings are implementation requirements for the Zeal version:

- D-pad Up/Down: move between fields or bettors.
- D-pad Left/Right: change selected value in the active field.
- A: confirm current field or confirm wager.
- B: cancel current edit or back out of selection.
- X: toggle wager type between Win and Exacta.
- Y: open or close past performance display.
- L/R: cycle between entrants or pages when the past performance display is open.
- Start: finish betting phase and move to jockey assignment or race start.
- Select: skip betting for the current bettor.

### Race Controls For Human Jockeys

Each human jockey controls one outside horse.

- D-pad Up: move nearer the rail.
- D-pad Down: move away from the rail.
- Y: Coax.
- B: Whip.

Optional duplicate mappings for comfort are allowed if they do not change mechanics.

Control behavior requirements:

- Coax gives a modest temporary speed increase.
- Whip gives a much larger burst at a heavy endurance cost.
- Excessive early use of either control must reduce late-race performance.
- Whip should be most effective only in the final part of the race.

## Race Logic

### Camera and Track Model

- Races are shown from a side view with horizontal scrolling.
- The race must visually communicate four lanes, but the inside path should still be tactically advantageous.
- The track should behave as slightly curved in gameplay terms, preserving the original idea that a horse nearer the rail covers less distance.

Implementation model:

- Simulate each horse on a forward progress axis plus a lane position axis.
- Lane changes alter effective path length.
- CPU horses prefer the rail and only move out to pass.
- Human jockeys can choose lane position at any time.

### Pace Model

Each horse's current speed is derived from:

- Base trait values.
- Race distance remaining.
- Surface modifier.
- Current fatigue.
- Lane/path effect.
- Coax or whip input if human-controlled.
- CPU tactical spurts if CPU-controlled.

Required race feel:

- Fast opening burst.
- Meaningful separation by the second or third furlong.
- Visible late-race fatigue, especially on muddy tracks.
- Possibility of late come-from-behind wins with correct whip timing.

### Furlong Markers

- Mark every furlong with a visible course marker.
- Trigger an audio cue as the leader approaches each marker.
- Human players should be able to time whip usage from these markers.

### Finish and Results

- When the winner crosses the line, trigger crowd cheer audio and show the winning time.
- Continue the race until all 4 horses finish.
- Display final order from first to fourth.
- Then return automatically to the betting sheet for the next race.

## Video Specification

### General Layout

The game uses three main presentation modes:

- Title screen.
- Betting and past-performance screens.
- Live race screen.

### Tile and Sprite Budget

There are 256 total 16x16 tiles shared across all uses.
Art direction and screen design must stay within this fixed set.

Recommended budget:

- 64-80 tiles for race track and scenery.
- 40-56 tiles for betting UI panels, frames, and icons.
- 48-64 tiles for font, numerals, and symbols.
- 48-72 tiles for horse and jockey animation frames.
- Remaining tiles reserved for tote board, title art, and special markers.

This budget is guidance, not a hard per-category rule, but the total must not exceed 256 unique tiles in memory at once for the chosen asset set.

### Prototype Sheet Layout

The current prototype asset pipeline should be organized into four sheets:

- `title.gif`: title and menu presentation tiles.
- `map.gif`: track, scenery, rail, and marker tiles.
- `hud.gif`: betting, results, and status UI tiles.
- `sprites.gif`: horses, jockeys, gate parts, and cursor/highlight sprites.

These sheets are planning artifacts for the first horse-racing-specific art pass. They do not need final art polish, but they should answer tile-budget and layout questions early.

#### Title Sheet

Target size:

- 17 x 2 tiles.
- 34 tiles total.

Planned use:

- 10 to 14 tiles for the title logo.
- 4 to 6 tiles for menu frames and separators.
- 4 to 6 tiles for prompts such as `PRESS START` and setup headings.
- 4 tiles reserved for selection arrows, cursors, or menu embellishments.
- Remaining tiles reserved for alternates or cleanup.

Prototype goal:

- Show a readable game title.
- Support the title screen and player setup screen without borrowing from race UI tiles.

#### Track Sheet

Target size:

- 16 x 4 tiles.
- 64 tiles total.

Planned use:

- 8 to 12 tiles for dry track variants.
- 8 to 12 tiles for muddy track variants.
- 8 to 12 tiles for turf track variants.
- 6 to 8 tiles for rails, lane boundaries, and edge transitions.
- 4 to 6 tiles for furlong posts and finish line structures.
- 8 to 12 tiles for distant grandstand, sky, infield, or crowd blocks.
- Remaining tiles for utility fillers and transitions.

Prototype goal:

- Prove that the race can read clearly with distinct dry, muddy, and turf conditions.
- Prove that foreground and background layers can share a coherent visual language.

#### HUD Sheet

Target size:

- 8 x 3 tiles.
- 24 tiles total.

Planned use:

- 8 to 10 tiles for panel borders, corners, and dividers.
- 4 to 6 tiles for cursor and selection highlights.
- 4 to 6 tiles for betting markers such as Win, Exacta, odds separators, and bankroll symbols.
- 4 to 6 tiles for compact results and race-state widgets.

Font and numerals should continue to be rendered through the text tile path rather than consuming this sheet unless proven necessary.

Prototype goal:

- Support betting-sheet framing, past-performance framing, compact race HUD, and tote-board/result accents.

#### Sprite Sheet

Target size:

- 8 x 2 tiles.
- 16 tiles total in the current prototype allocation.

Planned use for the first pass:

- 8 tiles for one two-frame horse body set.
- 4 tiles for a second horse color/style or jockey variation.
- 2 to 4 tiles for gate doors, a highlight cursor, or a marker sprite.

Prototype goal:

- Validate whether horses should be built as combined horse-plus-jockey sprites or split layers.
- Validate whether the current 16-tile prototype allocation is enough for an early race prototype.

Expected follow-up:

- The sprite budget will likely need to grow beyond this prototype allocation once horse animation and color readability are tested.
- If that happens, reduce decorative background usage before cutting HUD readability.

### Background Layer

Use the 80x40 background tilemap for distant scenery.

Suggested uses:

- Sky gradient or distant infield backdrop.
- Grandstand silhouette.
- Trees, rails, infield decorations.
- Surface-dependent color treatment.

Behavior:

- Scroll horizontally during races.
- Move more slowly than the foreground to create parallax.
- Use different palettes or tile arrangements for dry, muddy, and turf conditions.

### Foreground Layer

Use the 80x40 foreground tilemap for trackside and HUD-adjacent visual elements.

Suggested uses:

- Rail graphics.
- Track edge details.
- Furlong markers.
- Starting gate elements that do not need to animate as sprites.
- Finish line structures.

Behavior:

- Scroll independently of the background.
- Move at race speed.
- Allow overlay-style composition for HUD framing when needed.

### Sprites

Use hardware sprites for moving race entities and screen highlights.

Primary race sprite use:

- 4 horses.
- 4 jockeys if baked separately from horses.
- Starting gate doors if animated with sprites.
- Tote board highlights or cursor markers.

The 128-sprite limit is generous relative to game needs; the design should prioritize stable frame rate and simple compositing over using sprite capacity aggressively.

### Horse Animation

Each horse should be readable first by color.

Minimum animation states:

- Running loop.
- Coax/effort variant optional.
- Whip burst variant optional.
- Finish or slow-down variant optional.

To conserve tile budget:

- Use shared body frames where possible.
- Distinguish horses primarily through palette swaps or color regions.
- Keep jockey differences minimal.

## Screen Specifications

### Title Screen

Must show:

- Game title.
- Prompt to start a new 10-race program.
- Controller help for proceeding.

Optional:

- Brief music cue inspired by race-day fanfare.

### Player Setup Screen

Must allow:

- Selecting the number of bettors from 1 to 6.
- Selecting whether 0, 1, or 2 jockeys will be human-controlled when races start.
- Optional controller detection display.

### Betting Sheet Screen

Must display:

- Race number out of 10.
- Bettor bankrolls.
- Current entrants.
- Current odds.
- Current race surface.
- Current race distance.
- Active wager entry row.

The screen should support fast turn-based entry using a controller without needing a keyboard.

### Past Performance Screen

Must display:

- The 4 horses in the current race.
- Up to 3 previous runs for each horse.
- Color-coded or icon-coded surface information.
- Times and finish summaries.

It should be possible to pop into this screen repeatedly during betting, matching the original flow.

### Starting Gate Screen

Must show:

- Four loaded horses at the gate.
- A short pre-race pause.
- An opening-gate transition into live action.

### Race Screen

Must show:

- Four active horses.
- Furlong markers.
- A finish line structure.
- A compact HUD with race distance and surface.

The race camera should track the pack so all 4 horses remain visible as much as possible.

### Results / Tote Board Screen

Must show:

- Winning time.
- Finish order from first through fourth.
- Bankroll updates for all bettors.

After a short pause, return to the next race's betting sheet automatically.

### Game Over Screen

After race 10, show:

- Final bankroll standings for all bettors.
- Winning bettor.
- Prompt to start a new 10-race program.

## Audio Specification

The audio set should reproduce the original event structure rather than add elaborate new systems.

Required cues:

- Title fanfare.
- UI click for valid selection.
- Error buzz for invalid selection.
- Call-to-post cue before the race.
- Starting gate clang.
- Hoofbeat loop during racing.
- Furlong gong.
- Whip crack.
- Crowd cheer at the finish.

Audio priorities:

- Race state cues are more important than music.
- Timing cues must be clear enough to support jockey play.

## AI Specification

### CPU Betting/Odds Role

The system must provide tentative pre-race odds and dynamically adjust them as wagers are placed.
The exact odds model can be simplified, but it must create the same play pattern:

- Early races are only partly predictable.
- Later races reward attention to patterns.
- Favorites and long shots can emerge through player behavior.

### CPU Jockey Behavior

CPU-controlled horses must:

- Seek the rail when practical.
- Move out to pass slower horses.
- Save major whip bursts for late-race use.
- Show race-to-race consistency that reflects each horse's hidden strengths.

## Technical Constraints

### Performance Targets

- Maintain smooth enough scrolling and sprite motion for jockey timing to feel reliable.
- Prioritize deterministic update logic over expensive rendering effects.
- Favor integer arithmetic throughout the race simulation.

### Memory and Data Strategy

- Store horse traits in compact fixed-size structures.
- Store only the last 3 performance lines per horse.
- Reuse tiles heavily between betting UI and race presentation.
- Use palette differences more than unique art where possible.

### Simulation Cadence

- Race logic should run at a fixed update step.
- Rendering can be decoupled only if the platform support already exists; otherwise keep one simple fixed loop.

## Non-Goals

The following are explicitly out of scope because they are not part of the original game concept:

- Career mode.
- Stable ownership.
- Horse breeding or training.
- Commentary text crawls during the race.
- More than 4 horses per race.
- More than 8 horses in the stable.
- Additional wager types.
- RPG progression or unlock systems.
- Split-screen or alternate camera modes.

## Acceptance Criteria

The specification is satisfied when the game supports all of the following:

- A full 10-race session for 1 to 6 bettors.
- $750 starting bankroll per bettor.
- Win and Exacta betting only.
- 4 entrants drawn from a stable of 8 persistent horses.
- Dry, muddy, and turf surfaces.
- Race lengths covering 3 to 10 furlongs using the original grouped structure.
- Up to 2 human jockeys on the outside horses.
- SNES-controller-based betting and race control.
- Independent background and foreground scrolling during races.
- A side-view race with visible furlong markers, finish order, and winning time.
- Past-performance displays that expose only the last 3 races per horse.
- Final winner determined by bankroll after race 10.
