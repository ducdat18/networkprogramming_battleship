# Person 1 - Task Checklist

Use this checklist to track your progress. Check off items as you complete them.

## 📖 Phase 1: Preparation (30 minutes)

- [ ] Read `README.md` in this folder
- [ ] Read `../SUMMARY.md` for overall context
- [ ] Review `1_team_division.md`
- [ ] Review `2_introduction.md`
- [ ] Review `3_design_diagrams.md`
- [ ] Review `6_conclusion.md`
- [ ] Understand what diagrams need to be created (read `diagrams/README_DIAGRAMS.md`)

**Completion Criteria**: You have a clear understanding of all sections and what needs to be done.

---

## ✏️ Phase 2: Customize Content (1 hour)

### Section 1: Team Division

- [ ] Open `1_team_division.md` in text editor
- [ ] Replace `[Name 1]` with actual team member 1 name
- [ ] Replace `[Name 2]` with actual team member 2 name
- [ ] Fill in Student IDs
- [ ] Fill in Email addresses
- [ ] Review task assignments and update if needed
- [ ] Run git commands to get accurate statistics:
  ```bash
  git log --author="Your Name" --oneline | wc -l
  git log --author="Other Name" --oneline | wc -l
  ```
- [ ] Update "Lines of Code" statistics for each person
- [ ] Update "Key Files" list based on actual contributions
- [ ] Save file

**Completion Criteria**: Section 1 has all real names, IDs, and accurate statistics.

---

### Section 2: Introduction (optional review)

- [ ] Open `2_introduction.md`
- [ ] Review content for accuracy
- [ ] Add any missing details about project objectives
- [ ] Verify technology stack is complete
- [ ] Check that all phase completion statuses are accurate
- [ ] Save if any changes made

**Completion Criteria**: Section 2 is accurate and up-to-date.

---

### Section 6: Conclusion (optional review)

- [ ] Open `6_conclusion.md`
- [ ] Review challenges and solutions - add any you personally faced
- [ ] Update team contribution statistics if needed
- [ ] Verify all achievements listed are accurate
- [ ] Check appendices for correct file paths
- [ ] Save if any changes made

**Completion Criteria**: Section 6 reflects actual project experience.

---

## 🎨 Phase 3: Create Diagrams (4-5 hours)

### Setup

- [ ] Decide which tool to use (draw.io recommended)
- [ ] If using draw.io, open https://app.diagrams.net/
- [ ] Create folder to save .drawio source files (for later editing)
- [ ] Review color scheme in `diagrams/README_DIAGRAMS.md`

### Diagram 1: Use Case Diagram (HIGH PRIORITY) ⭐

- [ ] Read Section 3.1 in `3_design_diagrams.md`
- [ ] Open draw.io → New Diagram → UML → Use Case Diagram
- [ ] Add actors: Player, Server System
- [ ] Add 10 player use cases (Register, Login, View Player List, etc.)
- [ ] Add 7 server use cases (Manage Sessions, Track Players, etc.)
- [ ] Add relationships: includes, extends, requires
- [ ] Apply colors: actors in blue, use cases in light gray
- [ ] Export as PNG: File → Export as → PNG (zoom 200%)
- [ ] Save as `diagrams/01_use_case_diagram.png`
- [ ] Verify file size (target: ~500 KB)

**Completion Criteria**: Clear use case diagram showing all actors and use cases with relationships.

---

### Diagram 2: System Architecture (HIGH PRIORITY) ⭐

- [ ] Read Section 3.2 in `3_design_diagrams.md`
- [ ] Open draw.io → New Diagram → Basic Flowchart
- [ ] Create 3 layers: Client (blue), Server (green), Database (yellow)
- [ ] Client layer: Add UI Manager, screens, ClientNetwork, SessionStorage
- [ ] Server layer: Add Server, handlers, managers
- [ ] Database layer: Add 5 tables
- [ ] Connect components with labeled arrows
- [ ] Add legends for colors
- [ ] Export as `diagrams/02_system_architecture.png`
- [ ] Verify file size (target: ~800 KB)

**Completion Criteria**: Complete 3-tier architecture diagram showing all major components.

---

### Diagram 3: Authentication Flow (HIGH PRIORITY) ⭐

- [ ] Read Section 3.3 in `3_design_diagrams.md`
- [ ] Open draw.io → New Diagram → Flowchart
- [ ] Create 3 swim lanes: Client, Server, Database
- [ ] Draw Registration flow (see text in Section 3.3)
- [ ] Draw Login flow
- [ ] Draw Auto-login flow
- [ ] Label all message arrows (REGISTER_REQUEST, LOGIN_RESPONSE, etc.)
- [ ] Use consistent colors for success (green) and error (red)
- [ ] Export as `diagrams/03_authentication_flow.png`
- [ ] Verify file size (target: ~1.2 MB)

**Completion Criteria**: Three authentication flows clearly illustrated with message sequences.

---

### Diagram 4: Matchmaking Flow (HIGH PRIORITY) ⭐

- [ ] Read Section 3.4 in `3_design_diagrams.md`
- [ ] Open draw.io → New Diagram → Flowchart
- [ ] Create 3 swim lanes: Player A, Server, Player B
- [ ] Draw Direct Challenge flow
- [ ] Draw Challenge Timeout flow
- [ ] Label all messages (CHALLENGE_SEND, CHALLENGE_RECEIVED, etc.)
- [ ] Show 60s timeout with timer notation
- [ ] Export as `diagrams/04_matchmaking_flow.png`
- [ ] Verify file size (target: ~1.0 MB)

**Completion Criteria**: Challenge lifecycle clearly shown from send to accept/decline/timeout.

---

### Diagram 5: Database ER Diagram (HIGH PRIORITY) ⭐

- [ ] Read Section 3.6 in `3_design_diagrams.md`
- [ ] Open draw.io → New Diagram → Entity Relationship
- [ ] Create 5 table entities: users, sessions, matches, match_boards, match_moves
- [ ] For each table:
  - [ ] Add table name as header
  - [ ] List all attributes
  - [ ] Mark PK (Primary Key) with key icon or asterisk
  - [ ] Mark FK (Foreign Key) with arrow icon
  - [ ] Mark UNIQUE constraints
- [ ] Draw relationships:
  - [ ] users → sessions (1-to-many)
  - [ ] users → matches (1-to-many)
  - [ ] matches → match_boards (1-to-2)
  - [ ] matches → match_moves (1-to-many)
- [ ] Use crow's foot notation
- [ ] Export as `diagrams/06_database_er_diagram.png`
- [ ] Verify file size (target: ~600 KB)

**Completion Criteria**: Complete database schema with all tables, fields, and relationships.

---

### Diagram 6: Class Diagram (HIGH PRIORITY) ⭐

- [ ] Read Section 3.7 in `3_design_diagrams.md`
- [ ] Open draw.io → New Diagram → UML → Class Diagram
- [ ] Create Server package (light green)
- [ ] Add classes: Server, DatabaseManager, PlayerManager, ChallengeManager, AuthHandler
- [ ] For each class, add 3 sections:
  - [ ] Top: Class name
  - [ ] Middle: Attributes (- for private, + for public)
  - [ ] Bottom: Methods (- for private, + for public)
- [ ] Create Client package (light blue)
- [ ] Add classes: UIManager, ClientNetwork, SessionStorage
- [ ] Draw relationships:
  - [ ] Solid arrow with diamond: composition (Server contains DatabaseManager)
  - [ ] Dashed arrow: dependency (Server uses AuthHandler)
- [ ] Export as `diagrams/07_class_diagram.png`
- [ ] Verify file size (target: ~1.2 MB)

**Completion Criteria**: Class diagram showing main classes with attributes, methods, and relationships.

---

### Diagram 7-11: Optional Diagrams (MEDIUM PRIORITY)

- [ ] Diagram 5: Gameplay Flow (`05_gameplay_flow.png`)
- [ ] Diagram 8: Sequence - Authentication (`08_seq_authentication.png`)
- [ ] Diagram 9: Sequence - Challenge (`09_seq_challenge.png`)
- [ ] Diagram 10: Sequence - Status Broadcast (`10_seq_status_broadcast.png`)
- [ ] Diagram 11: Sequence - Game Move (`11_seq_game_move.png`)

**Note**: These are nice-to-have. Focus on diagrams 1-6 first.

---

## 📄 Phase 4: Convert to Word Document (1.5 hours)

### Create Word Document

- [ ] Open Microsoft Word (or Google Docs)
- [ ] Create new blank document
- [ ] Save as `person1_draft.docx`

### Format Document

- [ ] Set font: Times New Roman, 12pt
- [ ] Set line spacing: 1.5
- [ ] Set margins: 1 inch (2.54 cm) all sides
- [ ] Add header with project title (optional)
- [ ] Add footer with page numbers

### Create Cover Page

- [ ] Add project title: "Battleship Multiplayer - Network Programming Project"
- [ ] Add team member names and IDs
- [ ] Add course name and instructor
- [ ] Add date: 2026-01-07
- [ ] Add university/institution name
- [ ] Center align, larger font (18-20pt for title)

### Add Table of Contents

- [ ] Insert page break after cover
- [ ] Add "Table of Contents" heading
- [ ] Insert automatic table of contents (Word: References → Table of Contents)
- [ ] Note: Will need to update after all content is added

### Copy Content

#### Section 1: Team Division

- [ ] Add page break
- [ ] Add heading: "1. Team Division and Work Distribution" (14-16pt, bold)
- [ ] Copy content from `1_team_division.md`
- [ ] Paste as plain text
- [ ] Format tables (use Word table feature)
- [ ] Apply consistent formatting

#### Section 2: Introduction

- [ ] Add page break
- [ ] Add heading: "2. Introduction" (14-16pt, bold)
- [ ] Copy content from `2_introduction.md`
- [ ] Format subsections (2.1, 2.2, etc.) with smaller bold headings
- [ ] Format tables and code blocks with monospace font
- [ ] Apply consistent formatting

#### Section 3: Design Diagrams

- [ ] Add page break
- [ ] Add heading: "3. Design Diagrams and System Analysis" (14-16pt, bold)
- [ ] For each subsection (3.1 - 3.8):
  - [ ] Add subsection heading (e.g., "3.1 Use Case Diagram")
  - [ ] Copy descriptive text from `3_design_diagrams.md`
  - [ ] **Remove** diagram instructions and PlantUML code
  - [ ] Insert actual PNG diagram:
    - [ ] Place cursor where diagram should appear
    - [ ] Insert → Picture → Select PNG file
    - [ ] Resize to page width (~6.5 inches)
    - [ ] Center align
    - [ ] Add caption below: "Figure X: [Diagram Name]"
    - [ ] Apply caption style (Italic, smaller font)

#### Section 6: Conclusion

- [ ] Add page break
- [ ] Add heading: "6. Conclusion" (14-16pt, bold)
- [ ] Copy content from `6_conclusion.md`
- [ ] Format subsections (6.1, 6.2, etc.)
- [ ] Format tables and code blocks
- [ ] Apply consistent formatting

### Finalize Document

- [ ] Update table of contents (right-click → Update Field)
- [ ] Check all page numbers are correct
- [ ] Verify all headings are consistently formatted
- [ ] Check all figures are numbered sequentially
- [ ] Verify all tables are formatted properly
- [ ] Save as `person1_draft.docx`

**Completion Criteria**: Complete Word document with all sections, formatted professionally.

---

## 🔍 Phase 5: Proofread and Polish (1 hour)

### Content Review

- [ ] Read through entire document start to finish
- [ ] Check that all `[Name 1]`, `[Name 2]` placeholders are replaced
- [ ] Check that all `[TODO]`, `[FILL IN]` placeholders are gone
- [ ] Verify all statistics and numbers are accurate
- [ ] Check that all file paths exist (e.g., `server/src/server.cpp`)

### Grammar and Spelling

- [ ] Run Word spell check (F7)
- [ ] Fix all spelling errors
- [ ] Fix all grammar suggestions (review each one)
- [ ] Check for consistent terminology:
  - [ ] "player" vs "user" (use consistently)
  - [ ] "match" vs "game" (use consistently)
  - [ ] "server" vs "backend" (use consistently)

### Formatting Review

- [ ] All headings are consistently formatted
- [ ] All body text is 12pt Times New Roman, 1.5 spacing
- [ ] All diagrams are inserted and visible
- [ ] All diagrams have captions
- [ ] All tables are formatted properly
- [ ] Page breaks are in correct places (no orphaned headings)
- [ ] No extra blank pages

### Figure and Table Numbers

- [ ] All figures numbered sequentially (Figure 1, 2, 3, ...)
- [ ] All tables numbered sequentially (Table 1, 2, 3, ...)
- [ ] All cross-references correct (e.g., "see Figure 5" actually refers to Figure 5)

### Final Checks

- [ ] Document is 12-15 pages (your portion)
- [ ] All sections present (1, 2, 3, 6)
- [ ] At least 6 diagrams inserted
- [ ] No placeholder text remaining
- [ ] Professional appearance
- [ ] Save as `person1_final.docx`

**Completion Criteria**: Polished, professional document ready to share with Person 2.

---

## 🤝 Phase 6: Coordinate with Person 2 (30 minutes)

### Share Draft

- [ ] Send `person1_final.docx` to Person 2
- [ ] Ask Person 2 to send their draft `person2_final.docx`
- [ ] Agree on merge date/time

### Review Person 2's Draft

- [ ] Open Person 2's draft
- [ ] Check for content overlap (avoid duplication)
- [ ] Check for consistent terminology
- [ ] Note any issues to discuss

### Coordination Meeting

- [ ] Schedule 30-minute call/meeting with Person 2
- [ ] Discuss:
  - [ ] Any overlapping content to remove
  - [ ] Terminology consistency (agree on terms)
  - [ ] Figure numbering (who has Figure 1-X, who has X+1 onwards)
  - [ ] Page count (are we within 20-30 pages?)
  - [ ] Who will do final merge (usually Person 2)
  - [ ] Timeline for final submission

### Provide Feedback

- [ ] Give Person 2 any feedback on their sections
- [ ] Receive feedback on your sections
- [ ] Make any final adjustments based on feedback
- [ ] Save updated version

**Completion Criteria**: Both drafts reviewed, feedback exchanged, merge plan agreed.

---

## ✅ Phase 7: Final Submission (Coordinate with Person 2)

### Final Merge (Person 2's responsibility)

- [ ] Person 2 merges both documents
- [ ] Person 2 updates table of contents
- [ ] Person 2 renumbers all figures sequentially
- [ ] Person 2 checks page count (20-30 pages)
- [ ] Person 2 generates final PDF

### Final Review (Both)

- [ ] Review merged document together
- [ ] Check table of contents is correct
- [ ] Check all page numbers
- [ ] Check all figures and captions
- [ ] Fix any last-minute issues

### Submission (Person 1 or Person 2 - decide who)

- [ ] Open Teams
- [ ] Navigate to correct post
- [ ] Upload `Battleship_Report_Final.pdf`
- [ ] Verify file uploaded successfully
- [ ] Confirm both team members are listed
- [ ] Celebrate! 🎉

**Completion Criteria**: Report submitted on Teams before deadline.

---

## 📊 Progress Tracking

### Summary

- [ ] Phase 1: Preparation (30 min) - DONE
- [ ] Phase 2: Customize Content (1 hour) - DONE
- [ ] Phase 3: Create Diagrams (4-5 hours) - DONE
  - [ ] 6 high-priority diagrams completed
  - [ ] 5 optional diagrams (bonus)
- [ ] Phase 4: Convert to Word (1.5 hours) - DONE
- [ ] Phase 5: Proofread (1 hour) - DONE
- [ ] Phase 6: Coordinate (30 min) - DONE
- [ ] Phase 7: Submit (15 min) - DONE

**Total Time**: 8-9 hours

### Current Status

**Date Started**: _______________
**Expected Completion**: _______________
**Actual Completion**: _______________

**Blockers/Issues**:
- (List any problems here)

**Help Needed From**:
- Person 2: (e.g., "Need to confirm team member names")
- Instructor: (e.g., "Unclear on diagram requirements")

---

## 🎯 Quality Standards

Before marking anything as "done", ensure it meets these standards:

**Diagrams**:
- ✅ High resolution (600 DPI, 2000+ pixels wide)
- ✅ Clear and readable (text at least 11pt)
- ✅ Consistent colors and style
- ✅ All elements labeled
- ✅ Professional appearance

**Writing**:
- ✅ No spelling errors
- ✅ No grammar errors
- ✅ Technical terms used correctly
- ✅ Clear and concise explanations
- ✅ Professional tone

**Formatting**:
- ✅ Consistent fonts and sizes
- ✅ Proper heading hierarchy
- ✅ Tables formatted neatly
- ✅ Page breaks in correct places
- ✅ No orphaned headings

**Completeness**:
- ✅ All required sections present
- ✅ No placeholder text
- ✅ All names and IDs filled in
- ✅ All diagrams inserted
- ✅ Page count appropriate (12-15 pages)

---

**Good luck! Use this checklist to stay organized and on track.** ✨
