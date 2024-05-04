# Cap2D
This simple program reads a bitmap (e.g. png) representing the shape of the capacitor plates. Red and green color channels are used to represent the top and bottom plates, respectively. Overlapped areas are visible as yellow due to the additive mix of the red and green channels.

Additional information, that is bitmap-pixel-scale and distance between plates, can be set in the "Params" dialog.

The program solves the capacitance of a defined 2.5D structure.

On the left window/picture there is a charge density map, also numerically readable by the mouse pointer. There are 3 charge density map views: both plates (R-Y-G colors), top and bottom. In particular, there is a cross-section selection line controlling the cross-section view.

The cross-section view on the right window/picture: Thin red (top) and green (bottom) lines represent the capacitor plates. All over around, the window shows the electric-potential color map. At the mouse pointer, there is the Electric field vector displayed. The corresponding electric-potential value and the electric-field absolute value are displayed numerically below. Assuming the voltage on the top and bottom plates to be +1 and -1 Volt. Color mappings can be changed using the right mouse click or by clicking on the "View" menu.

At the moment, there is no calculation for a dielectric influence!
However, it is still possible to use the program to solve the real-life PCB capacitances, multiplying the capacitance value by relative permittivity (Εr):

1. for internal PCB layers - more exact
2. for top/bottom PCB layers - as an estimation only - up to 50% error
   
The software is to be downloaded in two versions: 32-bit and 64-bit. The reason for this is that the equation-solving part, for now, is a straightforward Gaussian elimination, suitable only for a relatively small number of variables and limited by n² storage requirement. To make it a bit better, the 64-bit version can access much bigger memory, available now on the standard PC.
The program starts with a preloaded demo.
