$fn = 250;
//$fn = 50;

inchIn_mm = 25.4;

tolerance = 1;

//innerD1 = 72 - tolerance;
//innerD2 = 86 + tolerance;
innerD1 = 54 - tolerance;
innerD2 = 66 + tolerance;
outerD = 96;
outerD2 = outerD -3;
outerD3 = outerD -8;


rSphere = (outerD3 + 20) / 2;

height = 4;
groove = 2;

bottomHeight = 33;

wTh = 2;
wBar = 10;

wTouch = 15;
lTouch = 11;

//panel();

panelLid();

//stand();

//bottom();

//translate([0,0,8])
//bulp();

////rotate_about_pt(180,0, [0,0,0])
////rotate([0,0,180])
//caddy();

//bar();

// rotate as per a, v, but around point pt
module rotate_about_pt(a, v, pt) {
    translate(pt)
        rotate(a,v)
            translate(-pt)
                children();   
}

module stand() {
    difference() {
        union() {
            difference() {
                difference() {
                    linear_extrude(bottomHeight) {
                        circle(d=outerD);
                    }
                    linear_extrude(bottomHeight - 1) {
                        circle(d=outerD2);                              
                    }
                }            
                
                translate([outerD2/2 -2,0,2]) {
                    rotate([0,90,0]) {
                        cylinder(h=6,d=5);
                    }
                    translate([1,-2.5,-5])
                        cube([5,5,5]);
                }
                
                hPanel = 10;
                thPanel = 2;
                translate([-36,25,3]) {
                    rotate([90,0,270]) {
                        linear_extrude(hPanel+4) {
                            square([lTouch * 4.5, wTouch + 10]);
                        }
                    }
                }
            }
            translate([0,0,bottomHeight]){
                difference() {
                    linear_extrude(height) {
                        circle(d=outerD3);
                    }
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

            translate([0,0,bottomHeight]) {
                mirror([0,0,1])
                {
                    postHole(-27, 34, 15);
                    postHole(-27, -34, 15);
                    postHole(17, -40, 15);
                    postHole(17, 40, 15);
                    postHole(-43.5, 0, 15);        
                }
            }
        }
        
        translate([0,0,bottomHeight - 1]){
            difference() {               
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
    
    difference(){
        translate([-36,25,3]) {
            rotate([90,0,270]) {
                panel();
            }
        }
        linear_extrude(bottomHeight - 1) {
            circle(d=outerD2);                              
        }
    }
}

module panel() {
    hPanel = 10;
    //hPanel = 5;
    thPanel = 2;
    
    lTerminal = 8;
    wTerminal = 3;
    
    difference() {
        linear_extrude(hPanel+4) {
            square([lTouch * 4.5, wTouch + 10]);
        }

        translate([thPanel /2, thPanel / 2, 0]) {
            linear_extrude(hPanel- thPanel) {
                square([lTouch * 4.5 - thPanel, wTouch + 10 - thPanel]);
            }
        }

        translate([thPanel /2, thPanel / 2, hPanel+thPanel]) {
            linear_extrude(hPanel- thPanel) {
                square([lTouch * 4.5 - thPanel, wTouch + 10 - thPanel]);
            }
        }

        
        translate([5,5,hPanel-thPanel+2]) {
            linear_extrude(thPanel) {
                square([lTouch, wTouch]);
            }
            translate([(lTouch-lTerminal)/2,0,-2]) {
                linear_extrude(thPanel) {
                    square([lTerminal, wTerminal]);
                }
            }
        }
        
        translate([lTouch * 3.5 - 5,5,hPanel-thPanel+2]) {
            linear_extrude(thPanel) {
                square([lTouch, wTouch]);
            }
            translate([(lTouch-lTerminal)/2,0,-2]) {
                linear_extrude(thPanel) {
                    square([lTerminal, wTerminal]);
                }
            }
        }
    }
    
    
    wTouch = 15;
    lTouch = 11;
}

module panelLid() {
    hLid = 1.5;
    thPanel = 2;
    
    lTerminal = 8;
    wTerminal = 3;
    
    difference() {
        union(){
            linear_extrude(hLid) {
                square([lTouch * 4.5, wTouch + 10]);
            }
            translate([thPanel / 2,thPanel / 2, 0]) {
                linear_extrude(hLid*2) {
                    square([lTouch * 4.5 - thPanel, wTouch + 10 - thPanel]);
                }
            }
        }
        
        translate([5,5,hLid]) {
            translate([(lTouch-lTerminal)/2,0,0]) {
                linear_extrude(thPanel) {
                    square([lTerminal, wTerminal]);
                }
            }
        }
        
        translate([lTouch * 3.5 - 5,5,hLid]) {
            translate([(lTouch-lTerminal)/2,0,0]) {
                linear_extrude(thPanel) {
                    square([lTerminal, wTerminal]);
                }
            }
        }
        
        translate([9,15,0]) {
            translate([(lTouch-lTerminal)/2,0,0]) {
                linear_extrude(0.5) {
                    circle(d=10);
                }
            }
        }
        
        translate([lTouch * 3.5 -1,15,0]) {
            translate([(lTouch-lTerminal)/2,0,0]) {
                linear_extrude(0.5) {
                    circle(d=10);
                }
            }
        }
    }
}

module bottom() {
    union() {
        linear_extrude(2) {
            circle(d=outerD);
        }
        
        difference() {
            difference() {
                linear_extrude(10) {
                    circle(d=outerD2);
                }
                linear_extrude(10) {
                    circle(d=outerD2 - 2);                              
                }
            }            
            
            translate([(outerD2 -2) /2 -2,0,6]) {
                translate([1,-2.5,-5])
                    cube([5,5,10]);
            }                     
        }
    }
}

module bulp() {
    difference() {
        translate([0,0,50]) {   
            difference() {
                union(){
                    sphere(r = rSphere);
                    translate([0,0,-28])
                        cylinder(d = outerD, h = 8);
                }
                sphere(r = rSphere - 1);
            }
        }
        union() {
            translate([0,0,-15]){
                linear_extrude(bottomHeight + 10) {
                    circle(d=outerD2);
                }
            }
            
            translate([0,0,bottomHeight]){
                linear_extrude(height) {
                    circle(d=outerD3);
                }
            }
        }
    }
}

module caddy() {
    linear_extrude(wTh) {
        difference(){
            circle(d=outerD2 - tolerance);
            translate([-8.25,0,0]){
                square([57, 28 + wTh*2], true);
            }
            translate([20.25,-outerD2/2,0]){
                square([30, outerD2], false);
            }
            
            translate([-8,-20,0]){
                square([50, 5], true);
            }
            translate([-8,-28,0]){
                square([40, 5], true);
            }
            translate([-3,-36,0]){
                square([25, 5], true);
            }

            translate([-8, 20,0]){
                square([50, 5], true);
            }
            translate([-8, 28,0]){
                square([40, 5], true);
            }
            translate([-8, 36,0]){
                square([25, 5], true);
            }

            translate([17,-40,0]){
                circle(1.5);
            }
            translate([17,40,0]){
                circle(1.5);
            }

            translate([-27,-34,0]){
                circle(1.5);
            }
            translate([-27,34,0]){
                circle(1.5);
            }

            translate([-43.5,0,0]){
                circle(1.5);
            }
            
        }
    }

    translate([-10.75,0,0]){
        cage();
    }
}

module bar() {
    devKitL = 58.5;
    w = wBar;
    wG = 5;
    h = 3;
    
    difference() {
        linear_extrude(h) {            
            square([devKitL, w], false);
        }
        
        translate([devKitL - 25, (w - wG) / 2, 0]) {
            linear_extrude(h) {
                square([25, 5], false);
            }
        }
    }
    
    translate([devKitL - 6,w,0]){
        linear_extrude(h) {
            polygon(points=[[0,0],[6,0],[0,2]]);
        }
    }
    
    mirror([0,1,0]) {
        translate([devKitL - 6,0,0]){
            linear_extrude(h) {
                polygon(points=[[0,0],[6,0],[0,2]]);
            }
        }
    }
    
    
    translate([0,0,h]){
        linear_extrude(9) {            
            square([4, w], false);
        }
    }
    
    translate([16,0,h]){
        linear_extrude(5) {            
            square([10, w], false);
        }
    }

}

module cage() {
    devKitL = 56;
    devKitW = 29;
    devKitWi = 28;    
    devKitLi = 52;
    devKitTh = 7.8;
    
    w = wTh;

    difference() {
        linear_extrude(devKitTh + w) {
            difference() {
                translate([w, 0, 0]) {
                    square([devKitL + w, devKitW + w*2], true);
                }
                square([devKitL, devKitW], true);
            }
        }
        
        //Das Loch für USB
        translate([devKitL / 2 +w, 0, w + 3]) {
            cube([w*2, 10, 6], true);
        }
    }
    
    
    linear_extrude(w) {
        translate([w, 0, devKitTh + w]) {
            difference() {
                square([devKitL + w, devKitW + w*2], true);
                
                translate([w - 3, devKitWi / 2 - w, 0]) {
                    square([devKitLi, w * 2], true);
                }
                translate([w - 3, -devKitWi / 2 + w, 0]) {
                    square([devKitLi, w * 2], true);
                }
            }
        }
    }
    
    translate([devKitL / 2 + w - 19, -4.5, devKitTh + w]) {
        linear_extrude(1) {
            square([20, 21]);
        }       
        difference() {
            linear_extrude(6) {
                square([20, wBar + wTh]);
            }
            translate([0, 1, 1]) {
                linear_extrude(4) {
                    square([19, wBar]);
                }
            }
            
            translate([9, 0, 1]) {
                linear_extrude(4) {
                    square([5, wBar + wTh]);
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

module slice(r=3.0,a1=0, a2=30) { 
    angle = a2 - a1;
    slices = ceil(angle / 90);
    
    aStart = a1;
    aEnd = slices == 1 ? a2 : a1 + 90;
    
    lastSlice = angle % 90 > 0 ? angle % 90 : 90;
    
    union(){
        for(s=[0:1:slices-1]){
            aStart = aStart + s * 90;
            aEnd = s == slices-1 ? aStart + lastSlice : aStart + 90;
            intersection() {
                circle(r=r);
                rotate(aStart) square(r);
                rotate(aEnd - 90) square(r);
            }         
        }
    }
}

offsetZ1 = 0;
module postHole(xPos, yPos, height)
{
    rPost = 3.5;
    rPost2 = 1.05;

    difference() 
    {    
        translate([xPos,yPos, offsetZ1]) cylinder(h=height, r=rPost);
                
        translate([xPos,yPos, offsetZ1 + height/2 +1]) cylinder(h=height -1, r=rPost2);
    }   
}