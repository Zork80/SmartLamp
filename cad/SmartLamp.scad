// --- Configuration ---
mode = "print"; // [print, assembly]

// Render parts individually
doCase = true;
doBottom = true;
doCaddy = true;
doBulb = true;

// Board Selection
esp_model = "DevKit"; // [DevKit, C6_Mini]
Scene = "Moon"; // [Moon, Earth, Mars, Smooth, RingOnly]
pattern_dir = "patterns/"; // Subfolder containing pattern STL files
bulb_wall = 1.6; // Wall thickness for Smooth bulb in mm

hasDisplay = false; // Toggle OLED support (WIP)

// --- GEOMETRIC PARAMETERS ---
LED_diameter = 86;
LED_width = 7;      
tolerance = 1;
outerD = 96;
wTh = 2;
bottomHeight = 33;
height = 4;
groove = 2;

/* [Hidden] */
inchIn_mm = 25.4;
$fn = (mode == "print") ? 150 : 50;
isPrintMode = (mode == "print");
outerD2 = outerD - 3;
outerD3 = outerD - 8;
rSphere = (outerD3 + 20) / 2;

// Calculation of LED ring dimensions
innerD1 = LED_diameter - (2 * LED_width) - tolerance;
innerD2 = LED_diameter + tolerance;

// Board data: [length, width, height, USB width, pin_offset]
board_configs = (esp_model == "C6_Mini") 
    ? [26.5, 18.0, 4.5, 9, 1.5]  // C6_Mini: 1.5mm offset
    : [55.2, 28.2, 4.5, 9, 2.0]; // DevKit: 2.0mm offset

b_len      = board_configs[0];
b_wid      = board_configs[1];
b_hgt      = board_configs[2];
usb_w      = board_configs[3];
pin_offset = board_configs[4]; 

t_w = b_wid + 0.8;
caddy_cut_x = 17 + 3; 
offset_x = caddy_cut_x - (b_len / 2) - wTh;

// OLED Parameters (SSD1306 0.96")
oled_w = 27.8; 
oled_h = 27.3;
oled_screen_w = 22; // Visible display area
oled_screen_h = 11; // Visible display area


// --- START OF LOGIC ---

if (doBottom) {
    bottom();    
}

iterator1 = doBottom ? 1 : 0;
iterator2 = doCase ? iterator1 + 1 : iterator1;
iterator3 = doCaddy ? iterator2 + 1 : iterator2;

translateCase = isPrintMode ? [0,0,0] : [0,0,2];

translate(translateCase) {
    // --- CASE ---
    if (doCase) {
        if(isPrintMode)
            translate([iterator1 * (outerD + 15), 0, bottomHeight + height])
                rotate([180,0,0]) case();
        else
            case();
    }

    // --- CADDY ---
    if (doCaddy) {
        if (isPrintMode) {
            translate([iterator2 * (outerD + 15), 0, 0]) {
                rotate([0, 0, 180]) caddy();
            }
        } else {
            translate([0, 0, 18]) {
                rotate([180, 180, 180]) caddy();
            }
        }
    }
    
    // --- BULB ---
    if (doBulb) {
        if(isPrintMode)
            translate([iterator3 * (outerD + 15), 0, -22])
                rotate([0,0,0]) bulb();
        else
            translate([0,0,11]) bulb();
    }
}

// --- MODULES ---

module caddy() {
    difference() {
        union() {
            linear_extrude(wTh) {
                difference(){
                    circle(d=outerD2 - tolerance);
                    // Weight reduction / ventilation slots
                    translate([-8, -20, 0]) square([50, 5], true);
                    translate([-8, -28, 0]) square([40, 5], true);
                    translate([-3, -36, 0]) square([25, 5], true);
                    translate([-8, 20, 0]) square([50, 5], true);
                    translate([-8, 28, 0]) square([40, 5], true);
                    translate([-8, 36, 0]) square([25, 5], true);
                    
                    // Mounting holes for case posts
                    translate([17, -40, 0]) circle(1.5);
                    translate([17, 40, 0]) circle(1.5);
                    translate([-27, -34, 0]) circle(1.5);
                    translate([-27, 34, 0]) circle(1.5);
                    if(!hasDisplay) translate([-43.5, 0, 0]) circle(1.5);
                }
            }
            translate([offset_x, 0, 0]) cage();
        }
        
        // Slots for pin headers
        translate([offset_x, 0, 0]) {
            for(i = [-1, 1]) {
                translate([0, i * (t_w / 2 - pin_offset), 0])
                    cube([b_len, 3, wTh * 3], true);
            }
        }
        
        // Collision cut for OLED display
        if(hasDisplay) {
            // Remove edge material to clear the display unit at 180 degrees
            translate([-(outerD2/2), 0, wTh/2])
                cube([15, oled_w + 5, wTh + 2], true);
        }

        // Clean cut for caddy edge
        translate([20 + 50, 0, 0])
            cube([100, outerD, 100], true);
    }
}

module cage() {
    w_cage = 1.0; 
    t_l = b_len + 0.5;
    wall_h = b_hgt + wTh + 2.0; 
    snap_l = t_l * 0.7; 
    snap_depth = 0.7; 

    difference() {
        // Outer wall
        translate([0, 0, wall_h/2])
            cube([t_l + 2*w_cage, t_w + 2*w_cage, wall_h], true);
        
        // Internal pocket for the board
        translate([0, 0, wTh + (b_hgt + 5)/2])
            cube([t_l, t_w, b_hgt + 5], true);
            
        // USB port cutout
        translate([t_l/2 + w_cage/2, 0, wTh + 2.5])
            cube([w_cage + 2, usb_w, 5], true);
    }
    
    // Snaps to hold the board in place
    for(i = [-1, 1]) {
        translate([-snap_l/2, i * (t_w/2), b_hgt + wTh - 2.3]) {
            rotate([90, 0, 90]) { 
                linear_extrude(height = snap_l) {
                    polygon(points=[
                        [0, 0],              
                        [i * -snap_depth, 0.4], 
                        [0, 4] 
                    ]);
                }
            }
        }
    }
}

module sector(d, a1, a2) {
    if (a2 - a1 > 180) {
        difference() {
            circle(d=d);
            translate([0,0,-0.5]) sector(d+1, a2-360, a1); 
        }
    } else {
        difference() {
            circle(d=d);
            rotate([0,0,a1]) translate([-d/2, -d/2, -0.5])
                square([d, d/2]);
            rotate([0,0,a2]) translate([-d/2, 0, -0.5])
                square([d, d/2]);
        }
    }
}

module slice(r=3.0, a1=0, a2=30) { 
    angle = a2 - a1;
    slices = ceil(angle / 90);
    lastSlice = angle % 90 > 0 ? angle % 90 : 90;
    
    union(){
        for(s=[0:1:slices-1]){
            startAngle = a1 + s * 90;
            endAngle = (s == slices-1) ? startAngle + lastSlice : startAngle + 90;
            intersection() {
                circle(r=r);
                rotate(startAngle) square(r);
                rotate(endAngle - 90) square(r);
            }         
        }
    }
}

module postHole(xPos, yPos, height) {
    rPost = 3.5;
    rPost2 = 1.05;

    difference() {    
        translate([xPos, yPos, 0]) cylinder(h=height, r=rPost);
        // Hole for self-tapping screw
        translate([xPos, yPos, height/2 + 1]) cylinder(h=height, r=rPost2);
    }   
}

module bulb() {
    difference() {
        translate([0,0,50]) {   
            difference() {
                union(){
                    difference(){
                        translate([0,0,8]) {
                            if(Scene == "Moon")
                                import(str(pattern_dir, "Moon.stl"));
                            else if(Scene == "Earth")
                                import(str(pattern_dir, "Earth.stl"));
                            else if(Scene == "Mars")
                                import(str(pattern_dir, "Mars.stl"));
                            else if(Scene == "Smooth")
                                difference() {
                                    sphere(d = 116);
                                    sphere(d = 116 - 2 * bulb_wall);
                                }
                        }
                        // Flatten bottom for ring mount
                        translate([0,0,-28])
                            cylinder(d = outerD, h = 4);
                    }
                    translate([0,0,-28])
                        cylinder(d = outerD, h = 4);
                }
            }
        }
        union() {
            // Hollow interior
            translate([0,0,-15])
                linear_extrude(bottomHeight + 10) circle(d=outerD2);
            
            // LED Ring seat
            translate([0,0,bottomHeight])
                linear_extrude(height) circle(d=outerD3);
        }
    }
}

module bottom() {
    union() {
        // Base plate
        linear_extrude(2) circle(d=outerD);
        
        // Inner rim for snap-fit/alignment
        difference() {
            difference() {
                linear_extrude(10) circle(d=outerD2);
                linear_extrude(10) circle(d=outerD2 - 2);                                             
            }            
            // USB cable clearance
            translate([(outerD2 - 2) / 2 - 2, 0, 6]) {
                translate([1, -2.5, -5]) cube([5, 5, 10]);
            }                      
        }
    }
}

module case() {
    difference() {
        union() {
            difference() {
                union() {
                    // Main enclosure body
                    difference() {
                        linear_extrude(bottomHeight) circle(d=outerD);
                        linear_extrude(bottomHeight - 1) circle(d=outerD2);
                    }
                    
                    if(hasDisplay) {
                        // Internal block to hold the OLED
                        // Centered vertically to match the cutout
                        rotate([0, 0, 180])
                        translate([outerD2/2 - 2, -oled_w/2 - 1, 2.5])
                            cube([3, oled_w + 2, oled_h + 2]);
                    }
                }
  
                if(hasDisplay) {
                    // OLED Cutouts
                    rotate([0, 0, 180])
                    translate([outerD/2 - 2, 0, bottomHeight/2]) {
                        // 1. Viewing window
                        cube([10, oled_screen_w, oled_screen_h], true);
                        
                        // 2. Internal pocket for PCB flush mounting
                        translate([-1, 0, 0])
                            cube([2, oled_w + 0.5, oled_h + 0.5], true);
                    }          
                }
                
                // USB Port hole
                translate([outerD2/2 - 2, 0, 2]) {
                    rotate([0, 90, 0]) cylinder(h=10, d=5);
                    translate([1, -2.5, -5]) cube([5, 5, 5]);
                }
            }
            
            // Upper mounting ring
            translate([0,0,bottomHeight]){
                difference() {
                    linear_extrude(height) circle(d=outerD3+4);
                    translate([0,0,height-groove]) {
                        linear_extrude(groove) {
                            difference() {
                                circle(d=innerD2);
                                circle(d=innerD1);
                            }        
                        }
                    }
                }
            }

            // Internal mounting posts
            translate([0, 0, bottomHeight]) {
                mirror([0, 0, 1]) {
                    postHole(-27, 34, 15);
                    postHole(-27, -34, 15);
                    postHole(17, -40, 15);
                    postHole(17, 40, 15);
                    if(!hasDisplay) postHole(-43.5, 0, 15); 
                }
            }
        }
        
        // Slots for LED ring wiring/mounting at the top edge
        translate([0, 0, bottomHeight - 1]){
            difference() {                
                linear_extrude(height + 2) {
                    union() {
                        slice(r=innerD2/2, a1=0, a2=10);
                        slice(r=innerD2/2, a1=350, a2=360);
                    }
                }
                linear_extrude(height + 2) {
                    union() {
                        slice(r=innerD1/2, a1=0, a2=10);
                        slice(r=innerD1/2, a1=350, a2=360);
                    }
                }
            }
        }
    }
}