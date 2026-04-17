# Horse Racing Todo

## MVP

The MVP is the smallest version that delivers the complete original game loop:

- [ ] Boot to a title screen and start a new 10-race program.
- [ ] Implement player setup for 1 to 6 bettors.
- [ ] Implement setup for 0, 1, or 2 human jockeys.
- [x] Detect and read SNES controller input needed for menus and racing.
- [ ] Define compact data structures for bettors, horses, race entries, and race results.
- [ ] Implement generation of 8 persistent horses at the start of a new session.
- [ ] Implement persistent horse traits: break speed, early pace, endurance, stretch speed, and 3 surface affinities.
- [ ] Implement generation of a 10-race program.
- [ ] Generate 4 entrants per race from the 8-horse stable.
- [ ] Generate race surface per race: dry, muddy, or turf.
- [ ] Generate race length per race using the original grouped structure.
- [ ] Implement bettor bankrolls with a $750 starting value.
- [ ] Enforce the $250 maximum wager or current bankroll, whichever is lower.
- [ ] Mark busted bettors as out of the game.
- [x] Implement the betting sheet screen.
- [x] Show race number, entrants, odds, distance, surface, and bettor bankrolls on the betting sheet.
- [ ] Implement turn-based pass-and-play betting entry.
- [ ] Implement Win bets.
- [ ] Implement Exacta bets.
- [ ] Lock in wagers and deduct bankroll immediately on confirmation.
- [ ] Implement tentative odds display for the current race.
- [ ] Adjust odds as wagers are placed.
- [ ] Implement the past performance screen.
- [ ] Store and display up to the last 3 runs per horse.
- [ ] Show enough past data to compare distance, surface, time, and finish summary.
- [ ] Support switching between betting and past performance views during betting.
- [ ] Implement jockey assignment before each race.
- [ ] Support all-CPU races.
- [ ] Support one human-controlled outside horse.
- [ ] Support two human-controlled outside horses when two controllers are available.
- [ ] Implement the starting gate sequence.
- [ ] Implement a side-view race screen with four horses visible.
- [ ] Implement horizontal race progression with independent foreground and background scrolling.
- [ ] Implement four race lanes with inside-lane advantage.
- [ ] Implement lane changing for human jockeys.
- [ ] Implement CPU lane behavior that prefers the rail and moves out to pass.
- [ ] Implement horse pace simulation using only integer arithmetic.
- [ ] Apply distance and surface effects to horse performance.
- [ ] Apply fatigue over race duration.
- [ ] Implement Coax input with a modest temporary speed boost.
- [ ] Implement Whip input with a strong temporary boost and heavy endurance cost.
- [ ] Implement late-race CPU whip behavior.
- [ ] Add visible furlong markers.
- [ ] Trigger a cue as the leader approaches each furlong marker.
- [ ] Detect finish order for all 4 horses.
- [ ] Display winning time.
- [ ] Display tote-board-style race results.
- [ ] Settle Win and Exacta payouts.
- [ ] Update bettor bankrolls after each race.
- [ ] Advance automatically from race results to the next betting phase.
- [ ] End the session after race 10.
- [ ] Display final standings and overall winner.
- [x] Build a minimal tile set that fits within the 256-tile budget.
- [ ] Build minimal horse and jockey art that is readable by color.
- [x] Build minimal dry, muddy, and turf visual variants.
- [ ] Implement core audio cues: UI click, error buzz, call to post, gate clang, hoofbeats, furlong gong, whip crack, and crowd cheer.
- [ ] Verify the full 10-race loop runs end to end without resetting or corrupting state.
- [ ] Verify all MVP features work with one controller.
- [ ] Verify all MVP features work with two controllers where applicable.
- [ ] Verify performance is acceptable on the 10 MHz Z80 target.

## MVP By Track

This section reorganizes the MVP into parallel tracks for implementation planning.

### Engineering

Build these in order. Later steps should not start until their prerequisites are stable.

#### Step 1: Core Runtime Foundation

- [ ] Define compact runtime structures for bettors, horses, races, wagers, past performances, and results.
- [x] Implement SNES controller input handling for menus and races.
- [ ] Build the boot flow from title screen to new 10-race program.
- [ ] Implement player setup for 1 to 6 bettors.
- [ ] Implement jockey setup for 0, 1, or 2 human riders.

#### Step 2: Session Initialization

- [ ] Generate 8 persistent horses at session start.
- [ ] Generate persistent horse traits for pace, endurance, break speed, stretch speed, and surface affinity.
- [ ] Generate a 10-race program with entrants, distance, and surface.
- [ ] Implement bankroll tracking with $750 starting stake and $250 max wager cap.
- [ ] Implement elimination of busted bettors.

#### Step 3: Betting Phase

- [ ] Implement the betting sheet logic.
- [ ] Implement turn-based pass-and-play wager entry.
- [ ] Implement tentative odds generation and update odds as bets are placed.
- [ ] Implement Win bet entry and validation.
- [ ] Implement Exacta bet entry and validation.
- [ ] Deduct bankroll immediately when a wager is confirmed.

#### Step 4: Race History And Handicapping Data

- [ ] Implement past-performance data capture for every completed race.
- [ ] Limit displayed past performances to the last 3 runs per horse.
- [ ] Implement the past performance screen and navigation back to betting.

#### Step 5: Jockey Assignment And Race Setup

- [ ] Implement jockey assignment before each race.
- [ ] Support all-CPU races.
- [ ] Support one human-controlled outside horse.
- [ ] Support two human-controlled outside horses when two controllers are present.
- [ ] Implement the starting gate sequence.

#### Step 6: Core Race Simulation

- [ ] Implement the side-view race simulation.
- [ ] Implement four race lanes and inside-lane path advantage.
- [ ] Implement integer-only pace simulation.
- [ ] Apply distance, surface, and fatigue effects during the race.
- [ ] Implement lane changing for human jockeys.
- [ ] Implement CPU lane choice with rail preference and passing behavior.
- [ ] Implement Coax behavior.
- [ ] Implement Whip behavior.
- [ ] Implement late-race CPU whip usage.
- [ ] Implement furlong marker state and leader-based timing.
- [ ] Detect finish order and winning time.

#### Step 7: Race Presentation Integration

- [ ] Implement independent foreground and background scrolling during races.

#### Step 8: Settlement And Session Progression

- [ ] Implement results flow.
- [ ] Implement Win bet settlement.
- [ ] Implement Exacta bet settlement.
- [ ] Update bankrolls after each race.
- [ ] Advance automatically to the next race.
- [ ] Implement end-of-session standings after race 10.

### Art

- [x] Define an MVP tile budget that stays within the 256 shared 16x16 tile limit.
- [x] Produce a minimal title screen tile set.
- [x] Produce MVP betting-sheet UI tiles.
- [x] Produce MVP past-performance UI tiles.
- [ ] Produce a readable font, numerals, and betting symbols.
- [ ] Produce horse art that reads clearly by color.
- [ ] Produce minimal jockey art.
- [ ] Produce a minimal starting gate visual.
- [x] Produce race track tiles for rails, lane reads, finish line, and furlong markers.
- [x] Produce background scenery tiles for parallax racing scenes.
- [x] Produce dry, muddy, and turf visual variants using palettes and minimal tile changes.
- [ ] Produce tote-board and final-standings presentation tiles.
- [ ] Verify all MVP art assets fit the tile and sprite budget.

### Audio

- [ ] Implement a valid UI click cue.
- [ ] Implement an invalid-action buzz cue.
- [ ] Implement the call-to-post cue.
- [ ] Implement the starting gate clang.
- [ ] Implement hoofbeat race audio.
- [ ] Implement furlong gong timing.
- [ ] Implement whip crack audio.
- [ ] Implement crowd cheer at the finish.
- [ ] Hook all core cues into the correct gameplay events.
- [ ] Verify audio timing is clear enough to support jockey play.

### Testing

- [ ] Verify a full 10-race session runs from title to final standings without corrupting state.
- [ ] Verify all MVP features work with one controller.
- [ ] Verify two-controller jockey support works when available.
- [ ] Verify bettors cannot wager more than bankroll or more than $250.
- [ ] Verify busted bettors are removed cleanly from later betting rounds.
- [ ] Verify only the 4 current entrants can be selected for Win and Exacta bets.
- [ ] Verify Exacta order matters and pays correctly at 15:1.
- [ ] Verify odds update correctly as bets are placed.
- [ ] Verify horse traits remain persistent across the entire 10-race session.
- [ ] Verify only the last 3 race records are shown per horse.
- [ ] Verify dry, muddy, and turf conditions all affect race outcomes.
- [ ] Verify distance groups affect pace and fatigue.
- [ ] Verify lane changes, rail preference, Coax, and Whip all affect race outcomes.
- [ ] Verify finish order and winning time are correct.
- [ ] Verify payouts and bankroll updates are correct after every race.
- [ ] Verify automatic progression from results to the next race works across all 10 races.
- [ ] Verify performance is acceptable on the 10 MHz Z80 target.

## MVP Milestones

This section groups the MVP into delivery checkpoints with clear completion gates.

### Milestone 1: Core Framework

Goal:
Create the base program flow, input layer, and runtime data model.

Tasks:

- [ ] Build boot flow from title screen to a new session.
- [ ] Implement player setup for 1 to 6 bettors.
- [ ] Implement jockey setup for 0, 1, or 2 human riders.
- [x] Implement SNES controller input handling for menus and races.
- [ ] Define compact data structures for bettors, horses, race programs, wagers, and results.
- [ ] Implement session reset and new-game initialization.

Completion gate:

- [ ] A new session can be started from the title screen.
- [ ] The game can capture valid controller input and move cleanly through setup.
- [ ] Core runtime structures initialize without corrupt state.

### Milestone 2: Race Program And Betting Loop

Goal:
Make one race playable from the betting side before live race presentation is finished.

Tasks:

- [ ] Generate 8 persistent horses at session start.
- [ ] Generate persistent horse traits for pace, endurance, break speed, stretch speed, and surface affinity.
- [ ] Generate a 10-race program with entrants, surfaces, and distance groups.
- [ ] Implement bankroll tracking with $750 starting stake.
- [ ] Enforce the $250 max wager cap.
- [ ] Mark busted bettors as out of the game.
- [ ] Implement the betting sheet logic.
- [x] Show race number, entrants, odds, distance, surface, and bettor bankrolls.
- [ ] Implement pass-and-play wager entry.
- [ ] Implement Win bets.
- [ ] Implement Exacta bets.
- [ ] Deduct bankroll immediately when a wager is confirmed.
- [ ] Implement tentative odds generation and update logic.

Completion gate:

- [ ] The first race can be configured and wagered entirely through the betting UI.
- [ ] Bankroll math is correct for valid and invalid wagers.
- [ ] Win and Exacta bets can be entered and stored correctly.

### Milestone 3: Past Performances And Results Data

Goal:
Support the original handicapping loop by capturing and displaying prior races.

Tasks:

- [ ] Implement race result records per horse.
- [ ] Store up to the last 3 runs per horse.
- [ ] Implement the past performance screen.
- [ ] Show distance, surface, time, and finish summary for prior runs.
- [ ] Support switching between betting and past performance views.

Completion gate:

- [ ] After completed races, each horse carries forward usable past-performance data.
- [ ] The betting phase can display only the last 3 runs per horse.
- [ ] Players can compare entrants before wagering without leaving the betting flow.

### Milestone 4: Live Race Prototype

Goal:
Get the race itself running with minimal presentation but correct gameplay behavior.

Tasks:

- [ ] Implement the starting gate sequence.
- [ ] Implement side-view race simulation for 4 horses.
- [ ] Implement foreground and background scrolling.
- [ ] Implement four race lanes with inside-lane advantage.
- [ ] Implement lane changing for human jockeys.
- [ ] Implement CPU lane behavior with rail preference and passing.
- [ ] Implement integer-only pace simulation.
- [ ] Apply distance, surface, and fatigue effects.
- [ ] Implement Coax behavior.
- [ ] Implement Whip behavior.
- [ ] Implement late-race CPU whip usage.
- [ ] Implement furlong markers and leader-based timing.
- [ ] Detect finish order and winning time.

Completion gate:

- [ ] A race can be started from the betting phase and run to completion.
- [ ] Human jockey controls affect at least the outside horses correctly.
- [ ] CPU horses show distinct behavior based on traits, surface, and distance.
- [ ] Finish order and winning time are recorded correctly.

### Milestone 5: Session Loop Completion

Goal:
Connect betting, racing, results, and progression into a full 10-race game.

Tasks:

- [ ] Implement jockey assignment before each race.
- [ ] Support all-CPU races.
- [ ] Support one human-controlled outside horse.
- [ ] Support two human-controlled outside horses when two controllers are present.
- [ ] Display tote-board-style race results.
- [ ] Settle Win and Exacta payouts.
- [ ] Update bettor bankrolls after each race.
- [ ] Advance automatically from results to the next betting phase.
- [ ] End the session after race 10.
- [ ] Display final standings and overall winner.

Completion gate:

- [ ] The game runs from title screen through all 10 races without manual resets.
- [ ] Bets settle correctly after every race.
- [ ] Final bankroll standings identify a winner after race 10.

### Milestone 6: MVP Presentation Package

Goal:
Add the minimum art and audio needed for a coherent playable release.

Tasks:

- [x] Define an MVP tile budget that fits the 256 shared 16x16 tile limit.
- [ ] Produce minimal title, betting-sheet, past-performance, race, tote-board, and standings visuals.
- [ ] Produce readable font, numerals, and betting symbols.
- [ ] Produce minimal horse and jockey art readable by color.
- [ ] Produce dry, muddy, and turf visual variants.
- [ ] Implement core audio cues: UI click, error buzz, call to post, gate clang, hoofbeats, furlong gong, whip crack, and crowd cheer.
- [ ] Hook audio to the correct gameplay events.

Completion gate:

- [ ] All MVP screens are readable and navigable.
- [ ] Horses, surfaces, and race state are visually distinguishable.
- [ ] Required audio cues play at the correct times.
- [ ] Art and audio remain within platform limits.

### Milestone 7: MVP Verification

Goal:
Confirm the game is complete, stable, and fast enough to count as MVP.

Tasks:

- [ ] Verify a full 10-race session runs from title to final standings without corrupting state.
- [ ] Verify all MVP features work with one controller.
- [ ] Verify two-controller jockey support works when available.
- [ ] Verify bettors cannot wager more than bankroll or more than $250.
- [ ] Verify only current entrants can be selected for Win and Exacta bets.
- [ ] Verify Exacta order and payout behavior.
- [ ] Verify horse traits remain persistent across the session.
- [ ] Verify only the last 3 race records are shown per horse.
- [ ] Verify dry, muddy, and turf conditions affect race outcomes.
- [ ] Verify distance groups affect pace and fatigue.
- [ ] Verify lane changes, rail preference, Coax, and Whip affect race outcomes.
- [ ] Verify payouts and bankroll updates are correct after every race.
- [ ] Verify performance is acceptable on the 10 MHz Z80 target.

Completion gate:

- [ ] The full 10-race game loop is complete and playable.
- [ ] Major rules from the specification are implemented correctly.
- [ ] No known blocker prevents calling the game MVP-complete.

## Phase 2: Production-Ready Presentation

This phase improves readability, consistency, and presentation without changing the MVP rules.

- [ ] Refine title screen layout and visual identity.
- [ ] Refine player setup screen with clearer controller prompts.
- [ ] Improve betting sheet readability and navigation flow.
- [ ] Improve past performance formatting for faster comparison.
- [ ] Add clearer surface icons or color-coding.
- [ ] Add clearer distance indicators on race info screens.
- [ ] Improve tote board presentation and final standings screen.
- [ ] Improve race HUD readability.
- [ ] Add a stronger starting gate presentation.
- [ ] Add distinct palette treatment for dry, muddy, and turf races.
- [ ] Add better parallax composition between background and foreground layers.
- [ ] Improve horse run animation within the same tile budget.
- [ ] Improve finish-line and furlong-marker visuals.
- [ ] Add title fanfare.
- [ ] Improve race audio mixing and timing priorities.
- [ ] Add better feedback for invalid betting actions.
- [ ] Tune camera tracking so all 4 horses remain readable more often.

## Phase 3: Tuning And Balance

This phase is for making the game feel fair, learnable, and replayable.

- [ ] Tune horse trait generation so horses feel distinct but not wildly random.
- [ ] Tune race program generation so surfaces and lengths feel varied across 10 races.
- [ ] Tune the odds model so favorites and long shots emerge believably.
- [ ] Tune Win payouts against observed race outcomes.
- [ ] Verify Exacta at 15:1 remains viable but not dominant.
- [ ] Tune fatigue curves for each distance group.
- [ ] Tune muddy-track slowdown to be significant but not frustrating.
- [ ] Tune CPU rail-seeking and passing behavior.
- [ ] Tune CPU whip timing.
- [ ] Tune Coax benefit versus long-term stamina cost.
- [ ] Tune Whip benefit versus collapse risk.
- [ ] Tune lane advantage so rail positioning matters without making other lanes useless.
- [ ] Run repeated simulated sessions to check bankroll swing and session pacing.
- [ ] Check that later races reward handicapping better than early races.
- [ ] Verify past performance data is actually sufficient to support informed picks.

## Phase 4: Robustness And Edge Cases

This phase hardens the implementation and removes avoidable failure modes.

- [ ] Handle controller disconnect or missing second controller gracefully.
- [ ] Handle bettors choosing to skip a race cleanly.
- [ ] Handle all bettors being broke before race 10 cleanly.
- [ ] Handle ties or near-tie finish logic deterministically.
- [ ] Verify no invalid horse selections can be locked in for a bet.
- [ ] Verify Exacta selection cannot use the same horse twice unless explicitly intended by rules.
- [ ] Verify bankroll math cannot underflow or overflow.
- [ ] Verify race history storage rolls correctly after 3 stored runs.
- [ ] Verify session reset fully regenerates horses and clears all prior state.
- [ ] Verify sprite usage stays within hardware limits in all race scenes.
- [ ] Verify tile usage stays within the 256-tile cap.
- [ ] Stress test rapid controller input on betting screens and during races.

## Phase 5: Stretch Improvements

These are optional additions that stay within the spirit of the original game but are not required for MVP.

- [ ] Add optional horse names alongside color identities.
- [ ] Add richer title and attract-mode presentation.
- [ ] Add more detailed grandstand and infield scenery using the same tile budget.
- [ ] Add more expressive jockey animation if performance allows.
- [ ] Add a more polished tote board layout with horse-head or color icons.
- [ ] Add a race recap summary between races.
- [ ] Add optional hidden debug overlays for horse state and pace tuning.
- [ ] Add deterministic seed support for repeatable test sessions.
- [ ] Add automated simulation mode for balance testing.

## Do Not Add

- [ ] Do not add Place or Show bets.
- [ ] Do not add more than 4 horses per race.
- [ ] Do not add more than 8 horses in the stable.
- [ ] Do not add career, breeding, or management systems.
- [ ] Do not add mechanics that depend on a keyboard instead of the SNES controller.
