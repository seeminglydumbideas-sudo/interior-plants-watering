use <sub_hw-316_4relays.scad>
use <sub_lilygo_t_display_s3.scad>
use <usb-debug-pcb.scad>

/*[ Rendering ]*/
//poly quality
$fn=100; // [100:high,6:low]

// Cross-section cut X (viewing only)
_cut_x = -60; // [-60:0.5:60]
// Cross-section cut Y (viewing only)
_cut_y = -60; // [-60:1:60]
// Cross-section cut Z (viewing only)
_cut_z = -60; // [-60:1:60]
// Opening: distance to separate top/bottom for viewing
_top_open = 20; // [0:1:80]
// Gap between pieces when laid out for print
_print_gap = 20; // [0:1:60]

// Top
_top=true;
// Bottom
_bottom=true;
// Model up (reference components housed in the top: ESP + USB debug pcb)
_model_up=true;
// Model down (reference components housed in the bottom: relay module)
_model_down=true;
// Print layout (lay both halves flat, ready to slice)
_print=false;

/*[ Display clip ]*/
// how far the barb reaches inward over the pcb edge. Post sits 1.65mm
// outside the pcb's y edge at y=+-12.85, but the display_hole cut already
// removes material starting at y=+-12.5, so there's only ~0.35mm of
// window to reach into before this starts getting sliced by that cut.
// Keep this between 0.15 (barely clears the hole) and 0.5 (barely reaches
// the pcb edge); 0.3 splits the difference but is a thin, unverified
// margin -- retune against the real board/print.
_dclip_protrusion = 1; // [0:0.05:0.5]
// height of the lead-in ramp (insertion side, cammed over on the way up)
_dclip_ramp_h = 1.0; // [0.2:0.1:3]
// height of the steep catch face (retention side, holds the pcb once past)
_dclip_catch_h = 0.4; // [0.1:0.1:2]
// z where the ramp starts (just above the post tip)
_dclip_base_z = 10; // [8:0.1:16]


// Barb on one display-corner post so the display pcb clips past it when
// pushed up into place instead of just resting loose on the posts.
// x,y: same corner position as the post. dir: +1 or -1, which way (y) the
// barb reaches inward over the pcb -- the post sits just outside the pcb's
// y edge (post y=+-14.5 vs pcb edge y=+-12.85), so the barb is attached to
// the post's y-facing side, not its x-facing side.
// Profile (in y-z, extruded along the post's x width): flush at the base,
// ramps inward to its max protrusion, then drops back to flush over a much
// shorter height -- a shallow lead-in ramp on the way up, a steep catch
// face once past. Assumes the bare pcb (61x25.7x1.2mm, see
// esp32_lilygo_t_display_s3()) sits with its bottom face around z=11.4 when
// the top is fully assembled (_top_open=0) -- verify against the real
// board/print and retune the _dclip_* params above if the fit is off.
module display_clip(x,y,dir) {
  translate([x, y + dir*1.5, _dclip_base_z])
    rotate([0,0,90])
      rotate([90,0,0])
        linear_extrude(height=3, center=true)
          polygon(points=[
            [0,0],
            [dir*_dclip_protrusion, _dclip_ramp_h],
            [0, _dclip_ramp_h+_dclip_catch_h]
          ]);
}


module box_bottom(main_pos,main_vol,hcut,insert_width,insert_height,insert_tol) {
  //https://www.handsontec.com/dataspecs/4Ch-relay.pdf
  holes=[69.6,49.4,3];
  // compute bottom H+Z
  h=hcut*main_vol[2];
  z=(main_pos[2]-main_vol[2]+h)/2;
  difference() {
    union() {
      difference() {
      // bottom + insert
       union() {
         // bottom volume
         translate([main_pos[0],main_pos[1],main_pos[2]+z]){
           cube([main_vol[0],main_vol[1],h],center=true);
         };
         // insert
         iz=z+h/2+insert_height/2-insert_tol;
         translate([main_pos[0],main_pos[1],iz]){
           cube([
             main_vol[0]-2*insert_width-2*insert_tol,
             main_vol[1]-2*insert_width-2*insert_tol,
             insert_height],
             center=true);
          };
        };
        // big hole for PCB (x widened +4.9 to 94.9, matches box_top's
        // large hole -- see main_vol/main_pos comment in main.scad)
        translate([main_pos[0],main_pos[1],main_pos[2]]){
            cube([
                94.9,
                57,
                main_vol[2]-5
            ],center=true);
        };
      };
      // support for replay PCB. The narrower r=1.3 peg (poking up above
      // the r=2 shelf, into the pcb's mounting hole) was shortened
      // (z=-14->-14.5, h=2.9->1.9) to make room for box_top's new
      // locating peg reaching down into the same hole from above --
      // its top is now flush with the shelf (z=-13.55) instead of
      // poking up to z=-12.55, leaving ~0.13mm clearance to that peg.
      translate([holes[0]/2,-holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([holes[0]/2,-holes[1]/2,-14.5])
        cylinder(h=1.9,r=1.3, center=true);

      translate([-holes[0]/2,-holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([-holes[0]/2,-holes[1]/2,-14.5])
        cylinder(h=1.9,r=1.3, center=true);

      translate([-holes[0]/2,holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([-holes[0]/2,holes[1]/2,-14.5])
        cylinder(h=1.9,r=1.3, center=true);

      translate([holes[0]/2,holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([holes[0]/2,holes[1]/2,-14.5])
        cylinder(h=1.9,r=1.3, center=true);
    }

    // screw holes
    translate([-holes[0]/2,holes[1]/2,-15])
      cylinder(h=10,r=1.4,center=true);
    translate([-holes[0]/2,-holes[1]/2,-15])
      cylinder(h=10,r=1.4,center=true);
    translate([holes[0]/2,-holes[1]/2,-15])
      cylinder(h=10,r=1.4,center=true);
    translate([holes[0]/2,holes[1]/2,-15])
      cylinder(h=10,r=1.4,center=true);

    translate([-holes[0]/2,holes[1]/2,-17.9])
      cylinder(h=1.5,r=2.2,center=true);
    translate([-holes[0]/2,-holes[1]/2,-17.9])
      cylinder(h=1.5,r=2.2,center=true);
    translate([holes[0]/2,-holes[1]/2,-17.9])
      cylinder(h=1.5,r=2.2,center=true);
    translate([holes[0]/2,holes[1]/2,-17.9])
      cylinder(h=1.5,r=2.2,center=true);

    // pump cable holes: 4x d=3.6mm through the -y wall, for the relay
    // outputs' cables running out to the pumps. Evenly spaced across
    // the relay board's own width (66.5mm -- see the "relays" cube in
    // sub_hw-316_4relays.scad); the model doesn't capture individual
    // terminal positions, so check against the real board and retune
    // x/z if the terminals land somewhere else.
    // z raised +15 (-14 -> 1) since the original position was too low;
    // needed hcut raised (0.2 -> 0.6, see main_pos/main_vol/hcut
    // comment) so box_bottom's wall is tall enough to still be solid
    // there (new ceiling ~4.25, ~3.25mm above this hole).
    translate([-24.9,-31,1])
      rotate([90,0,0])
        cylinder(h=10,d=3.6,center=true);
    translate([-8.3,-31,1])
      rotate([90,0,0])
        cylinder(h=10,d=3.6,center=true);
    translate([8.3,-31,1])
      rotate([90,0,0])
        cylinder(h=10,d=3.6,center=true);
    translate([24.9,-31,1])
      rotate([90,0,0])
        cylinder(h=10,d=3.6,center=true);

  }
}


module box_top_buttons(main_pos,main_vol,hcut,insert_width,insert_height,insert_tol) {
  // buttons
  // base (the large part) made 0.5mm thinner (2.6 -> 2.1) since it was
  // pressing too hard against the pcb's own button. Kept its top face
  // fixed at z=17.7 (where it merges with the stem) and shortened it
  // from the bottom instead, so the face that contacts the pcb button
  // sits 0.5mm higher (z=15.1 -> 15.6) -- less preload.
  // The stem originally shared that same bottom face (z=15.1) -- left
  // alone, its narrower tip would now poke out below the base as the
  // new (and only, and much smaller) contact point, which isn't what
  // "thinner" should do. Shortened it by the same 0.5mm (keeping its
  // top fixed, where it pokes out through the case) so both stay flush.
  translate([18,9,17.85])
    cube([2.6,2.6,4.5],center=true);
  translate([18.7,9,16.65])
    cube([4,6,2.1],center=true);

  translate([18,-9,17.85])
    cube([2.6,2.6,4.5],center=true);
  translate([18.7,-9,16.65])
    cube([4,6,2.1],center=true);

}


module box_top(main_pos,main_vol,hcut,insert_width,insert_height,insert_tol) {
 // compute bottom H+Z
 h=(1-hcut)*main_vol[2];
 z=(main_vol[2]-h)/2;
 holes=[69.6,49.4,3];

 difference() {
   union() {
     // top volume - big-hole
     difference() {
       // top volume
       translate([main_pos[0],main_pos[1],main_pos[2]+z]){
         cube([main_vol[0],main_vol[1],h],center=true);
       };
       // insert
       iz=-h/2+z+insert_height/2;
       translate([main_pos[0],main_pos[1],main_pos[2]+iz]){
         cube([
           main_vol[0]-2*insert_width+2*insert_tol,
           main_vol[1]-2*insert_width+2*insert_tol,
           insert_height+insert_tol],
           center=true);
       };
       // large hole (x widened +4.9 to 94.9, see main_vol/main_pos comment
       // below -- keeps this cavity's left edge anchored while the box
       // grows to the right)
       translate([main_pos[0],main_pos[1],main_pos[2]])
          cube([94.9,57,main_vol[2]-3],center=true);
       // USB hole
       translate([main_pos[0]+main_vol[0]/2,main_pos[1],main_pos[2]+13.9])
          cube([20,9.5,3.5],center=true);

      };
      // display border
      translate([-12.4,0,17]) {
        difference() {
          cube([58,28,2],center=true);
          cube([56.6,26,3],center=true);
        };
      };
      // relays pads
      translate([holes[0]/2,-holes[1]/2,3.08])
        cube([4,4,31],center=true);
      translate([holes[0]/2,+holes[1]/2,3.08])
        cube([4,4,31],center=true);
      translate([-holes[0]/2,-holes[1]/2,3.08])
        cube([4,4,31],center=true);
      translate([-holes[0]/2,+holes[1]/2,3.08])
        cube([4,4,31],center=true);
      // relay pcb locating pegs: the pad used to end flat (z=-12.42)
      // with just the axial screw hole through it -- added a peg sized
      // to the relay pcb's own 3mm mounting holes (holes[2]) so it
      // clips/registers the pcb when lowered into place. box_bottom's
      // matching peg (support for replay PCB) was shortened to make
      // room -- its top is now flush with its shelf (z=-13.55), leaving
      // ~0.13mm of clearance to this one's tip (z=-13.42). Still worth
      // checking the fit on a print.
      translate([holes[0]/2,-holes[1]/2,-12.92])
        cylinder(h=1,d=holes[2],center=true);
      translate([holes[0]/2,+holes[1]/2,-12.92])
        cylinder(h=1,d=holes[2],center=true);
      translate([-holes[0]/2,-holes[1]/2,-12.92])
        cylinder(h=1,d=holes[2],center=true);
      translate([-holes[0]/2,+holes[1]/2,-12.92])
        cylinder(h=1,d=holes[2],center=true);
      // relay pad stiffener ribs: the pads are long (31mm), thin
      // (4x4mm) cantilevers hanging from the ceiling and flex easily.
      // Widening them to reach the nearest wall (y=+-33, only 8.3mm
      // away from holes[1]/2=24.7) over their upper half roughly halves
      // the effective unsupported length, which is what actually
      // matters for cantilever deflection (~length^3).
      // Height increased +20% (top stays anchored at the pad's own top,
      // 18.58, fused into the ceiling -- the extra reach comes off the
      // bottom instead, extending further down the pad toward the pcb).
      rib_top=18.58;
      rib_h=(18.58-3.08)*1.2;
      rib_z=rib_top-rib_h/2;
      translate([holes[0]/2,(holes[1]/2+main_vol[1]/2)/2,rib_z])
        cube([4,main_vol[1]/2-holes[1]/2,rib_h],center=true);
      translate([holes[0]/2,-(holes[1]/2+main_vol[1]/2)/2,rib_z])
        cube([4,main_vol[1]/2-holes[1]/2,rib_h],center=true);
      translate([-holes[0]/2,(holes[1]/2+main_vol[1]/2)/2,rib_z])
        cube([4,main_vol[1]/2-holes[1]/2,rib_h],center=true);
      translate([-holes[0]/2,-(holes[1]/2+main_vol[1]/2)/2,rib_z])
        cube([4,main_vol[1]/2-holes[1]/2,rib_h],center=true);
      // pcb-usb pads (x shifted +4.7 to match the corrected usb_debug_pcb
      // position -- see render_model()'s "usb" comment)
      translate([45.2,14,16.55])
        cube([6,6,4],center=true);
      translate([30.2,14,16.55])
        cube([6,6,4],center=true);
      translate([45.2,-14,16.55])
        cube([6,6,4],center=true);
      translate([30.2,-14,16.55])
        cube([6,6,4],center=true);
      // display pads (bottom reaches down to back up the retention barb --
      // its ramp starts at _dclip_base_z, so the post has to reach at
      // least that low, plus a small margin so it isn't a knife edge)
      dpad_top=18.7;
      dpad_bottom=min(11.3, _dclip_base_z-0.2);
      dpad_h=dpad_top-dpad_bottom;
      dpad_z=(dpad_top+dpad_bottom)/2;
      // (9,-14.5) corner moved to (14,-14.5). The reset button is on the
      // pcb's +y edge locally, but render_model()'s rotate([0,0,180])
      // flips y, so in case coordinates it lands at y=-12.85 -- this is
      // the corner that collides with it now (reset sits around
      // x=6.5-10.5, y=-13.85..-10.85)
      translate([9,14.5,dpad_z])
        cube([3,3,dpad_h],center=true);
      translate([-30,14.5,dpad_z])
        cube([3,3,dpad_h],center=true);
      translate([14,-14.5,dpad_z])
        cube([3,3,dpad_h],center=true);
      translate([-30,-14.5,dpad_z])
        cube([3,3,dpad_h],center=true);
      // display retention clips (pcb snaps past these when pushed up)
      display_clip(9,14.5,-1);
      display_clip(-30,14.5,-1);
      display_clip(14,-14.5,1);
      display_clip(-30,-14.5,1);
      // reset pinhole guide: the pinhole below just punches through the
      // wall, leaving the pin to cross open cavity on its own to find
      // the button -- hard to aim. This bridges that gap (wall at
      // y=-33 to 1mm short of the button), same x/z as the pinhole,
      // which cuts straight through it below.
      // The button's own footprint is y=-13.85..-10.85 (rst cube +
      // its body cube, see sub_lilygo_t_display_s3.scad) -- nearest
      // edge to the wall is -13.85, so the guide stops at -14.85 to
      // keep a clear 1mm margin (it previously ended at -12, which
      // was actually 1.85mm into the button's own footprint).
      // Extended up to z=19.3 -- flush with the shell's own top face,
      // not past it (this is a plain union, so anything added beyond
      // 19.3 would poke out of the case, not get clipped by the outer
      // shell) -- so it's a rib fused to both the wall (y=-33) and the
      // ceiling above, instead of a cantilever hanging off the wall
      // alone with its far end floating in open cavity. Avoids needing
      // print support under it.
      translate([8.5,-23.925,15.7])
        cube([3,18.15,7.2],center=true);
    };
    // display hole
    translate([-13.7,0,16])
      cube([44.8,25,10],center=true);
    // button holes
    translate([18,9,11])
      cube([3,3,140],center=true);
    translate([18,-9,11])
      cube([3,3,140],center=true);
    // pinhole : reset button access. The reset switch is side-actuated
    // (its actuator sticks out toward -y in case coordinates, at
    // y=-12.85, not upward), so the pin has to come in horizontally
    // along y, from the outer wall (y=-33) through the open cavity to
    // the button -- not straight down through the top. Same size
    // convention as the sensor case's boot-button pinhole (d=1), just
    // longer to make that reach.
    translate([8.5,-23,13.6])
      rotate([90,0,0])
        cylinder(h=26,d=1,center=true);
    // screw holes
    translate([-holes[0]/2,holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);
    translate([-holes[0]/2,-holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);
    translate([holes[0]/2,-holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);
    translate([holes[0]/2,holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);
    // screw pcb-usb pads (x shifted +4.7, matches the pcb-usb pads above)
    translate([45.2,13.7,13])
      cylinder(h=10,d=2,center=true);
    translate([30.2,13.7,13])
      cylinder(h=10,d=2,center=true);
    translate([45.2,-13.7,13])
      cylinder(h=10,d=2,center=true);
    translate([30.2,-13.7,13])
      cylinder(h=10,d=2,center=true);
  };
}

module render_model() {
  // models
  translate([0,0,-13]) {
    if (_model_up) {
      // ESP
      translate([-10,0,25+_top_open])
        color("#30AA30")
         rotate([0,0,180]) {
           esp32_lilygo_t_display_s3();
           //esp32_lilygo_t_display_s3_view();
         }
      // usb
      // Measured on the actual print/assembly: once the usb-debug
      // module's connector is fully plugged into the esp's usb port,
      // there's a 7.5mm gap between the two pcbs' facing edges. ESP pcb's
      // facing edge sits at global x=20.5 (fixed, from its own
      // dimensions), and usb_debug_pcb()'s facing edge is at local
      // x=-10.2, so this translate is 20.5+7.5-(-10.2)=38.2 (was
      // 26+7.5=33.5, short by 4.7mm).
      translate([38.2,0,27.2+_top_open])
        color("#AAAA30")
          usb_debug_pcb();
    }
    if (_model_down) {
      // 4 relays module
      translate([0,0,-_top_open])
      color("#30AAAA")
        hw_316();
    }
  }
}

module render_box() {
  // top
  if (_top) {
    translate([0,0,_top_open])
    color("#AAAAFF")
    box_top(main_pos, main_vol,hcut,insert_width,insert_height,insert_tol);
    // buttons
    translate([0,0,_top_open])
    color("#FFFF00")
    box_top_buttons();
  }
  // bottom
  if (_bottom)
  translate([0,0,-_top_open])
  color("#AA30FF")
  box_bottom(main_pos, main_vol,hcut,insert_width,insert_height,insert_tol);
}


module render_model_cut() {
    // model
    if (_model_up || _model_down)
    difference() {
      render_model();
      translate([-50+_cut_x,0,0])
        cube([120,120,120],center=true);
      translate([0,0,50-_cut_z])
        cube([120,120,120],center=true);
      translate([0,50-_cut_y,0])
        cube([120,120,120],center=true);
    }
    // box
    if (_top || _bottom)
    difference() {
      render_box();
      translate([-50+_cut_x,0,0])
        cube([120,120,120],center=true);
      translate([0,0,50-_cut_z])
        cube([120,120,120],center=true);
      translate([0,50-_cut_y,0])
        cube([120,120,120],center=true);
    }

}

// ================
// PRINT
// ================
// Lays both halves (plus the buttons) flat on the print bed, separately.
// bottom: already modeled floor-down / standoffs-up -> no rotation needed.
// top: flipped 180 deg so its flat outer face (display/button/usb side) sits
//      on the bed and the open cavity faces up, matching how enclosure lids
//      are normally printed (best bed adhesion, no overhangs on that face).
// buttons: printed as their own separate part (not attached to the top
//      shell). box_top_buttons()'s own local geometry already has the
//      wide base and the narrow stem flush at the bottom (both z=15.6,
//      stem continuing up to z=20.1) -- no rotation needed, just lift
//      it onto the bed.
// NOTE: this orientation is a best-guess default, not verified against an
// actual slice/print (no OpenSCAD CLI available in this environment) --
// please sanity-check it in a slicer before printing.
module view_print(pos,vol,hcut,insert_width,insert_height,insert_tol,gap) {
  bottom_lift = vol[2]/2 - pos[2];
  top_lift = pos[2] + vol[2]/2;
  // box_top_buttons()'s combined bounding z-min (stem+base both flush
  // at z=15.6) -- lift by this so it lands right on the bed, wide-side
  // down.
  btn_lift = -15.6;

  translate([0, -vol[1]/2 - gap/2, bottom_lift])
    color("#AA30FF")
      box_bottom(pos,vol,hcut,insert_width,insert_height,insert_tol);

  translate([0, vol[1]/2 + gap/2, top_lift])
    rotate([0,180,0])
      color("#AAAAFF")
        box_top(pos,vol,hcut,insert_width,insert_height,insert_tol);

  translate([vol[0]/2 + gap, 0, btn_lift])
    color("#FFFF00")
      box_top_buttons();
}

// ================
// MAIN
// ================

// volumes
// main_vol[0]/main_pos[0] widened +4.9mm (100->104.9, 4->6.45) so the +x
// wall reaches the usb-debug module's connector (moved +4.7mm earlier,
// see render_model()'s "usb" comment), flush instead of protruding past
// it. Growing vol by X while shifting pos by X/2 keeps the -x wall fixed
// at its old position (-46) -- everything on that side (esp's far edge
// only has ~0.5mm clearance there) is untouched; only the +x wall moves.
// The "large hole"/"big hole for pcb" cavities in box_top/box_bottom are
// widened by the same +4.9 (90->94.9) so their left edge stays anchored
// too, instead of encroaching on that same clearance.
main_pos=[6.45,0,0.3];
main_vol=[104.9,66,38];
// hcut raised 0.2 -> 0.6 to move the top/bottom split line up, so
// box_bottom's own wall is tall enough to reach the pump cable holes
// (moved to z=1, see box_bottom's "pump cable holes" comment). This
// only trims material off box_top's own outer wall below the new split
// (z=4.25) -- everything box_top actually places up there (display,
// buttons, usb, the reset guide) sits well above that, and the relay
// pads/pegs/ribs/screw-hole are separate additions positioned by
// absolute z, unaffected by hcut either way. box_top's own ceiling
// (z=19.3, fixed) and the "insert" mating lip in both halves track the
// new split line automatically since they're computed from h/hcut, not
// hardcoded.
hcut=0.6;
insert_width=3;
insert_height=6;
insert_tol=0.05;


if (_top || _bottom || _model_up || _model_down)
  render_model_cut();

if (_print)
  view_print(main_pos, main_vol, hcut, insert_width, insert_height, insert_tol, _print_gap);
