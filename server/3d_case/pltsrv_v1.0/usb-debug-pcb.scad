module usb_debug_pcb() {
    pcb=[20.4,36.3,0.7];
    usbf=[10.5,9.0,3.1];    
    usbm=[10.8,8.2,2.4];
    union() {
      // PCB
      difference() {
        cube(pcb, center=true);
        translate([7,-(25+2.5)/2,-4])
          cylinder(h=8,d=3.5);
        translate([-8,-(25+2.5)/2,-4])
          cylinder(h=8,d=3.5);
        translate([7,(25+2.5)/2,-4])
          cylinder(h=8,d=3.5);
        translate([-8,(25+2.5)/2,-4])
          cylinder(h=8,d=3.5);
      }
      //25 31.5
      // USB
      translate([+(pcb[0]+usbf[0])/2,0,0])
        cube(usbf, center=true);    
      translate([-(pcb[0]+usbm[0])/2,0,0])
        cube(usbm, center=true);    
    }
}

$fn=100;
color("#AAAA30")
rotate([0,0,0])
  usb_debug_pcb();    
