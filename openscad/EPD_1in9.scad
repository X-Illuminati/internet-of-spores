// Waveshare EPD_1in9 Segmented e-Paper Display
// Modeled from https://www.waveshare.com/w/upload/5/53/1.9inch_Segment_e-Paper_Specification.pdf

$fn=36;
FREECAD=1; //set to 1 for FreeCAD rendering, 0 for OpenSCAD preview

// Helpful Constants
THICKNESS = 1.2; //measured: DS value is 1.14
EPD_BG_H = 49.35;
EPD_BG_W = 32.11;
EPD_EPL_H = 43.75;
EPD_EPL_W = 29.11;
EPD_EPL_OFF = 1.5;
EPD_FPC_OFF_H = 0.81;
EPD_FPC_OFF_L = 20; //measured: DS doesn't indicate

FPC_W1 = 5.35; //width at bottom glass: measured - DS doesn't indicate
FPC_W2 = 4.5; //width at connector plastic
FPC_L = 16.25; //from bend to connector
FPC_H = 11.18; //from bottom glass to bend
FPC_T = 0.08; //kapton thickness
CON_L = 6; //connector plastic
CON_T = 0.3; //connector plastic thickness: includes FPC_T
FPC_FOLD_DIST = 0;
FPC_FOLD_THICKNESS = 2.5; //pretty much a guess: height of circuit board + EPD_FPC_OFF_H + a bitextra

EPD_COLOR_TOP = "white";
EPD_COLOR_BOT = "black";
SEG_COLOR_TOP = "black";
SEG_COLOR_BOT = "white";
FPC_COLOR = "orange";
FPC_CON_COLOR = "black";

SEG_UNIT_B = 1.1;
SEG_UNIT_S = SEG_UNIT_B / 1.6;
SEG_TALL_H = 4.95;
SEG_SHORT_H = SEG_TALL_H / 1.6;
SEG_LONG_W = 2.87;
SEG_THIN_W = SEG_LONG_W / 1.6;

// Helpful modules
module inset() {
    difference() {
        children(0);
        translate([0,0,-.1])
            scale([1,1,2])
                children([1:$children-1]);
    }
    children([1:$children-1]);
}

module segment(top_color=SEG_COLOR_TOP, bot_color=SEG_COLOR_BOT) {
    color(bot_color)
        scale([1,1,0.5])
            children();
    color(top_color)
        translate([0,0,THICKNESS/2])
            scale([1,1,0.5])
                children();    
}

module big_unit_seg()
    cube([SEG_UNIT_B, SEG_UNIT_B, THICKNESS]);

module small_unit_seg()
    cube([SEG_UNIT_S, SEG_UNIT_S, THICKNESS]);

module tall_seg()
    cube([SEG_TALL_H, SEG_UNIT_B, THICKNESS]);

module short_seg()
    cube([SEG_SHORT_H, SEG_UNIT_S, THICKNESS]);

module long_seg()
    cube([SEG_UNIT_B, SEG_LONG_W, THICKNESS]);

module thin_seg()
    cube([SEG_UNIT_S, SEG_THIN_W, THICKNESS]);

module corner_seg(r, unit_size) {
    translate([unit_size/2, unit_size/2])
    rotate([0,0,r*90])
    hull() {
        translate([-unit_size/6,-unit_size/6])
            cylinder(h=THICKNESS, d=unit_size*2/3);
        cube([unit_size/2, unit_size/2, THICKNESS]);
        translate([-unit_size/2, 0])
            cube([unit_size/2, unit_size/2, THICKNESS]);
        translate([0, -unit_size/2])
            cube([unit_size/2, unit_size/2, THICKNESS]);
    }
}

module big_corner_seg(rotation)
    corner_seg(rotation, SEG_UNIT_B);

module small_corner_seg(rotation)
    corner_seg(rotation, SEG_UNIT_S);

module decimal_seg() {
    translate([SEG_UNIT_B/2, SEG_UNIT_B/2])
    hull() {
        translate([-SEG_UNIT_B/6,-SEG_UNIT_B/6])
            cylinder(h=THICKNESS, d=SEG_UNIT_B*2/3);
        translate([SEG_UNIT_B/6,-SEG_UNIT_B/6])
            cylinder(h=THICKNESS, d=SEG_UNIT_B*2/3);
        translate([-SEG_UNIT_B/6, SEG_UNIT_B/6])
            cylinder(h=THICKNESS, d=SEG_UNIT_B*2/3);
        translate([SEG_UNIT_B/6, SEG_UNIT_B/6])
            cylinder(h=THICKNESS, d=SEG_UNIT_B*2/3);
    }
}

// Main
inset() { //EPD inset in bottom glass
    color(EPD_COLOR_TOP)
        cube([EPD_BG_H, EPD_BG_W, THICKNESS]);

    inset() { //segments inset in EPD
        segment(EPD_COLOR_TOP, EPD_COLOR_BOT)
            translate([EPD_EPL_OFF, EPD_EPL_OFF])
                cube([EPD_EPL_H, EPD_EPL_W, THICKNESS]);

        // byte 0 bit 0
        segment()
            translate([6.12, 3.75]) big_corner_seg(0);

        // byte 0 bit 1
        segment()
            translate([7.45, 3.75]) tall_seg();

        // byte 0 bit 2
        segment()
            translate([12.62, 3.75]) big_unit_seg();

        // byte 0 bit 3
        segment()
            translate([13.95, 3.75]) tall_seg();

        // byte 0 bit 4
        segment()
            translate([19.12, 3.75]) big_corner_seg(1);

        // byte 1 bit 0
        segment()
            translate([6.12, 6.35]) big_corner_seg(0);

        // byte 1 bit 1
        segment()
            translate([7.45, 6.35]) tall_seg();

        // byte 1 bit 2
        segment()
            translate([12.62, 6.35]) big_unit_seg();

        // byte 1 bit 3
        segment()
            translate([13.95, 6.35]) tall_seg();

        // byte 1 bit 4
        segment()
            translate([19.12, 6.35]) big_corner_seg(1);

        // byte 1 bit 5
        segment()
            translate([6.12, 7.65]) long_seg();

        // byte 1 bit 6
        segment()
            translate([12.62, 7.65]) long_seg();

        // byte 1 bit 7
        segment()
            translate([19.12, 7.65]) long_seg();

        // byte 2 bit 0
        segment()
            translate([6.12, 10.75]) big_corner_seg(3);

        // byte 2 bit 1
        segment()
            translate([7.45, 10.75]) tall_seg();

        // byte 2 bit 2
        segment()
            translate([12.62, 10.75]) big_unit_seg();

        // byte 2 bit 3
        segment()
            translate([13.95, 10.75]) tall_seg();

        // byte 2 bit 4
        segment()
            translate([19.12, 10.75]) big_corner_seg(2);

        // byte 3 bit 0
        segment()
            translate([6.12, 13.35]) big_corner_seg(0);

        // byte 3 bit 1
        segment()
            translate([7.45, 13.35]) tall_seg();

        // byte 3 bit 2
        segment()
            translate([12.62, 13.35]) big_unit_seg();

        // byte 3 bit 3
        segment()
            translate([13.95, 13.35]) tall_seg();

        // byte 3 bit 4
        segment()
            translate([19.12, 13.35]) big_corner_seg(1);

        // byte 3 bit 5
        segment()
            translate([6.12, 14.68]) long_seg();

        // byte 3 bit 6
        segment()
            translate([12.65, 14.68]) long_seg();

        // byte 3 bit 7
        segment()
            translate([19.12, 14.68]) long_seg();

        // byte 4 bit 0
        segment()
            translate([6.12, 17.8]) big_corner_seg(3);

        // byte 4 bit 1
        segment()
            translate([7.45, 17.8]) tall_seg();

        // byte 4 bit 2
        segment()
            translate([12.62, 17.8]) big_unit_seg();

        // byte 4 bit 3
        segment()
            translate([13.95, 17.8]) tall_seg();

        // byte 4 bit 4
        segment()
            translate([19.12, 17.8]) big_corner_seg(2);

        // byte 4 bit 5
        segment()
            translate([19.12, 20.13]) decimal_seg();

        // byte 5 bit 0
        segment()
            translate([25.07, 6.35]) big_corner_seg(0);

        // byte 5 bit 1
        segment()
            translate([26.4, 6.35]) tall_seg();

        // byte 5 bit 2
        segment()
            translate([31.6, 6.35]) big_unit_seg();

        // byte 5 bit 3
        segment()
            translate([32.9, 6.35]) tall_seg();

        // byte 5 bit 4
        segment()
            translate([38.07, 6.35]) big_corner_seg(1);

        // byte 5 bit 5
        segment()
            translate([25.07, 7.65]) long_seg();

        // byte 5 bit 6
        segment()
            translate([31.6, 7.65]) long_seg();

        // byte 5 bit 7
        segment()
            translate([38.07, 7.65]) long_seg();

        // byte 6 bit 0
        segment()
            translate([25.07, 10.75]) big_corner_seg(3);

        // byte 6 bit 1
        segment()
            translate([26.4, 10.75]) tall_seg();

        // byte 6 bit 2
        segment()
            translate([31.6, 10.7]) big_unit_seg();

        // byte 6 bit 3
        segment()
            translate([32.9, 10.75]) tall_seg();

        // byte 6 bit 4
        segment()
            translate([38.07, 10.75]) big_corner_seg(2);

        // byte 7 bit 0
        segment()
            translate([25.07, 13.35]) big_corner_seg(0);

        // byte 7 bit 1
        segment()
            translate([26.4, 13.35]) tall_seg();

        // byte 7 bit 2
        segment()
            translate([31.6, 13.35]) big_unit_seg();

        // byte 7 bit 3
        segment()
            translate([32.9, 13.35]) tall_seg();

        // byte 7 bit 4
        segment()
            translate([38.07, 13.35]) big_corner_seg(1);

        // byte 7 bit 5
        segment()
            translate([25.07, 14.68]) long_seg();

        // byte 7 bit 6
        segment()
            translate([31.6, 14.68]) long_seg();

        // byte 7 bit 7
        segment()
            translate([38.07, 14.68]) long_seg();

        // byte 8 bit 0
        segment()
            translate([25.07, 17.8]) big_corner_seg(3);

        // byte 8 bit 1
        segment()
            translate([26.4, 17.8]) tall_seg();

        // byte 8 bit 2
        segment()
            translate([31.6, 17.8]) big_unit_seg();

        // byte 8 bit 3
        segment()
            translate([32.9, 17.8]) tall_seg();

        // byte 8 bit 4
        segment()
            translate([38.07, 17.8]) big_corner_seg(2);

        // byte 8 bit 5
        segment()
            translate([38.07, 20.13]) decimal_seg();

        // byte 9 bit 0
        segment()
            translate([30.57, 22.46]) small_corner_seg(0);

        // byte 9 bit 1
        segment()
            translate([31.38, 22.46]) short_seg();

        // byte 9 bit 2
        segment()
            translate([34.6, 22.46]) small_unit_seg();

        // byte 9 bit 3
        segment()
            translate([35.41, 22.46]) short_seg();

        // byte 9 bit 4
        segment()
            translate([38.6, 22.46]) small_corner_seg(1);

        // byte 9 bit 5
        segment()
            translate([30.57, 23.27]) thin_seg();

        // byte 9 bit 6
        segment()
            translate([34.6, 23.27]) thin_seg();

        // byte 9 bit 7
        segment()
            translate([38.6, 23.27]) thin_seg();

        // byte 10 bit 0
        segment()
            translate([30.57, 25.2]) small_corner_seg(3);

        // byte 10 bit 1
        segment()
            translate([31.38, 25.2]) short_seg();

        // byte 10 bit 2
        segment()
            translate([34.6, 25.2]) small_unit_seg();

        // byte 10 bit 3
        segment()
            translate([35.41, 25.2]) short_seg();

        // byte 10 bit 4
        segment()
            translate([38.6, 25.2]) small_corner_seg(2);

        // byte 10 bit 5 - percent symbol
        segment()
            translate([29.22, 22.8]) rotate([0, 0, 56])
                cube([.3, 3.63, THICKNESS]);
        segment()
            translate([26.97, 22.75]) rotate([0, 0, 90])
                scale([.09, .09]) linear_extrude(THICKNESS)
                    text("O", font="DejaVu Sans Mono:style=Bold");
        segment()
            translate([29.58, 24.4]) rotate([0, 0, 90])
                scale([.09, .09]) linear_extrude(THICKNESS)
                    text("O", font="DejaVu Sans Mono:style=Bold");

        // byte 11 bit 0
        segment()
            translate([11.47, 22.46]) small_corner_seg(0);

        // byte 11 bit 1
        segment()
            translate([12.24, 22.46]) short_seg();

        // byte 11 bit 2
        segment()
            translate([15.46, 22.46]) small_unit_seg();

        // byte 11 bit 3
        segment()
            translate([16.27, 22.46]) short_seg();

        // byte 11 bit 4
        segment()
            translate([19.49, 22.46]) small_corner_seg(1);

        // byte 11 bit 5
        segment()
            translate([11.43, 23.27]) thin_seg();

        // byte 11 bit 6
        segment()
            translate([15.46, 23.27]) thin_seg();

        // byte 11 bit 7
        segment()
            translate([19.49, 23.27]) thin_seg();

        // byte 12 bit 0
        segment()
            translate([11.43, 25.2]) small_corner_seg(3);

        // byte 10 bit 1
        segment()
            translate([12.24, 25.2]) short_seg();

        // byte 10 bit 2
        segment()
            translate([15.46, 25.2]) small_unit_seg();

        // byte 10 bit 3
        segment()
            translate([16.27, 25.2]) short_seg();

        // byte 10 bit 4
        segment()
            translate([19.49, 25.2]) small_corner_seg(2);

        // byte 13 bit 0 - °C (lower bar)
        segment()
            translate([9.98, 24.23])
                cube([.5, 1, THICKNESS]);

        // byte 13 bit 1 - °F (middle bar)
        segment()
            translate([8.75, 24.23])
                cube([.5, 1, THICKNESS]);

        // byte 13 bit 2 - °C/F (common)
        segment()
            translate([8.75, 22.52]) rotate([0, 0, 90])
                scale([.16, .16]) linear_extrude(THICKNESS)
                    text("°", font="DejaVu Sans Mono:style=Regular");
        segment()
            translate([7.53, 24.23])
                cube([.5, 1, THICKNESS]);
        segment()
            translate([7.53, 23.7])
                linear_extrude(THICKNESS)
                    polygon([[0,.5], [.1,.33], [.33,.1], [.5,0],
                        [2.45,0], [2.62,.1], [2.85,.33], [2.95,.5]]);

        // byte 13 bit 3 - bluetooth
        segment()
            translate([5.19, 20.54]) rotate([0, 0, 90])
            difference() {
                linear_extrude(THICKNESS, convexity=4)
                    polygon(
                        //points
                        [
                            //bottom hole
                            [1.15,.35],[1.4,.6],[1.15,.85],
                            //top hole
                            [1.15,1.25],[1.4,1.5],[1.15,1.75],
                            // outer shape
                            [1,0],[1.6,.6],[1.15,1.05],[1.6,1.5],[1,2.1],[1,1.25],
                            [.6,1.65],[.5,1.55],[1,1.05],[.5,.55],[.6,.45],[1,.85]
                        ],
                        //paths
                        [
                            [6,7,8,9,10,11,12,13,14,15,16,17], //outer shape
                            if(!FREECAD) [0,1,2], // bottom hole
                            if(!FREECAD) [3,4,5] // top hole
                        ],
                        convexity=4
                    );
                if(FREECAD) linear_extrude(THICKNESS*5, center=true,convexity=4)
                    polygon(
                        //points
                        [
                            //bottom hole
                            [1.15,.35],[1.4,.6],[1.15,.85],
                            //top hole
                            [1.15,1.25],[1.4,1.5],[1.15,1.75],
                            // outer shape
                            [1,0],[1.6,.6],[1.15,1.05],[1.6,1.5],[1,2.1],[1,1.25],
                            [.6,1.65],[.5,1.55],[1,1.05],[.5,.55],[.6,.45],[1,.85]
                        ],
                        //paths
                        [
                            [0,1,2], // bottom hole
                        ],
                        convexity=4
                    );
                if(FREECAD) linear_extrude(THICKNESS*5, center=true,convexity=4)
                    polygon(
                        //points
                        [
                            //bottom hole
                            [1.15,.35],[1.4,.6],[1.15,.85],
                            //top hole
                            [1.15,1.25],[1.4,1.5],[1.15,1.75],
                            // outer shape
                            [1,0],[1.6,.6],[1.15,1.05],[1.6,1.5],[1,2.1],[1,1.25],
                            [.6,1.65],[.5,1.55],[1,1.05],[.5,.55],[.6,.45],[1,.85]
                        ],
                        //paths
                        [
                            [3,4,5] // top hole
                        ],
                        convexity=4
                    );
            }

        // byte 13 bit 4 - low battery
        segment()
            translate([4.85, 26.1]) rotate([0, 0, 180])
            difference() {
                linear_extrude(THICKNESS, convexity=4)
                    polygon(
                        //points
                        [
                            //bottom hole
                            [1.05,.15],[1.15,.2],[1.2,.3],
                            [1.2,2.35],[1.15,2.45],[1.05,2.5],
                            [.3,2.5],[.2,2.45],[.15,2.35],
                            [.15,.3],[.2,.2],[.3,.15],
                            //top hole
                            [.45,2.65],[.9,2.65],
                            [.9,2.8],[.88,2.83],[.85,2.85],
                            [.5,2.85],[.47,2.83],[.45,2.8],
                            // outer shape
                            [1.05,0],[1.25,.1],[1.35,.3],
                            [1.35,2.35],[1.25,2.55],[1.05,2.65],
                            [1.05,2.85],[1,2.95],[.95,3],
                            [.4,3],[.35,2.95],[.3,2.8],
                            [.3,2.65],[.1,2.55],[0,2.35],
                            [0,.3],[.1,.1],[.3,0]
                        ],
                        //paths
                        [
                            [20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37], //outer shape
                            if(!FREECAD) [0,1,2,3,4,5,6,7,8,9,10,11], // bottom hole
                            if(!FREECAD) [12,13,14,15,16,17,18,19] // top hole
                        ],
                        convexity=4
                    );
                if(FREECAD) linear_extrude(THICKNESS*5,center=true,convexity=4)
                    polygon(
                        //points
                        [
                            //bottom hole
                            [1.05,.15],[1.15,.2],[1.2,.3],
                            [1.2,2.35],[1.15,2.45],[1.05,2.5],
                            [.3,2.5],[.2,2.45],[.15,2.35],
                            [.15,.3],[.2,.2],[.3,.15],
                            //top hole
                            [.45,2.65],[.9,2.65],
                            [.9,2.8],[.88,2.83],[.85,2.85],
                            [.5,2.85],[.47,2.83],[.45,2.8],
                            // outer shape
                            [1.05,0],[1.25,.1],[1.35,.3],
                            [1.35,2.35],[1.25,2.55],[1.05,2.65],
                            [1.05,2.85],[1,2.95],[.95,3],
                            [.4,3],[.35,2.95],[.3,2.8],
                            [.3,2.65],[.1,2.55],[0,2.35],
                            [0,.3],[.1,.1],[.3,0]
                        ],
                        //paths
                        [
                            [0,1,2,3,4,5,6,7,8,9,10,11], // bottom hole
                        ],
                        convexity=4
                    );
                if(FREECAD) linear_extrude(THICKNESS*5,center=true,convexity=4)
                    polygon(
                        //points
                        [
                            //bottom hole
                            [1.05,.15],[1.15,.2],[1.2,.3],
                            [1.2,2.35],[1.15,2.45],[1.05,2.5],
                            [.3,2.5],[.2,2.45],[.15,2.35],
                            [.15,.3],[.2,.2],[.3,.15],
                            //top hole
                            [.45,2.65],[.9,2.65],
                            [.9,2.8],[.88,2.83],[.85,2.85],
                            [.5,2.85],[.47,2.83],[.45,2.8],
                            // outer shape
                            [1.05,0],[1.25,.1],[1.35,.3],
                            [1.35,2.35],[1.25,2.55],[1.05,2.65],
                            [1.05,2.85],[1,2.95],[.95,3],
                            [.4,3],[.35,2.95],[.3,2.8],
                            [.3,2.65],[.1,2.55],[0,2.35],
                            [0,.3],[.1,.1],[.3,0]
                        ],
                        //paths
                        [
                            [12,13,14,15,16,17,18,19] // top hole
                        ],
                        convexity=4
                    );
            }
    }
}

// flat-flex cable
translate([EPD_BG_H, EPD_FPC_OFF_L, EPD_FPC_OFF_H]) {
    color(FPC_COLOR) {
        translate([FPC_FOLD_DIST, 0, -FPC_FOLD_THICKNESS/2]) rotate([-90,0,0]) scale([.8,1,1])
            difference() {
                cylinder(h=FPC_W1, r=FPC_T+FPC_FOLD_THICKNESS/2);
                cylinder(h=FPC_W1*3, r=FPC_FOLD_THICKNESS/2, center=true);
                translate([-FPC_FOLD_THICKNESS*2, -FPC_FOLD_THICKNESS, -.1])
                    cube([FPC_FOLD_THICKNESS*2, FPC_FOLD_THICKNESS*2, FPC_W1+.2]);
            }
        cube([FPC_FOLD_DIST, FPC_W1, FPC_T]);
        translate([PI*FPC_FOLD_THICKNESS/2+2*FPC_FOLD_DIST, 0, -FPC_FOLD_THICKNESS])
            rotate([0, 180, 0]) {
                translate([FPC_FOLD_DIST+PI*FPC_FOLD_THICKNESS/2, 0])
                    cube([FPC_H-FPC_FOLD_DIST-PI*FPC_FOLD_THICKNESS/2-FPC_W2, FPC_W1, FPC_T]);
                translate([FPC_H-FPC_W2, -FPC_L+FPC_W1])
                    difference() {
                        cube([FPC_W2, FPC_L, FPC_T]);
                        translate([FPC_W2/3, FPC_L-FPC_W2*2/3, -FPC_T]) difference() {
                            cube([FPC_W2+.1, FPC_W2+.1, FPC_T*3]);
                            cylinder(h=8*FPC_T, r=FPC_W2*2/3, center=true);
                        }
                    }
                translate([FPC_H-FPC_W2-FPC_W2/3, -FPC_W2/3])
                    difference() {
                        cube([FPC_W2, FPC_W2, FPC_T]);
                        cylinder(h=3*FPC_T, r=FPC_W2/3, center=true);
                    }
            }
    }
    color(FPC_CON_COLOR)
        translate([-FPC_H+PI*FPC_FOLD_THICKNESS/2+2*FPC_FOLD_DIST, -FPC_L+FPC_W1, -FPC_FOLD_THICKNESS-CON_T])
            cube([FPC_W2, CON_L, CON_T-FPC_T]);
}
