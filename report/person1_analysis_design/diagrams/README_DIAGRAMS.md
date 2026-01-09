# Diagram Creation Guide

This folder should contain all 11 diagrams for Section 3 of the report.

## Required Diagrams

### High Priority (MUST HAVE)
1. ✅ `01_use_case_diagram.png` - Use case diagram showing all actors and use cases
2. ✅ `02_system_architecture.png` - Three-tier architecture (Client, Server, Database)
3. ✅ `03_authentication_flow.png` - Auth flow (Register, Login, Auto-login)
4. ✅ `04_matchmaking_flow.png` - Challenge flow (Direct challenge, Timeout)
5. ✅ `06_database_er_diagram.png` - Database schema with 5 tables
6. ✅ `07_class_diagram.png` - Main classes (Server, Client, Managers, Handlers)

### Medium Priority (NICE TO HAVE)
7. ⚪ `05_gameplay_flow.png` - Gameplay flow (Ship placement, Turn-based, Match end)
8. ⚪ `08_seq_authentication.png` - Sequence diagram: Complete auth flow
9. ⚪ `09_seq_challenge.png` - Sequence diagram: Challenge acceptance
10. ⚪ `10_seq_status_broadcast.png` - Sequence diagram: Status broadcast to 3 players
11. ⚪ `11_seq_game_move.png` - Sequence diagram: Game move handling

## Image Requirements

- **Format**: PNG
- **Resolution**: Minimum 600 DPI (for print quality)
- **Size**: Width should be 2000-2500 pixels (will scale to ~6.5 inches in Word)
- **Background**: White or transparent
- **File Size**: Try to keep under 2 MB each (compress if needed)

## Recommended Tools

### Option 1: draw.io (Easiest)
- **Website**: https://app.diagrams.net/
- **Desktop App**: https://github.com/jgraph/drawio-desktop/releases
- **Pros**: Free, intuitive, lots of templates
- **Cons**: None really

**How to use**:
1. Go to app.diagrams.net
2. Choose "Create New Diagram"
3. Select template (UML, Flowchart, etc.)
4. Drag and drop shapes
5. Export: File → Export as → PNG (set zoom to 200%)

### Option 2: Lucidchart (Professional)
- **Website**: https://www.lucidchart.com/
- **Pros**: Beautiful templates, collaboration features
- **Cons**: Limited free tier (60 objects per diagram)

### Option 3: PlantUML (For Programmers)
- **Website**: https://plantuml.com/
- **Pros**: Text-based, version control friendly
- **Cons**: Steeper learning curve

See `plantuml_templates.puml` in this folder for ready-to-use templates.

## Quick Start with draw.io

### 1. Use Case Diagram
1. Open draw.io
2. File → New Diagram → UML → Use Case Diagram
3. Add stick figures for actors (Player, Server)
4. Add ellipses for use cases (Login, Register, Send Challenge, etc.)
5. Connect with arrows
6. Export as PNG (File → Export as → PNG, zoom 200%)

### 2. System Architecture
1. File → New Diagram → Basic Flowchart
2. Use rectangles for components
3. Use cylinder for database
4. Add arrows for data flow
5. Group by layers (use background rectangles with light colors)

### 3. Flow Diagrams
1. File → New Diagram → Flowchart
2. Use swim lanes for different actors (Client, Server, Database)
3. Use rectangles for processes
4. Use arrows for message flow
5. Label all arrows with message names

### 4. ER Diagram
1. File → New Diagram → Entity Relationship
2. Add rectangles for tables
3. List attributes inside (mark PK, FK)
4. Connect with lines (crow's foot notation for 1-to-many)

### 5. Class Diagram
1. File → New Diagram → UML → Class Diagram
2. Add rectangles divided into 3 sections:
   - Top: Class name
   - Middle: Attributes
   - Bottom: Methods
3. Use arrows for relationships (solid for composition, dashed for dependency)

## Color Scheme (Consistent across all diagrams)

```
Client components:  #ADD8E6 (Light Blue)
Server components:  #90EE90 (Light Green)
Database:           #FFFFE0 (Light Yellow)
Managers/Handlers:  #FFB6C1 (Light Pink)
Network/Messages:   #D3D3D3 (Light Gray)

Text:               #000000 (Black)
Arrows:             #000000 (Black)
Success flow:       #00AA00 (Green)
Error flow:         #DD0000 (Red)
```

## Export Settings (draw.io)

When exporting from draw.io:
1. File → Export as → PNG
2. **Zoom**: 200% (for high DPI)
3. **Border Width**: 10 pixels
4. **Transparent Background**: ❌ (use white)
5. **Include a copy of my diagram**: ✅ (allows re-editing)

This will generate high-quality images suitable for printing.

## Tips for Better Diagrams

1. **Consistency**: Use same shapes for same concepts across all diagrams
2. **Labels**: Label all arrows and connections
3. **Spacing**: Don't crowd elements - leave white space
4. **Alignment**: Use grid/snap to align elements neatly
5. **Font Size**: Minimum 11pt, prefer 12-14pt
6. **Legends**: Add color legend if using multiple colors
7. **Simplicity**: Show only essential details, hide complexity

## Example File Sizes (target)

```
01_use_case_diagram.png:      ~500 KB
02_system_architecture.png:   ~800 KB
03_authentication_flow.png:   ~1.2 MB
04_matchmaking_flow.png:      ~1.0 MB
05_gameplay_flow.png:         ~1.5 MB
06_database_er_diagram.png:   ~600 KB
07_class_diagram.png:         ~1.2 MB
08-11_seq_*.png:              ~800 KB each

Total: ~8-10 MB (well within Word file limits)
```

## Diagram Creation Checklist

For each diagram:
- [ ] Created with correct content (refer to Section 3 in `3_design_diagrams.md`)
- [ ] Exported as PNG (2000+ pixels wide)
- [ ] Named correctly (e.g., `01_use_case_diagram.png`)
- [ ] File size reasonable (<2 MB)
- [ ] Colors consistent with scheme above
- [ ] All text readable (zoom to 100% and check)
- [ ] Saved in this `diagrams/` folder

## Need Help?

If stuck on creating a specific diagram:
1. Search Google Images for "[diagram type] example" (e.g., "use case diagram example")
2. Watch YouTube tutorial: "How to create [diagram type] in draw.io"
3. Refer to the detailed text descriptions in `../3_design_diagrams.md`

Good luck! Take your time with these - quality diagrams make a huge difference in report clarity.
