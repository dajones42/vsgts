# VSGTS Controls

## View Controls

Right mouse click moves the look at location to the selected scene location.
If that location is a rail car, the look at location will track the movement of the car,
and the rail car and train will be selected for other possible actions.

The scroll wheel adjusts the distance between the camera and the look at location
(i.e. zooms in and out).
Zooming in all the way positions the camera .1 meters from the look at location
so the look at location becomes a look from location.

Keyboard view controls include:

- Page Up: zoom in.
- Page Down: zoom out.
- Up Arrow: rotate view direction up.
- Down Arrow: rotate view direction down.
- Left Arrow: rotate view direction to left.
- Right Arrow: rotate view direction to right.
- End: sets the view direction to look down on the current location.
- Home: zooms in all the way and looks horizontally.
- 1: Selects head-out or inside view of controled rail car.
- 2: Looks at first car in train.
- 3: Looks at last car in train.
- 4: Track side view of first car in train.
- 5: Selects head-out or inside view of selected rail car.
- 6: Brakeman view from selected rail car.
- Control Left Arrow: Looks at previous car in train or moves along an activity defined path.
- Control Right Arrow: Looks at next car in train or moves along an activity defined path.

## Train Controls

The keyboard is used for all train controls.
Currently train brakes are always automatic air brakes with a subset of H6 like control positions
(running, lap and service).

- A: Decrease throttle.
- D: Increase throttle.
- W: Increase direction control.
- S: Decrease direction control.
- ";": Decrease train brake (change to lap or running).
- "'": Increate train brake (change to lap or service).
- "[": Decrease engine brake.
- "]": Increase engine brake.
- "{": Decrease hand brake on selected car.
- "}": Increase hand brake on selected car.
- "/": Bail off train brake.
- G: Throw switch ahead of train.
- Shift G: Throw switch behind train.
- U: Uncouple train between cars near view look at location.
- C: Connect air brake hoses.
- Shift C: Take control of selected train.
- H: turn headlight on.
- Shift H: turn headlight off.

### Switching train controls

Switching controls use AI train speed control and stopping distance logic.
The train will stop on coupling or at a misaligned switch that is not close by.
Press decrease train brake key to switch back to normal control.

- ">": Start train moving forward.
- "<": Start train moving backward.
- "^": Increase speed.
- ",": Decrease speed.
- ".": Stop.

## GUI Controls

- F1: Toggle display of menu bar.
- F5: Toggle display of train status window.
- F6: Toggle display of track labels.
- F7: Toggle display of way bills or car labels.

## Simulation Controls

- Z: Pause or reduce time multiplier.
- X: Resume or increase time multiplier.
