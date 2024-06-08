example gcode:
G0 X-3 Z-3 Y1
G1 X3 Z-3 Y1
G1 X3 Z3 Y1
G1 X-3 Z3 Y1
G1 X0 Z0 Y1
G1 X0 Z0 Y2
G1 X3 Z3 Y2
G1 X3 Z-3 Y2
G1 X-3 Z-3 Y2
G1 X0 Z0 Y1

![image](https://github.com/mik305/3d_printer/assets/95429175/1c993d33-0291-47b1-8c66-255cd8e30204)

ruch kamery po wciśnięciu alt: w, a, s, d, e, q, ctrl, spacja
ruch ekstrudera: strzałki

to do:
- w oknie imgui wybór czy ruch ma być strzałkami czy z kodu gcode
- opcja wklejania gcoda i drukarka reagująca na ten kod
- poprawienie kształtu ekstrudera z kwadratu
- zostawianie materiału za ekstruderem
- uprzątnięcie kodu, usunięcie pozostałości światła - nie potrzebne już
