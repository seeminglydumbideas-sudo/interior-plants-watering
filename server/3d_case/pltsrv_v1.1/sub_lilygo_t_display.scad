module esp32_lilygo_t_display() {
    pcb=[51.5,25.4,1.1];
    disp=[31,17.5,2.4];
    usb=[9,7.3,3.3];    
    bt=[4.3,3.5,2.4];
    rst=[2,2,1];    
    union() {       
      // PCB
      cube(pcb, center=true);           
      // screen        
      translate([(pcb[0]-disp[0])/2-7,0,1.7])
       cube(disp, center=true);           
      // USB
      translate([-(pcb[0]-usb[0])/2-1.5,0,(pcb[2]+usb[2])/2])
        cube(usb, center=true);    
      // buttons
      translate([-(pcb[0]-bt[0])/2+1,8,(pcb[2]+bt[2])/2])        
        cube(bt, center=true);    
      translate([-(pcb[0]-bt[0])/2+1,-8,(pcb[2]+bt[2])/2])        
        cube(bt, center=true);    
      // reset button
      translate([-pcb[0]/2+12,-pcb[1]/2,(pcb[2]+rst[2]+1)/2])
        cube(rst, center=true);    
      translate([-pcb[0]/2+12,-pcb[1]/2+1,pcb[2]/2+1])
        cube([4,2,2], center=true);    
      // safe sapce
      translate([-2.75,0,-4])
        cube([46,25,7], center=true);    
    }
}

color("#30AA30")
rotate([0,0,0])
  esp32_lilygo_t_display();    
