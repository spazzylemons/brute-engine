# Brute

First-person 3D game for Playdate.

## Code structure

The C engine is all in one directory, with each file and public symbol prefixed
according to its module. This makes finding symbols easier without even needing
to look them up, and keeps related code together in alphabetical order. It also
prevents needing to adjust relative include directives. The modules are:

- A: Actors
- B: Brute core module
- I: Playdate interface module
- M: Map loading and processing
- R: Rendering
- U: Utilities (Math, etc.)
