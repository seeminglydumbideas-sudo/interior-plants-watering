module esp32_lilygo_t_display_s3() {
    pcb= [61.0,25.7,1.2];
    disp=[56.1,25.7,5.1];
    
    usb=[9,9,3.3]; 
    
    bt=[3.5,4.3,2.4];
    rst=[2,2,1];    
    
    union() {       
      // PCB
      cube(pcb, center=true);           
      // screen        
      translate([(pcb[0]-disp[0])/2,0,(pcb[2]+disp[2])/2])
        cube(disp, center=true);           
        
      // USB
      translate([-(pcb[0]-usb[0])/2-2,0,(pcb[2]+usb[2])/2])
        cube(usb, center=true);    
         
      // buttons
      translate([-(pcb[0]-bt[0])/2+0.6,+9,(pcb[2]+bt[2])/2])        
        cube(bt, center=true);    
      translate([-(pcb[0]-bt[0])/2+0.6,-9,(pcb[2]+bt[2])/2])
        cube(bt, center=true);
      // reset button (other side of the pcb -- +y edge, not -y)
      translate([-pcb[0]/2+12,pcb[1]/2,(pcb[2]+rst[2]+1)/2])
        cube(rst, center=true);
      translate([-pcb[0]/2+12,pcb[1]/2-1,pcb[2]/2+1])
        cube([4,2,2], center=true);
      // safe sapce
      translate([-3.5,0,-4])
        cube([48,25,8], center=true);    
    }
}

module esp32_lilygo_t_display_s3_view() {
  pcb= [61.0,25.7,1.2];
  dispv=[56.1-7-4.5,25.6,10]; // visible
  translate([(pcb[0]-dispv[0])/2-4.5,0,5.5])
    cube(dispv, center=true);   
}



color("#30AA30")
rotate([0,0,0])
  esp32_lilygo_t_display_s3();    
color("#30AAAA")
rotate([0,0,0])
  esp32_lilygo_t_display_s3_view();