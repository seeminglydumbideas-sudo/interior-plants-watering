use <sensor.scad>

// ================
// holes
// ================
module hole_ESP() {
   translate([0,0,2]) {
    union(){
     translate([0,-7,3.55])// (v8.1) 3mm down
       rotate([0,0,0]) {
        cube([18.2,18.5,11.1],center=true); // space for cables + esp32
        cube([15,21.2,11.1],center=true); // only keep 
        translate([0,0,1.5])
         esp32(0.2);
       };
      translate([0,-15.6,7.3]) // (v8.1) 3mm down
      cable(0.2);
    }        
  }
}

module hole_model(batt_length) {
  // hole : esp+cable
  hole_ESP();
  // hole : sensor
  translate([0,-45,0])
    sensor(0.2, false);  
  // hole : sensor electronic to align to other holes
  translate([0,-9.3,1.6])
    cube([18.2,25.8,2],center=true);
  // battery
  translate([0,10+batt_length/2,4]) // -> 1mm down
    battery(batt_length,0.2);    
  // antenna
  translate([0,27,12.1])// (v8.1) ~2mm down
    antenna([1,2,0.2]);        
  // hole : battery-sensor_esp32
  translate([0, 23, 5.55])
    cube([15,40,11.1],center=true);
}

//color("#a09090",0.5)
//  model(63);
color("#f0a090",0.5)
  hole_model(63);
