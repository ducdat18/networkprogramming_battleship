# Packet Structure Diagrams

Mermaid diagrams for visualizing packet structure and binary format.

## Diagram Files

1. **wire_format.mmd** - Complete message wire format (Header + Payload)
2. **message_header_layout.mmd** - MessageHeader memory layout with offsets
3. **hex_dump_login.mmd** - LoginRequest hex dump example (96 bytes)
4. **hex_dump_move.mmd** - MoveMessage hex dump example (6 bytes)
5. **serialization_process.mmd** - Serialization/deserialization flow

## Building Diagrams

### Option 1: Using Mermaid CLI (Recommended)

Install mermaid-cli:
```bash
npm install -g @mermaid-js/mermaid-cli
```

Build all diagrams:
```bash
./build_diagrams.sh
```

This will generate PNG files in `../images/`:
- `wire_format.png`
- `message_header_layout.png`
- `hex_dump_login.png`
- `hex_dump_move.png`
- `serialization_process.png`

### Option 2: Online Mermaid Editor

1. Go to https://mermaid.live/
2. Copy contents of `.mmd` file
3. Click "Download PNG"
4. Save to `../images/` with correct filename

### Option 3: VS Code Extension

Install "Mermaid Preview" extension:
1. Open `.mmd` file in VS Code
2. Right-click → "Mermaid: Preview"
3. Right-click preview → "Export to PNG"

## Diagram Usage in LaTeX

After building, these images are referenced in the LaTeX sections:

- `01_protocol_overview.tex` → `wire_format.png`
- `02_message_header.tex` → `message_header_layout.png`
- `04_auth_packets.tex` → `hex_dump_login.png`
- `05_matchmaking_gameplay_packets.tex` → `hex_dump_move.png`
- `06_serialization.tex` → `serialization_process.png`

## Notes

- All diagrams use `theme: base` for clean, professional look
- Colors are consistent across diagrams
- Export at 1200px width for high quality
- Use transparent background for LaTeX integration
