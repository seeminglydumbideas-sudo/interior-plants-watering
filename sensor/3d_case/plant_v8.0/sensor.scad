use <MCAD/boxes.scad>

//$fn=100;

// large battery 1200mAh : 63
// small battery  600mAh : 38
module battery(length, tol=0) {
    cube([21+tol,length+tol,10+tol],center=true);
}

function inc(arr,tol) = [arr[0]+tol,arr[1]+tol,arr[2]+tol];
function incA(arr,tol) = [arr[0]+tol[0],arr[1]+tol[1],arr[2]+tol[2]];

module esp32(tol=0) {
    pcb=[17.7,21.1,1.3];
    usb=[9,7.3,3.3];
    chip=[13,14,2.1];
    resistors=[17,18,3.6];

    union() {
      // PCB
      cube(inc(pcb, tol), center=true);    
      // USB
      translate([0,-pcb[1]/2+usb[1]/2-1.5,usb[2]/2+pcb[2]/2])
       cube(inc(usb, tol), center=true);    
      // chips
      translate([0,+3.5,chip[2]/2+pcb[2]/2])
       cube(chip, center=true);
      // place for resistors (voltage and battery cables)
      translate([0,0,-resistors[2]/2-pcb[2]/2])
       cube(inc(resistors, tol), center=true);
      // BLE antenna connector
      translate([0,pcb[1]/2,pcb[2]/2+1.5])
       cube(inc([5,5,3], tol), center=true);


    }
}

module antenna(tol=[0,0,0]) {
    union(){        
     cube(incA([22,42,0.5], tol),center=true);
     translate([0,0,-1])        
      cube(incA([10,30,2], tol),center=true);
    }
}

module sensor(tol=0, do_hole=true) {
    pcb=[22.5,97,1.3];
    union() {
      difference() {
        cube(inc(pcb, tol), center=true);    
        if (do_hole) {
          translate([+pcb[0]/2,pcb[1]/2-20.5,0])
            cylinder(6,r=4/2,center=true);
          translate([-pcb[0]/2,pcb[1]/2-20.5,0])
            cylinder(6,r=4/2,center=true);
        }
      }
      // limit line
      translate([0,pcb[1]/2-31,0])
        cube([pcb[0]-8,1,pcb[2]+0.2],center=true);
      // soudures
      translate([0,pcb[1]/2-4,-pcb[2]/2+0.1])
        cube([8,6,2],center=true);
      // chip
      translate([0,pcb[1]/2-13,1.5])
        cube([17,25,2],center=true);
    }
}


module cable(tol=0) {
   translate([0,-4,0])
     cube(inc([8.2,8,2.5],tol),center=true);// usb
   translate([0,-18.9,0])
     cube(inc([12.5,30-4,7],tol),center=true); // plug
}

module model(batt_length, tol=0) {
  translate([0,-45,0])
    sensor(tol);  
    
  translate([0,25,14.3])
    antenna([tol,tol,tol]);    
  translate([0,10+batt_length/2,5])
    battery(batt_length,tol);

  translate([0,0,2]) {
    translate([0,-7,8])
      rotate([0,0,0])
        esp32(tol);
    translate([0,-15.6,10.3])
      cable(tol);
  }
}


batS=38;
batL=63;
model(batS);




