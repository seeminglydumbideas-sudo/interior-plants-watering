use <sub_hw-316_4relays.scad>
use <sub_lilygo_t_display_s3.scad>
use <usb-debug-pcb.scad>

$fn=100;
// slider widget for number in range
cut_x = -60; // [-60:1:60]
cut_y = -60; // [-60:1:60]
cut_z = -60; // [-60:1:60]
top_open = 20; // [0:1:80]

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
        // big hole for PCB
        translate([main_pos[0],main_pos[1],main_pos[2]]){
            cube([
                90,
                57,
                main_vol[2]-5
            ],center=true);
        };
      };
      // support for replay PCB
      translate([holes[0]/2,-holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([holes[0]/2,-holes[1]/2,-14])
        cylinder(h=2.9,r=1.3, center=true);

      translate([-holes[0]/2,-holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([-holes[0]/2,-holes[1]/2,-14])
        cylinder(h=2.9,r=1.3, center=true);

      translate([-holes[0]/2,holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([-holes[0]/2,holes[1]/2,-14])
        cylinder(h=2.9,r=1.3, center=true);

      translate([holes[0]/2,holes[1]/2,-15])
        cylinder(h=2.9,r=2, center=true);
      translate([holes[0]/2,holes[1]/2,-14])
        cylinder(h=2.9,r=1.3, center=true);
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

    
  }
}


module box_top_buttons(main_pos,main_vol,hcut,insert_width,insert_height,insert_tol) {
  // buttons
  translate([18,9,17.6])
    cube([2.8,2.8,5],center=true);
  translate([18.7,9,16.4])
    cube([4,6,2.6],center=true);

  translate([18,-9,17.6])
    cube([2.8,2.8,5],center=true);
  translate([18.7,-9,16.4])
    cube([4,6,2.6],center=true);

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
       // large hole
       translate([main_pos[0],main_pos[1],main_pos[2]])
          cube([90,57,main_vol[2]-3],center=true);     
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
      // pcb-usb pads
      translate([40.5,14,16.55])
        cube([6,6,4],center=true);           
      translate([25.5,14,16.55])
        cube([6,6,4],center=true);           
      translate([40.5,-14,16.55])
        cube([6,6,4],center=true);           
      translate([25.5,-14,16.55])
        cube([6,6,4],center=true);              
      // display pads
      translate([9,14.5,15])
        cube([3,3,7.4],center=true);           
      translate([-30,14.5,15])
        cube([3,3,7.4],center=true);
      translate([9,-14.5,15])
        cube([3,3,7.4],center=true);           
      translate([-30,-14.5,15])
        cube([3,3,7.4],center=true);           
    };    
    // display hole
    translate([-13.7,0,16])      
      cube([44.8,25,10],center=true);
    // button holes
    translate([18,9,11])
      cube([3,3,140],center=true);
    translate([18,-9,11])
      cube([3,3,140],center=true);    
    // screw holes
    translate([-holes[0]/2,holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);           
    translate([-holes[0]/2,-holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);           
    translate([holes[0]/2,-holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);           
    translate([holes[0]/2,holes[1]/2,-10])
      cylinder(h=10,r=1,center=true);            
    // screw pcb-usb pads
    translate([40.5,13.7,13])
      cylinder(h=10,d=2,center=true);
    translate([25.5,13.7,13])
      cylinder(h=10,d=2,center=true);
    translate([40.5,-13.7,13])
      cylinder(h=10,d=2,center=true);
    translate([25.5,-13.7,13])
      cylinder(h=10,d=2,center=true);
  };  
}

module render_model() {
  // models
  translate([0,0,-13]) {
    // ESP
    translate([-10,0,25+top_open])
      color("#30AA30")
       rotate([0,0,180]) {
         esp32_lilygo_t_display_s3();    
         //esp32_lilygo_t_display_s3_view();    
       }
    // 4 relays module
    translate([0,0,-top_open])
    color("#30AAAA")
      hw_316();
    // usb                                 
    translate([26+7.5,0,27.2+top_open])
      color("#AAAA30")
        usb_debug_pcb();
  }
}

module render_box() {
  // top
  translate([0,0,top_open])
  color("#AAAAFF")
  box_top(main_pos, main_vol,hcut,insert_width,insert_height,insert_tol);
  // buttons
  translate([0,0,top_open])
  color("#FFFF00")
  box_top_buttons();
  // bottom
  translate([0,0,-top_open])
  color("#AA30FF")
  box_bottom(main_pos, main_vol,hcut,insert_width,insert_height,insert_tol);
}


module render_model_cut() {
    // model
    difference() {
      render_model();
      translate([-50+cut_x,0,0])
        cube([120,120,120],center=true);
      translate([0,0,50-cut_z])
        cube([120,120,120],center=true);
      translate([0,50-cut_y,0])
        cube([120,120,120],center=true);
    }
    // box    
    difference() {
      render_box();
      translate([-50+cut_x,0,0])
        cube([120,120,120],center=true);
      translate([0,0,50-cut_z])
        cube([120,120,120],center=true);
      translate([0,50-cut_y,0])
        cube([120,120,120],center=true);
    }

}

// volumes
main_pos=[4,0,0.3];
main_vol=[100,66,38];
hcut=0.2;
insert_width=3;
insert_height=6;
insert_tol=0.05;


render_model_cut();

  
  
