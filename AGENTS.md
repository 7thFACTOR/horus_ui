# General
- never build/compile the code, regardless of any condition, user will
- never run the built exe
- acp = command means add commit and push to github
- reb = command means rebased onto master so master is in sync with dev branch, push after

# Code Style
- keep the naming style you see in the code already, when not sure, ask
- always use CRLF for EOL, convert when you see otherwise in the file you edit
- empty line between variable declarations and if/for/while/switch etc. conditionals
- empty line between closing curly brace of a function/scope and the next declaration
- .cpp comments start in lowercase, a space after //, header functions desc start with uppercase letter
- minimalistic code, avoid hardcoding, search for places where to put constants, like in Settings struct
- avoid global statics, try to put them in context structs where available
- never prefix globals with g, use descriptive names instead
- for any change, check demo for consistency with the changes, if new feature or widget added, add a demo for it