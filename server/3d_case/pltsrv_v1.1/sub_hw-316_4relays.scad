module hw_316() {
    //https://www.handsontec.com/dataspecs/4Ch-relay.pdf
    pcb=[76,56,1.1];
    holes=[69.6,49.4,3];
    relays=[66.5,27.5,15.5];
    
    union() {       
      // PCB-holes
      difference() {
        // PCB
        cube(pcb, center=true);           
        // holes
        translate([holes[0]/2,holes[1]/2,0])
          cylinder(h=2,r=holes[2]/2,center=true);
        translate([-holes[0]/2,holes[1]/2,0])
          cylinder(h=2,r=holes[2]/2,center=true);
        translate([holes[0]/2,-holes[1]/2,0])
          cylinder(h=2,r=holes[2]/2,center=true);
        translate([-holes[0]/2,-holes[1]/2,0])
          cylinder(h=2,r=holes[2]/2,center=true);
      }
      // relays
      translate([0,-(pcb[1]-relays[1])/2+5,relays[2]/2+0.5])
        cube(relays, center=true);
      // connectors
      translate([-22,20,13])
        cube([16,10,25], center=true);
      // bottom safe space
      translate([0,0,-1])
        cube([pcb[0]-1,pcb[1]-12,1], center=true);
      translate([0,0,-1])
        cube([pcb[0]-12,pcb[1]-1,1], center=true);
    }
}


color("#30AAAA")
  hw_316();