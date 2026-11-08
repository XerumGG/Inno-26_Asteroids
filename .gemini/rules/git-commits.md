# Git Commit and Push Protocol

Always format all Git commits and pushes using the following structure:

```
num.dd-mm-yyyy #change commit_message
```

### Format Breakdown:
- **`num`**: 2-digit zero-padded incremental change number for the day, starting at `01` (e.g. `01`, `02`, `03`).
- **`dd-mm-yyyy`**: Current date in day-month-year format (e.g. `08-11-2026`).
- **`#change`**: Change category tag:
  - `#feat`: New feature or gameplay mechanics
  - `#core`: Architectural or foundational system changes
  - `#fix`: Bug fixes or balance adjustments
  - `#docs`: Documentation, design docs, or notes
  - `#refactor`: Code cleanup or restructuring
  - `#style`: Formatting or cosmetic UI updates
- **`commit_message`**: Descriptive sentence explaining the changes.

### Example:
`01.08-11-2026 #feat Initial C++ architecture, arcade hover physics, 5-level auto-clutch, and race lifecycle`

Always use Git LFS for binary files (`.uasset`, `.umap`, textures, meshes, audio).
