---
name: horus-ui
description: Use when working on the horus_ui C++ immediate mode GUI library. Covers architecture, widget patterns, drag-drop, theming, id stack conventions, and build system.
---

# Code Style

- empty line between variable declarations and if/for/while/switch etc. conditionals
- empty line between closing curly brace of a function/scope and the next declaration
- .cpp comments start lowercase, a space after //, header functions desc start with uppercase letter
- minimalistic code, avoid hardcoding, search for places where to put constants, like in Settings struct
- for any change, check demo for consistency with the changes, if new feature or widget added, add a demo for it


# General
- never build, user will

