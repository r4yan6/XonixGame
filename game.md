# Xonix Game — Project Rules & Constraints

**Course:** Programming Fundamentals - Summer 2026
**Institution:** FAST-NUCES, Islamabad Campus
**Due:** August 6, 2026 – 11:59 PM
**Total Marks:** 120 (+10 Bonus)
**Group size:** 2 members

---

## ✅ Allowed

- `<SFML/Graphics.hpp>`
- `<time.h>`
- `<cmath>`
- `<cstdlib>`
- `<string>`
- `<cstring>`

## ❌ Restricted / Not Allowed

- `<vector>`
- `<algorithm>`
- Built-in lists
- Built-in queues
- Built-in stack
- **OOP concepts** (classes, objects, inheritance, polymorphism, etc.) — *this constraint is not printed in the official assignment PDF; it's noted here per course convention for a Programming Fundamentals course. Confirm explicitly with your instructor/TA if there's any doubt.*

> Since STL containers are off-limits, plan on plain arrays / fixed-size C-style structures and manual algorithms (sorting the scoreboard, managing enemy lists, etc.) instead of `std::vector`/`std::sort`.

---

## Submission Rules

- Submit via **Google Classroom only** — no email, no exceptions.
- No late submissions accepted.
- Combine all work into a single `.zip` named `ROLL-NUM.zip` (e.g. `23i0000.zip`).
- Group leader is responsible for correct, timely submission.

---

## Feature Checklist

| # | Feature | Marks | Key Points |
|---|---------|-------|------------|
| 1 | Basic Features | 10 | Single/two-player modes, start menu (Start/Select Level/Scoreboard), end menu, high-score highlight, Restart/Main Menu/Exit |
| 2 | Difficulty & Enemy Count | 20 | Easy (2), Medium (4), Hard (6), Continuous (starts at 2, +2 every 20s) |
| 3 | Movement Counter | 5 | Count +1 each time player *starts* building tiles (not per tile) |
| 4 | Enemy Speed & Movement | 20 | Elapsed time tracked; speed +fixed amount every 20s; at 30s, half of enemies switch to geometric patterns (zig-zag, circular, etc.) — each pattern in its own function |
| 5 | Scoring & Rewards | 10 | See scoring logic below |
| 6 | Scoreboard | 10 | Top 5 scores in a `.txt` file, descending order, score + time, auto-update on qualifying game over |
| 7 | Two-Player Mode | 25 | Shared board + timer, individual score/power-up display, P1 = arrow keys, P2 = WASD; see collision rules below |
| 8 | Report | 20 | 1-2 pages: workflow, diagram (e.g. draw.io), task split, feature progress, rationale for unspecified design choices |
| 9 | Bonus Features | 10 | Sound effects + background color change on power-up use — **only counts if all required features are complete** |

**Total: 120 + 10 Bonus**

---

## Scoring Logic (Feature 5) — Read Carefully

1. Base: 1 tile captured = 1 point.
2. **2x bonus:** capturing >10 tiles in a single move → double points.
3. After the 2x bonus has triggered **3 times total**, the threshold drops from >10 to **>5** tiles for future 2x bonuses.
4. **4x bonus:** once 5 total bonus occurrences (2x or 4x) have happened, capturing >5 tiles → quadruple points instead.
5. **Power-ups:**
   - First one awarded at score 50.
   - Then at 70, 100, 130, and every +30 after that.
   - Unused power-ups stack in inventory (don't auto-trigger).
   - Effect: freeze all enemies for 3 seconds (and in 2P mode, also freeze the opposing player for 3 seconds).

## Two-Player Collision Rules (Feature 7)

- Both players mid-build and collide with each other → **both die**.
- Player A touches Player B's in-progress (under-construction) trail → **A dies**, and vice versa.
- Player A collides with Player B while only A is building (B is not) → **A dies**, and vice versa.
- Power-up use freezes both enemies **and** the other player for 3 seconds.
- Game ends when both players are eliminated; whoever has the higher score wins.

---

## Notes for Implementation Planning

- Movement rule (already implemented in starter): while building, reversing direction directly = death.
- Enemy touching an under-construction tile = player death (already implemented).
- Divide the 9 features across two people — natural split is roughly: (menus + scoreboard + report) vs (enemy AI/patterns + scoring system + two-player logic), then integrate.
- Document any assumption you make for unspecified details directly in the report — this is explicitly asked for and likely factors into grading judgment calls.
