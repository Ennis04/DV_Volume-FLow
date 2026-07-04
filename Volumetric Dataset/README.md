# Volume Visualisation Documentation

This document answers the questions required for **Part 1B** of the assignment.

## Dataset: Head
- **Total number of important surfaces:** 
- **Surface 1:** (e.g., Skin at iso-value ~500)
- **Surface 2:** (e.g., Bone at iso-value ~1150)

## Dataset: Foot
- **Total number of important surfaces:** 
- **Surface 1:** (e.g., Outer tissue at iso-value ~60)
- **Surface 2:** (e.g., Bone structure at iso-value ~120)

## Dataset: Frog
- **Total number of important surfaces:** 
- **Surface 1:** (e.g., Outer layer at iso-value ~40)
- **Surface 2:** (e.g., Inner organs at iso-value ~80)

## Transfer Functions Justification
*Briefly explain your transfer function choices (colors/opacities) and whether it was possible to display all surfaces simultaneously or if they obstructed each other.*

(Your answer here)

## Additional User Controls
In `RayMarching.cpp`, the following additional user controls were implemented:
- **Keys `[` and `]`**: Dynamically shifts the opacity window mapping left or right. This allows you to interactively reveal and hide different density layers without needing to recompile the program.
- On-screen instructions were added to make these controls obvious to the user.
