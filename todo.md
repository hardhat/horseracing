# Horse Racing Todo

## Immediate Next Tasks

These are the next implementation targets with the best gameplay payoff and the fewest prerequisite gaps.

- [x] Implement integer-only pace simulation using horse traits, race distance, surface, and fatigue.
- [x] Make Coax and Whip materially affect race outcomes for the human-controlled outside horse.
- [ ] Surface busted bettors as out of the game in the betting, results, and standings UI.

## MVP

The MVP is the smallest version that delivers the complete original game loop:

- [x] Boot to a title screen and start a new 10-race program.
- [x] Implement player setup for 1 to 6 bettors.
- [ ] Implement setup for 0 or 1 human jockeys in the one-controller build.
- [x] Detect and read SNES controller input needed for menus and racing.
- [x] Define compact data structures for bettors, horses, race entries, and race results.
- [x] Implement generation of 8 persistent horses at the start of a new session.
- [x] Implement persistent horse traits: break speed, early pace, endurance, stretch speed, and 3 surface affinities.
- [x] Implement generation of a 10-race program.
- [x] Generate 4 entrants per race from the 8-horse stable.
- [x] Generate race surface per race: dry, muddy, or turf.
- [x] Generate race length per race using the original grouped structure.
- [x] Implement bettor bankrolls with a $750 starting value.
- [x] Enforce the $250 maximum wager or current bankroll, whichever is lower.
- [x] Prevent busted bettors from placing further bets.
- [ ] Show busted bettors as out of the game in the UI.
- [x] Implement the betting sheet screen.
- [x] Show race number, entrants, odds, distance, surface, and bettor bankrolls on the betting sheet.
- [x] Implement turn-based pass-and-play betting entry.
- [x] Implement Win bets.
- [x] Implement Exacta bets.
- [x] Lock in wagers and deduct bankroll immediately on confirmation.
- [x] Implement tentative odds display for the current race.
- [x] Adjust displayed odds as wagers are placed and lock Win payout odds at confirmation.
- [x] Implement the past performance screen.
- [x] Store and display up to the last 3 runs per horse.
- [x] Show enough past data to compare distance, surface, time, and finish summary.
- [x] Support switching between betting and past performance views during betting.
- [x] Implement jockey assignment before each race.
- [x] Support all-CPU races.
- [x] Support one human-controlled outside horse.
- [ ] Support two human-controlled outside horses when two controllers are available.
- [x] Implement the starting gate sequence.
- [x] Implement a side-view race screen with four horses visible.
- [ ] Implement horizontal race progression with independent foreground and background scrolling.
- [ ] Implement four race lanes with inside-lane advantage.
- [ ] Implement lane changing for human jockeys.
- [ ] Implement CPU lane behavior that prefers the rail and moves out to pass.
- [x] Implement horse pace simulation using only integer arithmetic.
- [x] Apply distance and surface effects to horse performance.
- [x] Apply fatigue over race duration.
- [x] Implement Coax input with a modest temporary speed boost.
- [x] Implement Whip input with a strong temporary boost and heavy endurance cost.
- [ ] Implement late-race CPU whip behavior.
- [ ] Add visible furlong markers.
- [ ] Trigger a cue as the leader approaches each furlong marker.
- [x] Detect finish order for all 4 horses.
- [x] Display winning time.
- [x] Display tote-board-style race results.
- [x] Settle Win and Exacta payouts.
- [x] Update bettor bankrolls after each race.
- [x] Advance automatically from race results to the next betting phase.
- [x] End the session after race 10.
- [x] Display final standings and overall winner.
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

- [x] Define compact runtime structures for bettors, horses, races, wagers, past performances, and results.
- [x] Implement SNES controller input handling for menus and races.
- [x] Build the boot flow from title screen to new 10-race program.
- [x] Implement player setup for 1 to 6 bettors.
- [ ] Implement jockey setup for 0 or 1 human riders.

#### Step 2: Session Initialization

- [x] Generate 8 persistent horses at session start.
- [x] Generate persistent horse traits for pace, endurance, break speed, stretch speed, and surface affinity.
- [x] Generate a 10-race program with entrants, distance, and surface.
- [x] Implement bankroll tracking with $750 starting stake and $250 max wager cap.
- [x] Prevent busted bettors from wagering in later rounds.
- [ ] Surface busted-bettor status in the UI.

#### Step 3: Betting Phase

- [x] Implement the betting sheet logic.
- [x] Implement turn-based pass-and-play wager entry.
- [x] Implement tentative odds generation and display.
- [x] Update displayed odds as bets are placed and lock Win payout odds at confirmation.
- [x] Implement Win bet entry and validation.
- [x] Implement Exacta bet entry and validation.
- [x] Deduct bankroll immediately when a wager is confirmed.

#### Step 4: Race History And Handicapping Data

- [x] Implement past-performance data capture for every completed race.
- [x] Limit displayed past performances to the last 3 runs per horse.
- [x] Implement the past performance screen and navigation back to betting.

#### Step 5: Jockey Assignment And Race Setup

- [x] Implement jockey assignment before each race.
- [x] Support all-CPU races.
- [x] Support one human-controlled outside horse.
- [x] Implement the starting gate sequence.

#### Step 6: Core Race Simulation

- [x] Implement the side-view race simulation.
- [ ] Implement four race lanes and inside-lane path advantage.
- [x] Implement integer-only pace simulation.
- [x] Apply distance, surface, and fatigue effects during the race.
- [ ] Implement lane changing for human jockeys.
- [ ] Implement CPU lane choice with rail preference and passing behavior.
- [x] Implement Coax behavior.
- [x] Implement Whip behavior.
- [ ] Implement late-race CPU whip usage.
- [ ] Implement furlong marker state and leader-based timing.
- [x] Detect finish order and winning time.

#### Step 7: Race Presentation Integration

- [ ] Implement independent foreground and background scrolling during races.

#### Step 8: Settlement And Session Progression

- [x] Implement results flow.
- [x] Implement Win bet settlement.
- [x] Implement Exacta bet settlement.
- [x] Update bankrolls after each race.
- [x] Advance automatically to the next race.
- [x] Implement end-of-session standings after race 10.

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

- [x] Extract the race simulation into a host-testable module.
- [x] Add a separate Linux test Makefile delegated from the main Makefile.
- [x] Add deterministic Linux-side unit tests for race distance, surface, and Coax/Whip behavior.
- [ ] Verify a full 10-race session runs from title to final standings without corrupting state.
- [ ] Verify all MVP features work with one controller.
- [ ] Verify two-controller jockey support works when available.
- [ ] Verify bettors cannot wager more than bankroll or more than $250.
- [ ] Verify busted bettors are removed cleanly from later betting rounds.
- [ ] Verify busted bettors are visibly marked as out of the game.
- [ ] Verify only the 4 current entrants can be selected for Win and Exacta bets.
- [ ] Verify Exacta order matters and pays correctly at 15:1.
- [ ] Verify odds update correctly as bets are placed and Win payouts use locked odds.
- [ ] Verify horse traits remain persistent across the entire 10-race session.
- [ ] Verify only the last 3 race records are shown per horse.
- [x] Verify dry, muddy, and turf conditions all affect race outcomes.
- [x] Verify distance groups affect pace and fatigue.
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

- [x] Build boot flow from title screen to a new session.
- [x] Implement player setup for 1 to 6 bettors.
- [ ] Implement jockey setup for 0 or 1 human riders.
- [x] Implement SNES controller input handling for menus and races.
- [x] Define compact data structures for bettors, horses, race programs, wagers, and results.
- [x] Implement session reset and new-game initialization.

Completion gate:

- [x] A new session can be started from the title screen.
- [x] The game can capture valid controller input and move cleanly through setup.
- [ ] Core runtime structures initialize without corrupt state.

### Milestone 2: Race Program And Betting Loop

Goal:
Make one race playable from the betting side before live race presentation is finished.

Tasks:

- [x] Generate 8 persistent horses at session start.
- [x] Generate persistent horse traits for pace, endurance, break speed, stretch speed, and surface affinity.
- [x] Generate a 10-race program with entrants, surfaces, and distance groups.
- [x] Implement bankroll tracking with $750 starting stake.
- [x] Enforce the $250 max wager cap.
- [x] Prevent busted bettors from placing further bets.
- [ ] Show busted bettors as out of the game in the UI.
- [x] Implement the betting sheet logic.
- [x] Show race number, entrants, odds, distance, surface, and bettor bankrolls.
- [x] Implement pass-and-play wager entry.
- [x] Implement Win bets.
- [x] Implement Exacta bets.
- [x] Deduct bankroll immediately when a wager is confirmed.
- [x] Implement tentative odds generation and display.
- [x] Update displayed odds as wagers are placed and lock Win payout odds at confirmation.

Completion gate:

- [x] The first race can be configured and wagered entirely through the betting UI.
- [ ] Bankroll math is correct for valid and invalid wagers.
- [x] Win and Exacta bets can be entered and stored correctly.

### Milestone 3: Past Performances And Results Data

Goal:
Support the original handicapping loop by capturing and displaying prior races.

Tasks:

- [x] Implement race result records per horse.
- [x] Store up to the last 3 runs per horse.
- [x] Implement the past performance screen.
- [x] Show distance, surface, time, and finish summary for prior runs.
- [x] Support switching between betting and past performance views.

Completion gate:

- [x] After completed races, each horse carries forward usable past-performance data.
- [x] The betting phase can display only the last 3 runs per horse.
- [x] Players can compare entrants before wagering without leaving the betting flow.

### Milestone 4: Live Race Prototype

Goal:
Get the race itself running with minimal presentation but correct gameplay behavior.

Tasks:

- [x] Implement the starting gate sequence.
- [x] Implement side-view race simulation for 4 horses.
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
- [x] Detect finish order and winning time.

Completion gate:

- [x] A race can be started from the betting phase and run to completion.
- [ ] Human jockey controls affect at least the outside horses correctly.
- [ ] CPU horses show distinct behavior based on traits, surface, and distance.
- [x] Finish order and winning time are recorded correctly.

### Milestone 5: Session Loop Completion

Goal:
Connect betting, racing, results, and progression into a full 10-race game.

Tasks:

- [x] Implement jockey assignment before each race.
- [x] Support all-CPU races.
- [x] Support one human-controlled outside horse.
- [x] Display tote-board-style race results.
- [x] Settle Win and Exacta payouts.
- [x] Update bettor bankrolls after each race.
- [x] Advance automatically from results to the next betting phase.
- [x] End the session after race 10.
- [x] Display final standings and overall winner.

Completion gate:

- [ ] The game runs from title screen through all 10 races without manual resets.
- [ ] Bets settle correctly after every race.
- [x] Final bankroll standings identify a winner after race 10.

### Milestone 6: MVP Presentation Package

Goal:
Add the minimum art and audio needed for a coherent playable release.

Tasks:

- [x] Define an MVP tile budget that fits the 256 shared 16x16 tile limit.
- [ ] Produce minimal title, betting-sheet, past-performance, race, tote-board, and standings visuals.
- [ ] Produce readable font, numerals, and betting symbols.
- [ ] Produce minimal horse and jockey art readable by color.
- [x] Produce dry, muddy, and turf visual variants.
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
