use <sensor.scad>

// ================
// holes
// ================
module hole_ESP() {
   translate([0,0,2]) {
    union(){
     translate([0,-7,6.5])
       rotate([0,0,0]) {
        cube([18,18.5,12.1],center=true);
        //translate([0,0.3,0])
        // cube([16,21.5,12],center=true);
        translate([0,0,1.5])
         esp32(0.2);
       };
      translate([0,-15.6,10.3])
      cable(0.2);
    }        
  }
}

module hole_model(batt_length) {  
  // esp+cable
  hole_ESP();
  // sensor
  translate([0,-45,0])
    sensor(0.2, false);  
  // hole esp32-sensor
  //translate([0,0,25])
  //  cube([10,20,10],center=true);
  // battery
  translate([0,10+batt_length/2,5])
    battery(batt_length,0.2);    
  // hole : battery-sensor_esp32
  translate([0,6,4])
    cube([18,10,7],center=true);
  // hole : sensor electronic to align to other holes
  translate([0,-7,1.5])
    cube([18,30,2],center=true);
  // hole : cables aligned to other holes
  translate([0,7,5])
    cube([10,15,10],center=true);


  // antenna
  translate([0,25,14.3])
    antenna([1,2,0.2]);        
  // hole antenna-*
  translate([0,21,12])
    cube([13.4,40,4.1],center=true);
}

color("#f0a090",0.5)
  hole_model(63);
color("#a09090",0.5)
  model(63);

