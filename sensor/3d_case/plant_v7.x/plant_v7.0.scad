use <MCAD/boxes.scad>
use <model_plant_sensor.scad>
use <model_plant_holes.scad>

/*[ Rendering ]*/
//poly quality
$fn=100; // [100:high,6:low]

/*[ Battery Type ]*/
//Type of battery (lenght)
_batLg=38; // [38:600mAh (small),63:1200mAh (large)]


// ================
// VOL
// ================
module right_insert(main_pos,main_vol,edge_depth,edge_width,tol=0) {
 translate([
    main_pos[0],
    main_pos[1],
    main_pos[2]]){
  cube([
    edge_depth*2,
    main_vol[1]-2*edge_width-2*tol,
    main_vol[2]-2*edge_width-2*tol],center=true);
 }
}

module volume_right(main_pos,main_vol,edge_depth,edge_width,tol=0) {
 // main volume
 difference() {
  translate(main_pos){
   cube(main_vol,center=true);
  };
  // remove left side
  translate([main_pos[0]+100,0,0]){
   cube([200,200,200],center=true);
  };
 }
 // "right insert": the part that is smaller 
 // and go inside case_left
 right_insert(main_pos,main_vol,edge_depth,edge_width,tol); 
}

module volume_left(main_pos,main_vol,edge_depth,edge_width) {
 // main volume
 difference() {
  translate(main_pos){
   cube(main_vol,center=true);
  };
  // remove left side
  translate([main_pos[0]-100,0,0]){
   cube([200,200,100],center=true);
  };
  // make place for the "right insert" () see case_right()
  right_insert(main_pos,main_vol,edge_depth, edge_width, 0);
 }
}



// ================
// CASE
// ================

module hole_screw_m2() {
  // hole for thread
  //translate([-2,7,-3])
  translate([0,0,-3.6])
   cylinder(h=8,r=1.7/2,center=true);
  // hole no thread
  translate([0,0,-8.5]) 
   cylinder(h=2,r=2.2/2,center=true);
  // hole for head
  translate([0,0,-12.5]) 
   cylinder(h=8,r=3.4/2,center=true);
}

SCREW_Z=4;

module case_left(main_pos,main_vol,edge_depth,edge_width,battery_length) {    
 // volume - hole
 difference() {    
    volume_left(main_pos,main_vol,edge_depth,edge_width);
    hole_model(battery_length);
 } 
 // things to hold sensor
 difference() {
  translate([11.5,-17,-0.5])
   cylinder(h=2,r=2,center=true);
  translate([14,-17,0])
   cube([5,5,5], center=true);
 }
 // long stuff for screw - screw hole
 translate([0.6,7,SCREW_Z])
   rotate([0,90,0])
     difference() {
       cylinder(h=15,r=1.8,center=true);
       hole_screw_m2();
     }         
}

module case_right(main_pos,main_vol,edge_depth,edge_width,edge_tol,battery_length) {
 // volume-holes-hole_screw    
 difference() {
    volume_right(main_pos,main_vol,edge_depth,edge_width,edge_tol);
    hole_model(battery_length);
    translate([0.6,7,SCREW_Z])
     rotate([0,90,0])
       hole_screw_m2();
 } 
 // things to hold sensor
 difference() {
  translate([-11.5,-17,-0.5])
   cylinder(h=2,r=1.8,center=true);
  translate([-14,-17,0])
   cube([5,5,5], center=true);
 }
 // hole for screw (with head stop)
 difference() { 
  translate([-8,7,SCREW_Z])
   rotate([0,90,0])
     cylinder(h=8,r=3,center=true);
   //cube([6,5.8,5.5],center=true);   
   // remove hole for screw
   translate([0.6,7,SCREW_Z])
     rotate([0,90,0])
       hole_screw_m2();
   // remove insert for long stuf
   translate([0.5,7,SCREW_Z])
     rotate([0,90,0])
      cylinder(h=15,r=1.9,center=true);
 }

}
// ================
// DEBUG
// ================
module view_left_case(batLg) {
 color("#c0c0c0")
  model(batLg);
 case_left(_pos,_vol,_edge_depth,_edge_width,batLg);    
}

module view_right_case(batLg) {
 color("#c0c0c0")
  model(batLg);
 case_right(_pos,_vol,_edge_depth,_edge_width,_edge_tol,batLg);  
}

module view_both_cases(pos,vol,ed,ew,et,bl,space,space=20) {
 translate([-space,0,0])
  color("#a060a0",1)
   case_right(pos,vol,ed,ew,et,bl);    
 translate([space,0,0])
  color("#60a0a0",1)
   case_left(pos,vol,ed,ew,bl);
}
    
module view_up(pos,vol,ed,ew,et,bl,space,cut) {
 difference() {
  view_both_cases(pos,vol,ed,ew,et,bl,space);
  translate([-150,-150,cut]) 
   cube([300,300,300]);
 }
}

module view_print(pos,vol,ed,ew,et,bl,space) {
 rotate([0,-90,0])
  translate([10,0,space])
   color("#a060a0",1)
    case_right(pos,vol,ed,ew,et,bl);    
 rotate([0,90,0])
  translate([-10,0,space])
    color("#60a0a0",1)
     case_left(pos,vol,ed,ew,bl);
}

// ================
// MAIN
// ================



_batLg=38;// short battery
//_batLg=63;// long battery

_pos=[0 , -6+_batLg/2, 6];
_vol=[25, 35+_batLg,  18];
_edge_depth=3.5;
_edge_width=0.7;
_edge_tol=0.1;

//volume_right(_pos,_vol,_edge_depth,_edge_width,_edge_tol); // DEBUG insert
//volume_left(_pos,_vol,_edge_depth,_edge_width); // DEBUG insert

//view_both_cases(_pos,_vol,_edge_depth,_edge_width,_edge_tol,_batLg, 20);
//view_up(_pos,_vol,_edge_depth,_edge_width,_edge_tol,_batLg, 0, SCREW_Z); // focus: screw
//view_up(_pos,_vol,_edge_depth,_edge_width,_edge_tol,_batLg, 0, 11); // focus: ??
//view_up(_pos,_vol,_edge_depth,_edge_width,_edge_tol,_batLg, 0, 0); // focus: sensor
//color("#a09090",0.5)
//  model(_batLg);

// short battery
view_print(_pos,_vol,_edge_depth,_edge_width,_edge_tol,_batLg, 10);// original
//view_print(_pos,inc(_vol,2),_edge_depth,_edge_width+0.5,_edge_tol,_batLg, 10); // fat
//view_print(_pos,inc(_vol,-1),_edge_depth-1.5,_edge_width-0.3,_edge_tol,_batLg, 10);// thin

/*_batLg=63;// long battery
_pos=[0 , -6+_batLg/2, 6];
_vol=[25, 35+_batLg,  18];
view_print(_pos,_vol,_edge_depth,_edge_width,_edge_tol,63, 10);// large battery */
 